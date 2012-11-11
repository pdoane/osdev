// ------------------------------------------------------------------------------------------------
// gfx/gfx.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfx.h"
#include "gfx/gfxpci.h"
#include "gfx/gtt.h"
#include "gfx/gfxmem.h"
#include "gfx/gfxdisplay.h"
#include "gfx/gfxring.h"
#include "console/console.h"
#include "cpu/io.h"
#include "input/input.h"
#include "mem/vm.h"
#include "net/rlog.h"
#include "pci/registry.h"
#include "stdlib/limits.h"
#include "stdlib/string.h"
#include "time/pit.h"

#include "cursor.c"

#define DEVICE_HD3000       0x0162
#define DEVICE_PANTHERPOINT 0x1e00

// ------------------------------------------------------------------------------------------------
typedef struct GfxDevice
{
    bool            active;

    GfxPci          pci;
    GfxGTT          gtt;
    GfxMemManager   memManager;
    GfxDisplay      display;

    GfxObject       surface;
    GfxObject       cursor;

    GfxRing         renderRing;
    GfxObject       renderContext;

    GfxObject       batchBuffer;

    GfxObject       colorCalcStates;
    GfxObject       blendStates;
    GfxObject       depthStencilStates;
    GfxObject       bindingTables[SHADER_COUNT];
    GfxObject       samplerTables[SHADER_COUNT];
    GfxObject       ccViewportTable;

    GfxObject       triangleVB;
} GfxDevice;

GfxDevice s_gfxDevice;

// ------------------------------------------------------------------------------------------------
void EnterForceWake()
{
    RlogPrint("Trying to entering force wake...\n");

    int trys = 0;
    int forceWakeAck;
    do {
        ++trys;
        forceWakeAck = GfxRead32(&s_gfxDevice.pci, FORCE_WAKE_MT_ACK);
        ConsolePrint("Waiting for Force Ack to Clear: Try=%d - Ack=0x%X\n", trys, forceWakeAck);
    } while (forceWakeAck != 0);

    RlogPrint("  ACK cleared...\n");

    GfxWrite32(&s_gfxDevice.pci, FORCE_WAKE_MT, MASKED_ENABLE(1));
    GfxRead32(&s_gfxDevice.pci, ECOBUS);

    RlogPrint("Wake written...\n");
    do {
        ++trys;
        forceWakeAck = GfxRead32(&s_gfxDevice.pci, FORCE_WAKE_MT_ACK);
        ConsolePrint("Waiting for Force Ack to be Set: Try=%d - Ack=0x%X\n", trys, forceWakeAck);
    } while (forceWakeAck == 0);

    RlogPrint("...Force Wake done\n");
}

// ------------------------------------------------------------------------------------------------
void ExitForceWake()
{
    GfxWrite32(&s_gfxDevice.pci, FORCE_WAKE_MT, MASKED_DISABLE(1));
    GfxRead32(&s_gfxDevice.pci, ECOBUS);
}

// ------------------------------------------------------------------------------------------------
static void GfxPrintPortState()
{
    RlogPrint("HDMI_CTL_B: 0x%08X\n", GfxRead32(&s_gfxDevice.pci, HDMI_CTL_B));
    RlogPrint("HDMI_CTL_C: 0x%08X\n", GfxRead32(&s_gfxDevice.pci, HDMI_CTL_C));
    RlogPrint("HDMI_CTL_D: 0x%08X\n", GfxRead32(&s_gfxDevice.pci, HDMI_CTL_D));
}

// ------------------------------------------------------------------------------------------------
/*
static void GfxPrintPipeState()
{
    //for (uint i = 0; i < 3; ++i)
    uint i = 0;
    {
        uint pipe = 0x1000 * i;
        RlogPrint("PIPE %d\n", i);

        // Output Timing
        RlogPrint("  CONF: %08x\n", GfxRead32(PIPE_CONF_A + pipe));

        u32 htotal = GfxRead32(PIPE_HTOTAL_A + pipe);
        uint hactive = (htotal & 0xffff) + 1;
        uint htotal_size = ((htotal >> 16) & 0xffff) + 1;
        RlogPrint("  HTOTAL: %08x %d,%d\n", htotal, hactive, htotal_size);

        u32 hblank = GfxRead32(PIPE_HBLANK_A + pipe);
        uint hblank_start = (hblank & 0xffff) + 1;
        uint hblank_end = ((hblank >> 16) & 0xffff) + 1;
        RlogPrint("  HBLANK: %08x %d,%d\n", hblank, hblank_start, hblank_end);

        u32 hsync = GfxRead32(PIPE_HSYNC_A + pipe);
        uint hsync_start = (hsync & 0xffff) + 1;
        uint hsync_end = ((hsync >> 16) & 0xffff) + 1;
        RlogPrint("  HSYNC: %08x %d,%d\n", hsync, hsync_start, hsync_end);

        u32 vtotal = GfxRead32(PIPE_VTOTAL_A + pipe);
        uint vactive = (vtotal & 0xffff) + 1;
        uint vtotal_size = ((vtotal >> 16) & 0xffff) + 1;
        RlogPrint("  VTOTAL: %08x %d,%d\n", vtotal, vactive, vtotal_size);

        u32 vblank = GfxRead32(PIPE_VBLANK_A + pipe);
        uint vblank_start = (vblank & 0xffff) + 1;
        uint vblank_end = ((vblank >> 16) & 0xffff) + 1;
        RlogPrint("  VBLANK: %08x %d,%d\n", vblank, vblank_start, vblank_end);

        u32 vsync = GfxRead32(PIPE_VSYNC_A + pipe);
        uint vsync_start = (vsync & 0xffff) + 1;
        uint vsync_end = ((vsync >> 16) & 0xffff) + 1;
        RlogPrint("  VSYNC: %08x %d,%d\n", vsync, vsync_start, vsync_end);

        u32 srcsz = GfxRead32(PIPE_SRCSZ_A + pipe);
        uint width = ((srcsz >> 16) & 0xfff) + 1;
        uint height = (srcsz & 0xffff) + 1;
        RlogPrint("  SRCSZ: %08x %dx%d\n", srcsz, width, height);

        // Cursor Plane
        RlogPrint("  CUR_CTL: %08x\n", GfxRead32(CUR_CTL_A + pipe));
        RlogPrint("  CUR_BASE: %08x\n", GfxRead32(CUR_BASE_A + pipe));
        RlogPrint("  CUR_POS: %08x\n", GfxRead32(CUR_POS_A + pipe));

        // Primary Plane
        RlogPrint("  PRI_CTL: %08x\n", GfxRead32(PRI_CTL_A + pipe));
        RlogPrint("  PRI_LINOFF: %08x\n", GfxRead32(PRI_LINOFF_A + pipe));
        RlogPrint("  PRI_STRIDE: %08x\n", GfxRead32(PRI_STRIDE_A + pipe));
        RlogPrint("  PRI_SURF: %08x\n", GfxRead32(PRI_SURF_A + pipe));
    }
}
*/

// ------------------------------------------------------------------------------------------------
void GfxInit(uint id, PciDeviceInfo *info)
{
    if (!(((info->classCode << 8) | info->subclass) == PCI_DISPLAY_VGA &&
        info->progIntf == 0))
    {
        return;
    }

    if ((info->vendorId != VENDOR_INTEL) ||
        (info->deviceId != DEVICE_HD3000))
    {
        ConsolePrint("Graphics Controller not recognised!\n");
        return;
    }


    memset(&s_gfxDevice, 0, sizeof(s_gfxDevice));
    s_gfxDevice.pci.id = id;
}

// ------------------------------------------------------------------------------------------------
static bool ValidateChipset()
{
    // Check we are on Pather Point Chipset
    uint isaBridgeId = PCI_MAKE_ID(0, 0x1f, 0);  // Assume location of ISA Bridge
    u16 classCode     = ((u16)PciRead8(isaBridgeId, PCI_CONFIG_CLASS_CODE) << 8) | (u16)PciRead8(isaBridgeId, PCI_CONFIG_SUBCLASS);
    if (classCode != PCI_BRIDGE_ISA)
    {
        ConsolePrint("Isa Bridge not found at expected location! (Found: 0x%X, Expected: 0x%X)\n", classCode, PCI_BRIDGE_ISA);
        return false;
    }

    u16 deviceId = PciRead16(isaBridgeId, PCI_CONFIG_DEVICE_ID) & 0xFF00;
    if (deviceId != DEVICE_PANTHERPOINT)
    {
        ConsolePrint("Chipset is not expect panther point (needed for display handling)! (Found: 0x%X, Expected: 0x%X)\n", deviceId, DEVICE_PANTHERPOINT);
        return false;
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
static void CreateStates()
{
    // Color Calc State
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.colorCalcStates, sizeof(ColorCalcState), 64);
    ColorCalcState *ccState = (ColorCalcState *)s_gfxDevice.colorCalcStates.cpuAddr;
    ccState->flags = 0;
    ccState->alphaRef.intVal = 0;
    ccState->constR = 1.0f;
    ccState->constG = 1.0f;
    ccState->constB = 1.0f;
    ccState->constA = 1.0f;

    // Blend State
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.blendStates, sizeof(BlendState), 64);
    BlendState *blendState = (BlendState *)s_gfxDevice.blendStates.cpuAddr;
    blendState->flags0 = 0;
    blendState->flags1 = 0;

    // Depth Stencil State
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.depthStencilStates, sizeof(DepthStencilState), 64);
    DepthStencilState *depthStencilState = (DepthStencilState *)s_gfxDevice.depthStencilStates.cpuAddr;
    depthStencilState->stencilFlags = 0;
    depthStencilState->stencilMasks = 0;
    depthStencilState->depthFlags = 0;

    // Shader State Tables
    for (uint i = 0; i < SHADER_COUNT; ++i)
    {
        // Binding Tables
        GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.bindingTables[i], BINDING_TABLE_SIZE * sizeof(u32), 32);
        u32* bindingTable = (u32 *)s_gfxDevice.bindingTables[i].cpuAddr;
        memset(bindingTable, 0, BINDING_TABLE_SIZE * sizeof(u32));

        // Sampler Tables
        GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.samplerTables[i], SAMPLER_TABLE_SIZE * sizeof(SamplerState), 32);
        SamplerState* samplerTable = (SamplerState *)s_gfxDevice.samplerTables[i].cpuAddr;
        memset(samplerTable, 0, SAMPLER_TABLE_SIZE * sizeof(SamplerState));
    }

    // Viewport State (SFClipViewport and ScissorRect can be disabled in the SF state, so ignore for now)
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.ccViewportTable, VIEWPORT_TABLE_SIZE * sizeof(CCViewport), 32);
    CCViewport *ccViewport = (CCViewport *)s_gfxDevice.ccViewportTable.cpuAddr;
    ccViewport->minDepth = -F32_MAX;
    ccViewport->maxDepth = F32_MAX;
}

// ------------------------------------------------------------------------------------------------
static void CreateTriangle()
{
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.triangleVB, sizeof(float) * 9, sizeof(float));
    float* p = (float *)s_gfxDevice.triangleVB.cpuAddr;

    *p++ = 0.0f;
    *p++ = 0.5f;
    *p++ = 0.5f;

    *p++ = 0.5f;
    *p++ = -0.5f;
    *p++ = 0.5f;

    *p++ = -0.5f;
    *p++ = -0.5f;
    *p++ = 0.5f;
}

// ------------------------------------------------------------------------------------------------
static void CreateTestBatchBuffer()
{
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.batchBuffer, 4 * KB, 4 * KB);
    u32 *cmd = (u32 *)s_gfxDevice.batchBuffer.cpuAddr;

    // Switch to 3D pipeline
    *cmd++ = PIPELINE_SELECT(PIPELINE_3D);

    // Update base addresses - just use 0 for everything with no caching or upper bounds
    *cmd++ = STATE_BASE_ADDRESS;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;
    *cmd++ = BASE_ADDRESS_MODIFY;

    // Color Calc State
    *cmd++ = _3DSTATE_CC_STATE_POINTERS;
    *cmd++ = s_gfxDevice.colorCalcStates.gfxAddr;

    // Blend State
    *cmd++ = _3DSTATE_BLEND_STATE_POINTERS;
    *cmd++ = s_gfxDevice.blendStates.gfxAddr;

    // Depth/Stencil State
    *cmd++ = _3DSTATE_DEPTH_STENCIL_STATE_POINTERS;
    *cmd++ = s_gfxDevice.depthStencilStates.gfxAddr;

    // Binding Tables
    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_VS;
    *cmd++ = s_gfxDevice.bindingTables[SHADER_VS].gfxAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_HS;
    *cmd++ = s_gfxDevice.bindingTables[SHADER_HS].gfxAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_DS;
    *cmd++ = s_gfxDevice.bindingTables[SHADER_DS].gfxAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_GS;
    *cmd++ = s_gfxDevice.bindingTables[SHADER_GS].gfxAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_PS;
    *cmd++ = s_gfxDevice.bindingTables[SHADER_PS].gfxAddr;

    // Sampler Tables
    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_VS;
    *cmd++ = s_gfxDevice.samplerTables[SHADER_VS].gfxAddr;

    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_HS;
    *cmd++ = s_gfxDevice.samplerTables[SHADER_HS].gfxAddr;

    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_DS;
    *cmd++ = s_gfxDevice.samplerTables[SHADER_DS].gfxAddr;

    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_GS;
    *cmd++ = s_gfxDevice.samplerTables[SHADER_GS].gfxAddr;

    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_PS;
    *cmd++ = s_gfxDevice.samplerTables[SHADER_PS].gfxAddr;

    // Viewport State
    *cmd++ = _3DSTATE_VIEWPORT_STATE_POINTERS_CC;
    *cmd++ = s_gfxDevice.ccViewportTable.gfxAddr;

    *cmd++ = _3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP;
    *cmd++ = 0;     // can be disabled in the 3DSTATE_SF

    *cmd++ = _3DSTATE_SCISSOR_STATE_POINTERS;
    *cmd++ = 0;     // can be disabled in the 3DSTATE_SF

    // Index Buffer
    *cmd++ = _3DSTATE_INDEX_BUFFER;
    *cmd++ = 0;
    *cmd++ = 0;

    // Vertex Buffer
    *cmd++ = _3DSTATE_VERTEX_BUFFERS(1);
    *cmd++ =
          (0 << VB_INDEX_SHIFT)
        | VB_ADDRESS_MODIFY
        | ((sizeof(float) * 3) << VB_PITCH_SHIFT);
    *cmd++ = s_gfxDevice.triangleVB.gfxAddr;
    *cmd++ = s_gfxDevice.triangleVB.gfxAddr + sizeof(float) * 9 - 1;
    *cmd++ = 0;

    // Vertex Elements
    *cmd++ = _3DSTATE_VERTEX_ELEMENTS(1);
    *cmd++ =
          (0 << VE_INDEX_SHIFT)
        | VE_VALID
        | (FMT_R32G32B32_FLOAT << VE_FORMAT_SHIFT)
        | (0 << VE_OFFSET_SHIFT);
    *cmd++ =
          (VFCOMP_STORE_SRC << VE_COMP0_SHIFT)
        | (VFCOMP_STORE_SRC << VE_COMP1_SHIFT)
        | (VFCOMP_STORE_SRC << VE_COMP2_SHIFT)
        | (VFCOMP_STORE_1_FP << VE_COMP3_SHIFT);

    // Vertex Shader
    *cmd++ = _3DSTATE_VS;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    // Hull Shader
    *cmd++ = _3DSTATE_HS;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    // Tesselation Engine
    *cmd++ = _3DSTATE_TE;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    // Domain Shader
    *cmd++ = _3DSTATE_DS;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    // Geometry Shader
    *cmd++ = _3DSTATE_GS;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    // Stream Out
    *cmd++ = _3DSTATE_STREAMOUT;
    *cmd++ = 0;
    *cmd++ = 0;

    // Dummy Draw (needed after MI_SET_CONTEXT or PIPELINE_SELECT)
    *cmd++ = _3DPRIMITIVE;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    /*
    // Triangle Draw
    *cmd++ = _3DPRIMITIVE;
    *cmd++ = (_3DPRIM_TRILIST << PRIM_TOPOLOGY_SHIFT);
    *cmd++ = 3;
    *cmd++ = 0;
    *cmd++ = 1;
    *cmd++ = 0;
    *cmd++ = 0;*/

    // Debug
    *cmd++ = MI_STORE_DATA_INDEX;
    *cmd++ = 0; // offset
    *cmd++ = 0xabcd0123;
    *cmd++ = 0;

    // End Batch Buffer
    *cmd++ = MI_BATCH_BUFFER_END;

    RlogPrint("Batch Buffer Size = %08x\n", (u8 *)cmd - s_gfxDevice.batchBuffer.cpuAddr);
}

// ------------------------------------------------------------------------------------------------
void GfxStart()
{
    if (!s_gfxDevice.pci.id)
    {
        ConsolePrint("Graphics not supported\n");
        return;
    }
    
    if (!ValidateChipset())
    {
        return;
    }

    GfxInitPci(&s_gfxDevice.pci);
    GfxInitGtt(&s_gfxDevice.gtt, &s_gfxDevice.pci);
    GfxInitMemManager(&s_gfxDevice.memManager, &s_gfxDevice.gtt, &s_gfxDevice.pci);
    GfxInitDisplay(&s_gfxDevice.display);
    GfxDisableVga(&s_gfxDevice.pci);

    // Need to force out of D6 state before we can read/write to registers
    EnterForceWake();
    {
        GfxMemEnableSwizzle(&s_gfxDevice.pci);
    }
    ExitForceWake();

    // Allocate Surface - 256KB aligned, +512 PTEs
    uint surfaceMemSize = 16 * MB;         // TODO: compute appropriate surface size
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.surface, surfaceMemSize, 256 * KB);
    memset(s_gfxDevice.surface.cpuAddr, 0x77, 720 * 400 * 4);

    // Allocate Cursor - 64KB aligned, +2 PTEs
    uint cursorMemSize = 64 * 64 * sizeof(u32) + 8 * KB;
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.cursor, cursorMemSize, 64 * KB);
    memcpy(s_gfxDevice.cursor.cpuAddr, cursor_image.pixel_data, 64 * 64 * sizeof(u32));

    // Allocate Render Ring
    GfxInitRing(&s_gfxDevice.renderRing, RING_RCS, &s_gfxDevice.memManager);

    // Allocate Render Context
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.renderContext, 4 * KB, 4 * KB);

    // Allocate States
    CreateStates();

    // Allocate Gfx Buffers
    CreateTriangle();

    // Allocate Batch Buffer
    CreateTestBatchBuffer();

    // Setup Primary Plane
    uint width = 720;                       // TODO: mode support
    //uint height = 400;
    uint stride = (width * sizeof(u32) + 63) & ~63;   // 64-byte aligned

    GfxWrite32(&s_gfxDevice.pci, PRI_CTL_A, PRI_PLANE_ENABLE | PRI_PLANE_32BPP);
    GfxWrite32(&s_gfxDevice.pci, PRI_LINOFF_A, 0);
    GfxWrite32(&s_gfxDevice.pci, PRI_STRIDE_A, stride);
    GfxWrite32(&s_gfxDevice.pci, PRI_SURF_A, s_gfxDevice.surface.gfxAddr);

    // Setup Cursor Plane
    GfxWrite32(&s_gfxDevice.pci, CUR_CTL_A, CUR_MODE_ARGB | CUR_MODE_64_32BPP);
    GfxWrite32(&s_gfxDevice.pci, CUR_BASE_A, s_gfxDevice.cursor.gfxAddr);

    // Log initial port state
    GfxPrintPortState();

    // MWDD FIX: Enable MSI

    // Enable render ring
    GfxSetRing(&s_gfxDevice.pci, &s_gfxDevice.renderRing);
    GfxPrintRingState(&s_gfxDevice.pci, &s_gfxDevice.renderRing);

    // Write test buffer stream
    GfxRing *ring = &s_gfxDevice.renderRing;
    u32 *cmd;

    // MI_STORE_DATA_INDEX (test memory is being written)
    cmd = GfxBeginCmd(ring, 4);
    *cmd++ = MI_STORE_DATA_INDEX;
    *cmd++ = 0; // offset
    *cmd++ = 0x12345678;
    *cmd++ = 0;
    GfxEndCmd(&s_gfxDevice.pci, ring, cmd);

    // PIPE_CONTROL with CS_STALL
    cmd = GfxBeginCmd(ring, 6);
    *cmd++ = PIPE_CONTROL;
    *cmd++ = PIPE_CONTROL_SCOREBOARD_STALL
        | PIPE_CONTROL_CS_STALL;
    *cmd++ = 0; // address
    *cmd++ = 0; // immediate data (low)
    *cmd++ = 0; // immediate data (high)
    *cmd++ = MI_NOOP;
    GfxEndCmd(&s_gfxDevice.pci, ring, cmd);

    // PIPE_CONTROL for flush
    cmd = GfxBeginCmd(ring, 6);
    *cmd++ = PIPE_CONTROL;
    *cmd++ = PIPE_CONTROL_DEPTH_CACHE_FLUSH
        | PIPE_CONTROL_STATE_CACHE_INVALIDATE
        | PIPE_CONTROL_CONST_CACHE_INVALIDATE
        | PIPE_CONTROL_VF_CACHE_INVALIDATE
        | PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE
        | PIPE_CONTROL_INSTR_CACHE_INVALIDATE
        | PIPE_CONTROL_RENDER_TARGET_CACHE_FLUSH
        | PIPE_CONTROL_WRITE_IMM
        | PIPE_CONTROL_TLB_INVALIDATE
        | PIPE_CONTROL_CS_STALL
        | PIPE_CONTROL_USE_GGTT;
    *cmd++ = s_gfxDevice.renderRing.statusPage.gfxAddr;
    *cmd++ = 2; // immediate data (low)
    *cmd++ = 0; // immediate data (high)
    *cmd++ = MI_NOOP;
    GfxEndCmd(&s_gfxDevice.pci, ring, cmd);

    // MI_SET_CONTEXT
    cmd = GfxBeginCmd(ring, 2);
    *cmd++ = MI_SET_CONTEXT;
    *cmd++ = s_gfxDevice.renderContext.gfxAddr
        | MI_GTT_ADDR
        | MI_EXT_STATE_SAVE
        | MI_EXT_STATE_RESTORE
        | MI_RESTORE_INHIBIT;
    GfxEndCmd(&s_gfxDevice.pci, ring, cmd);

    // TODO - a PIPE_CONTROL with Post-Sync Operation set and a depth stall
    // needs to be sent just prior to updating most of the state operations.

    // MI_BATCH_BUFFER_START
    cmd = GfxBeginCmd(ring, 2);
    *cmd++ = MI_BATCH_BUFFER_START;
    *cmd++ = s_gfxDevice.batchBuffer.gfxAddr;
    GfxEndCmd(&s_gfxDevice.pci, ring, cmd);

    GfxPrintRingState(&s_gfxDevice.pci, ring);

    s_gfxDevice.active = true;
}

// ------------------------------------------------------------------------------------------------
void GfxPoll()
{
    if (!s_gfxDevice.active)
    {
        return;
    }

    static u32 lastCursorPos = 0;

    // Update cursor position
    u32 cursorPos = (g_mouseY << 16) | g_mouseX;
    if (cursorPos != lastCursorPos)
    {
        lastCursorPos = cursorPos;
        GfxWrite32(&s_gfxDevice.pci, CUR_POS_A, cursorPos);
        GfxPrintRingState(&s_gfxDevice.pci, &s_gfxDevice.renderRing);
    }
}
