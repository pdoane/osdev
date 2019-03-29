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

#define SCREEN_WIDTH        720
#define SCREEN_HEIGHT       400

// ------------------------------------------------------------------------------------------------
enum ShaderObject
{
    VS_PASSTHROUGH_P,
    PS8_SOLID_FF8040FF,
    PS16_SOLID_FF8040FF,
    PS32_SOLID_FF8040FF,
    SHADER_OBJ_COUNT
};

// ------------------------------------------------------------------------------------------------
typedef struct GfxHeap
{
    GfxObject       storage;
    uint            next;
    uint            size;
} GfxHeap;

// ------------------------------------------------------------------------------------------------
typedef struct GfxDevice
{
    bool                active;

    GfxPci              pci;
    GfxGTT              gtt;
    GfxMemManager       memManager;
    GfxDisplay          display;

    GfxObject           surface;
    GfxObject           cursor;

    GfxRing             renderRing;
    GfxObject           renderContext;

    GfxHeap             dynamicHeap;
    ColorCalcState     *colorCalcStateTable;
    BlendState         *blendStateTable;
    DepthStencilState  *depthStencilStateTable;
    u32                *bindingTable[SHADER_COUNT];
    CCViewport         *ccViewportTable;
    SFClipViewport     *sfClipViewportTable;
    SurfaceState       *surfaceState;

    GfxHeap             surfaceHeap;
    SamplerState       *samplerTable[SHADER_COUNT];

    GfxHeap             instructionHeap;
    u32                *shaderObjs[SHADER_OBJ_COUNT];

    GfxObject           batchBuffer;
    GfxObject           triangleVB;
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
static void GfxPrintStatistics()
{
    EnterForceWake();
    {
        RlogPrint("Stats:\n");
        RlogPrint("  IA_VERTICES_COUNT: %d\n", GfxRead64(&s_gfxDevice.pci, IA_VERTICES_COUNT));
        RlogPrint("  IA_PRIMITIVES_COUNT: %d\n", GfxRead64(&s_gfxDevice.pci, IA_PRIMITIVES_COUNT));
        RlogPrint("  VS_INVOCATION_COUNT: %d\n", GfxRead64(&s_gfxDevice.pci, VS_INVOCATION_COUNT));
        RlogPrint("  CL_INVOCATION_COUNT: %d\n", GfxRead64(&s_gfxDevice.pci, CL_INVOCATION_COUNT));
        RlogPrint("  CL_PRIMITIVES_COUNT: %d\n", GfxRead64(&s_gfxDevice.pci, CL_PRIMITIVES_COUNT));
        RlogPrint("  PS_INVOCATION_COUNT: %d\n", GfxRead64(&s_gfxDevice.pci, PS_INVOCATION_COUNT));
        RlogPrint("  PS_DEPTH_COUNT: %d\n", GfxRead64(&s_gfxDevice.pci, PS_DEPTH_COUNT));
    }
    ExitForceWake();
}

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
static void InitHeap(GfxDevice *device, GfxHeap *heap, uint size, uint align)
{
    GfxAlloc(&device->memManager, &heap->storage, size, align);
    memset(heap->storage.cpuAddr, 0, size);
    heap->size = size;
    heap->next = 0;
}

// ------------------------------------------------------------------------------------------------
static u8 *HeapAlloc(GfxHeap *heap, uint size, uint align)
{
    uint result = heap->next;
    uint offset = result & (align - 1);
    if (offset)
        result += align - offset;

    heap->next = result + size;
    return heap->storage.cpuAddr + result;
}

// ------------------------------------------------------------------------------------------------
static void CreateStates(GfxDevice *device)
{
    // Create heaps
    InitHeap(device, &device->dynamicHeap, 8 * KB, 64);
    InitHeap(device, &device->surfaceHeap, 8 * KB, 64);

    // Color Calc State - Dynamic State
    device->colorCalcStateTable = (ColorCalcState *)HeapAlloc(&device->dynamicHeap, sizeof(ColorCalcState), COLOR_CALC_TABLE_ALIGN);
    ColorCalcState *ccState = device->colorCalcStateTable;
    ccState->flags = 0;
    ccState->alphaRef.intVal = 0;
    ccState->constR = 1.0f;
    ccState->constG = 1.0f;
    ccState->constB = 1.0f;
    ccState->constA = 1.0f;

    // Blend State - Dynamic State
    device->blendStateTable = (BlendState *)HeapAlloc(&device->dynamicHeap, sizeof(BlendState), BLEND_TABLE_ALIGN);
    BlendState *blendState = device->blendStateTable;
    blendState->flags0 =
          BLEND_COLOR
        | (BLEND_FUNC_ADD << BLEND_FUNC_ALPHA_SHIFT)
        | (BLEND_FACTOR_ONE << BLEND_SRC_ALPHA_SHIFT)
        | (BLEND_FACTOR_ZERO << BLEND_DST_ALPHA_SHIFT)
        | (BLEND_FUNC_ADD << BLEND_FUNC_COLOR_SHIFT)
        | (BLEND_FACTOR_ONE << BLEND_SRC_COLOR_SHIFT)
        | (BLEND_FACTOR_ZERO << BLEND_DST_COLOR_SHIFT);
    blendState->flags1 = 0;

    // Depth Stencil State - Dynamic State
    device->depthStencilStateTable = (DepthStencilState *)HeapAlloc(&device->dynamicHeap, sizeof(DepthStencilState), DEPTH_STENCIL_TABLE_ALIGN);
    DepthStencilState *depthStencilState = device->depthStencilStateTable;
    depthStencilState->stencilFlags = 0;
    depthStencilState->stencilMasks = 0;
    depthStencilState->depthFlags = 0;

    // Initialize Empty Shader State Tables
    for (uint i = 0; i < SHADER_COUNT; ++i)
    {
        // Binding Tables - Surface State
        device->bindingTable[i] = (u32 *)HeapAlloc(&device->surfaceHeap, BINDING_TABLE_SIZE * sizeof(u32), BINDING_TABLE_ALIGN);
        u32 *bindingTable = device->bindingTable[i];
        memset(bindingTable, 0, BINDING_TABLE_SIZE * sizeof(u32));

        // Sampler Tables - Dynamic State
        device->samplerTable[i] = (SamplerState *)HeapAlloc(&device->dynamicHeap, SAMPLER_TABLE_SIZE * sizeof(SamplerState), SAMPLER_TABLE_ALIGN);
        SamplerState *samplerTable = device->samplerTable[i];
        memset(samplerTable, 0, SAMPLER_TABLE_SIZE * sizeof(SamplerState));
    }

    // Viewport State - Dynamic State
    device->ccViewportTable = (CCViewport *)HeapAlloc(&device->dynamicHeap, CC_VIEWPORT_TABLE_SIZE * sizeof(CCViewport), CC_VIEWPORT_TABLE_ALIGN);
    CCViewport *ccViewport = device->ccViewportTable;
    for (uint i = 0; i < CC_VIEWPORT_TABLE_SIZE; ++i)
    {
        ccViewport->minDepth = -F32_MAX;
        ccViewport->maxDepth = F32_MAX;
        ++ccViewport;
    }

    device->sfClipViewportTable = (SFClipViewport *)HeapAlloc(&device->dynamicHeap, SF_CLIP_VIEWPORT_TABLE_SIZE * sizeof(SFClipViewport), SF_CLIP_VIEWPORT_TABLE_ALIGN);
    SFClipViewport *sfClipViewport = device->sfClipViewportTable;
    for (uint i = 0; i < SF_CLIP_VIEWPORT_TABLE_SIZE; ++i)
    {
        // Points are transformed by scale and then translate.
        sfClipViewport->scaleX = SCREEN_WIDTH * 0.5f;
        sfClipViewport->scaleY = SCREEN_HEIGHT * 0.5f;
        sfClipViewport->scaleZ = 1.0f;
        sfClipViewport->transX = SCREEN_WIDTH * 0.5f;
        sfClipViewport->transY = SCREEN_HEIGHT * 0.5f;
        sfClipViewport->transZ = 0.0f;
        sfClipViewport->pad0[0] = 0.0f;
        sfClipViewport->pad0[1] = 0.0f;

        // Guardband is in NDC space.
        sfClipViewport->guardbandMinX = -1.0f;
        sfClipViewport->guardbandMaxX = 1.0f;
        sfClipViewport->guardbandMinY = -1.0f;
        sfClipViewport->guardbandMaxY = 1.0f;
        ++sfClipViewport;
    }

    // Scissor State  - Dynamic State (ignore for now as it is disabled in the SF state)

    // Render Surface State
    device->surfaceState = (SurfaceState *)HeapAlloc(&device->surfaceHeap, sizeof(SurfaceState), BINDING_TABLE_ALIGN);
    SurfaceState * surfaceState = device->surfaceState;
    surfaceState->flags0 =
          SURFTYPE_2D << SURFACE_TYPE_SHIFT
        | FMT_B8G8R8A8_UNORM << SURFACE_FORMAT_SHIFT;
    surfaceState->baseAddr = device->surface.gfxAddr;
    surfaceState->width = SCREEN_WIDTH - 1;
    surfaceState->height = SCREEN_HEIGHT - 1;
    surfaceState->pitchDepth =
        (SCREEN_WIDTH * sizeof(u32) - 1) << SURFACE_PITCH_SHIFT;
    surfaceState->flags4 = 0;
}

// ------------------------------------------------------------------------------------------------
static void CreateShaders(GfxDevice *device)
{
    InitHeap(device, &device->instructionHeap, 4 * KB, 64);

    device->shaderObjs[VS_PASSTHROUGH_P] = 0;  // TODO
    device->shaderObjs[PS8_SOLID_FF8040FF] = 0;
    device->shaderObjs[PS16_SOLID_FF8040FF] = 0;
    device->shaderObjs[PS32_SOLID_FF8040FF] = 0;
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
static void CreateTestBatchBuffer(GfxDevice *device)
{
    GfxAlloc(&s_gfxDevice.memManager, &device->batchBuffer, 4 * KB, 4 * KB);
    u32 *cmd = (u32 *)device->batchBuffer.cpuAddr;

    // Switch to 3D pipeline
    *cmd++ = PIPELINE_SELECT(PIPELINE_3D);

    // Update base addresses - just use 0 for everything with no caching or upper bounds
    *cmd++ = STATE_BASE_ADDRESS;
    *cmd++ = 0 | BASE_ADDRESS_MODIFY;                                       // General State Base Address
    *cmd++ = device->surfaceHeap.storage.gfxAddr | BASE_ADDRESS_MODIFY;     // Surface State Base Address
    *cmd++ = device->dynamicHeap.storage.gfxAddr | BASE_ADDRESS_MODIFY;     // Dynamic State Base Address
    *cmd++ = 0 | BASE_ADDRESS_MODIFY;                                       // Indirect State Base Address
    *cmd++ = device->instructionHeap.storage.gfxAddr | BASE_ADDRESS_MODIFY; // Instruction Base Address
    *cmd++ = 0 | BASE_ADDRESS_MODIFY;                                       // General State Access Upper Bound
    *cmd++ = 0 | BASE_ADDRESS_MODIFY;                                       // Dynamic State Access Upper Bound
    *cmd++ = 0 | BASE_ADDRESS_MODIFY;                                       // Indirect State Access Upper Bound
    *cmd++ = 0 | BASE_ADDRESS_MODIFY;                                       // Instruction Access Upper Bound

    // Color Calc State
    *cmd++ = _3DSTATE_CC_STATE_POINTERS;
    *cmd++ = (u8*)device->colorCalcStateTable - device->dynamicHeap.storage.cpuAddr;

    // Blend State
    *cmd++ = _3DSTATE_BLEND_STATE_POINTERS;
    *cmd++ = (u8*)device->blendStateTable - device->dynamicHeap.storage.cpuAddr;

    // Depth/Stencil State
    *cmd++ = _3DSTATE_DEPTH_STENCIL_STATE_POINTERS;
    *cmd++ = (u8*)device->depthStencilStateTable - device->dynamicHeap.storage.cpuAddr;

    // Binding Tables
    u32* psBindingTable = device->bindingTable[SHADER_PS];
    psBindingTable[0] = (u8*)device->surfaceState - device->surfaceHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_VS;
    *cmd++ = (u8*)device->bindingTable[SHADER_VS] - device->surfaceHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_HS;
    *cmd++ = (u8*)device->bindingTable[SHADER_HS] - device->surfaceHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_DS;
    *cmd++ = (u8*)device->bindingTable[SHADER_DS] - device->surfaceHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_GS;
    *cmd++ = (u8*)device->bindingTable[SHADER_GS] - device->surfaceHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_BINDING_TABLE_POINTERS_PS;
    *cmd++ = (u8*)device->bindingTable[SHADER_PS] - device->surfaceHeap.storage.cpuAddr;

    // Sampler Tables
    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_VS;
    *cmd++ = (u8*)device->samplerTable[SHADER_VS] - device->dynamicHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_HS;
    *cmd++ = (u8*)device->samplerTable[SHADER_HS] - device->dynamicHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_DS;
    *cmd++ = (u8*)device->samplerTable[SHADER_DS] - device->dynamicHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_GS;
    *cmd++ = (u8*)device->samplerTable[SHADER_GS] - device->dynamicHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_SAMPLER_STATE_POINTERS_PS;
    *cmd++ = (u8*)device->samplerTable[SHADER_PS] - device->dynamicHeap.storage.cpuAddr;

    // Viewport State
    *cmd++ = _3DSTATE_VIEWPORT_STATE_POINTERS_CC;
    *cmd++ = (u8*)device->ccViewportTable - device->dynamicHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP;
    *cmd++ = (u8*)device->sfClipViewportTable - device->dynamicHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_SCISSOR_STATE_POINTERS;
    *cmd++ = 0;     // can be disabled in the 3DSTATE_SF

    // URB
    *cmd++ = _3DSTATE_URB_VS;
    *cmd++ =
          (0 << URB_START_ADDR_SHIFT)       // 0KB start
        | (0 << URB_ENTRY_ALLOC_SIZE_SHIFT) // 1 64B entry
        | (704 << URB_ENTRY_COUNT_SHIFT);   // maximum entry count for VS

    *cmd++ = _3DSTATE_URB_HS;
    *cmd++ =
          (0 << URB_START_ADDR_SHIFT)       // 0KB start
        | (0 << URB_ENTRY_ALLOC_SIZE_SHIFT) // 1 64B entry
        | (0 << URB_ENTRY_COUNT_SHIFT);     // no entries

    *cmd++ = _3DSTATE_URB_DS;
    *cmd++ =
          (0 << URB_START_ADDR_SHIFT)       // 0KB start
        | (0 << URB_ENTRY_ALLOC_SIZE_SHIFT) // 1 64B entry
        | (0 << URB_ENTRY_COUNT_SHIFT);     // no entries

    *cmd++ = _3DSTATE_URB_GS;
    *cmd++ =
          (0 << URB_START_ADDR_SHIFT)       // 0KB start
        | (0 << URB_ENTRY_ALLOC_SIZE_SHIFT) // 1 64B entry
        | (0 << URB_ENTRY_COUNT_SHIFT);     // no entries

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
    *cmd++ = device->triangleVB.gfxAddr;
    *cmd++ = device->triangleVB.gfxAddr + sizeof(float) * 9 - 1;
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

    // Vertex Fetch State
    *cmd++ = _3DSTATE_VF_STATISTICS;


    // Vertex Shader
    *cmd++ = _3DSTATE_VS;
    *cmd++ = (u8*)device->shaderObjs[VS_PASSTHROUGH_P] - device->instructionHeap.storage.cpuAddr;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ =
          (1 << VS_DISPATCH_GRF_SHIFT)
        | (1 << VS_URB_READ_LENGTH_SHIFT)
        | (0 << VS_URB_READ_OFFSET_SHIFT);
    *cmd++ =
          (1 << VS_MAX_THREAD_SHIFT)
        | VS_ENABLE;

    *cmd++ = _3DSTATE_CONSTANT_VS;
    *cmd++ = 0;
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

    *cmd++ = _3DSTATE_CONSTANT_HS;
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

    *cmd++ = _3DSTATE_CONSTANT_DS;
    *cmd++ = 0;
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

    *cmd++ = _3DSTATE_CONSTANT_GS;
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

    // Setup
    *cmd++ = _3DSTATE_CLIP;
    *cmd++ = (CULL_NONE << CLIP_CULL_SHIFT);
    *cmd++ = 0;
    *cmd++ = (1 << CLIP_MAX_VP_INDEX_SHIFT);

    // Rasterizer
    *cmd++ = _3DSTATE_DRAWING_RECTANGLE;
    *cmd++ = 0;
    *cmd++ =
          ((SCREEN_HEIGHT - 1) << DRAWING_RECT_Y_MAX_SHIFT)
        | ((SCREEN_WIDTH - 1) << DRAWING_RECT_X_MAX_SHIFT);
    *cmd++ = 0;

    *cmd++ = _3DSTATE_SF;
    *cmd++ =
          (DFMT_D32_FLOAT << SF_FORMAT_SHIFT)
        | SF_VIEW_TRANSFORM;
    *cmd++ = (CULL_NONE << SF_CULL_SHIFT);
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    *cmd++ = _3DSTATE_SBE;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    // Pixel Shader (Windower)
    *cmd++ = _3DSTATE_WM;
    *cmd++ = WM_THREAD_DISPATCH;
    *cmd++ = 0;

    *cmd++ = _3DSTATE_PS;
    *cmd++ = (u8*)device->shaderObjs[PS8_SOLID_FF8040FF] - device->instructionHeap.storage.cpuAddr;
    *cmd++ = (1 << PS_BINDING_COUNT_SHIFT);     // comment says this is for prefetching?
    *cmd++ = 0;
    *cmd++ =
          (15 << PS_MAX_THREAD_SHIFT)
        | PS_DISPATCH8;
    *cmd++ =
          (1 << PS_DISPATCH0_GRF_SHIFT)
        | (1 << PS_DISPATCH1_GRF_SHIFT)
        | (1 << PS_DISPATCH2_GRF_SHIFT);
    *cmd++ = (u8*)device->shaderObjs[PS16_SOLID_FF8040FF] - device->instructionHeap.storage.cpuAddr;   // TODO - order of kernels?
    *cmd++ = (u8*)device->shaderObjs[PS32_SOLID_FF8040FF] - device->instructionHeap.storage.cpuAddr;

    *cmd++ = _3DSTATE_CONSTANT_PS;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    *cmd++ = _3DSTATE_SAMPLE_MASK;
    *cmd++ = 1;

    *cmd++ = _3DSTATE_MULTISAMPLE;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    *cmd++ = _3DSTATE_DEPTH_BUFFER;
    *cmd++ =
          (SURFTYPE_NULL << DEPTH_BUF_SURFACE_TYPE_SHIFT)
        | (DFMT_D32_FLOAT << DEPTH_BUF_SURFACE_FMT_SHIFT);
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;
    *cmd++ = 0;

    *cmd++ = _3DSTATE_STENCIL_BUFFER;
    *cmd++ = 0;
    *cmd++ = 0;

    *cmd++ = _3DSTATE_HIER_DEPTH_BUFFER;
    *cmd++ = 0;
    *cmd++ = 0;

    *cmd++ = _3DSTATE_CLEAR_PARAMS;
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

    // Triangle Draw
    *cmd++ = _3DPRIMITIVE;
    *cmd++ = (_3DPRIM_TRILIST << PRIM_TOPOLOGY_SHIFT);
    *cmd++ = 3;
    *cmd++ = 0;
    *cmd++ = 1;
    *cmd++ = 0;
    *cmd++ = 0;

    // Debug
    *cmd++ = MI_STORE_DATA_INDEX;
    *cmd++ = 0; // offset
    *cmd++ = 0xabcd0123;
    *cmd++ = 0;

    // End Batch Buffer
    *cmd++ = MI_BATCH_BUFFER_END;

    RlogPrint("Batch Buffer Size = %08x\n", (u8 *)cmd - device->batchBuffer.cpuAddr);
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
    memset(s_gfxDevice.surface.cpuAddr, 0x77, SCREEN_WIDTH * SCREEN_HEIGHT * 4);

    // Allocate Cursor - 64KB aligned, +2 PTEs
    uint cursorMemSize = 64 * 64 * sizeof(u32) + 8 * KB;
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.cursor, cursorMemSize, 64 * KB);
    memcpy(s_gfxDevice.cursor.cpuAddr, cursor_image.pixel_data, 64 * 64 * sizeof(u32));

    // Allocate Render Ring
    GfxInitRing(&s_gfxDevice.renderRing, RING_RCS, &s_gfxDevice.memManager);

    // Allocate Render Context
    GfxAlloc(&s_gfxDevice.memManager, &s_gfxDevice.renderContext, 4 * KB, 4 * KB);

    // Allocate States
    CreateStates(&s_gfxDevice);
    CreateShaders(&s_gfxDevice);

    // Allocate Gfx Buffers
    CreateTriangle();

    // Allocate Batch Buffer
    CreateTestBatchBuffer(&s_gfxDevice);

    // Setup Primary Plane
    uint width = SCREEN_WIDTH;                       // TODO: mode support
    //uint height = SCREEN_HEIGHT;
    uint stride = (width * sizeof(u32) + 63) & ~63;   // 64-byte aligned

    GfxWrite32(&s_gfxDevice.pci, PRI_CTL_A, PRI_PLANE_ENABLE | PRI_PLANE_32BPP);
    GfxWrite32(&s_gfxDevice.pci, PRI_LINOFF_A, 0);
    GfxWrite32(&s_gfxDevice.pci, PRI_STRIDE_A, stride);
    GfxWrite32(&s_gfxDevice.pci, PRI_SURF_A, s_gfxDevice.surface.gfxAddr);

    // Setup Cursor Plane
    GfxWrite32(&s_gfxDevice.pci, CUR_CTL_A, CUR_MODE_ARGB | CUR_MODE_64_32BPP);
    GfxWrite32(&s_gfxDevice.pci, CUR_BASE_A, s_gfxDevice.cursor.gfxAddr);

    // Setup resolution
    GfxWrite32(&s_gfxDevice.pci, PIPE_SRCSZ_A, ((SCREEN_WIDTH - 1) << 16) + (SCREEN_HEIGHT - 1));

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
    *cmd++ = PIPE_CONTROL(5);
    *cmd++ = PIPE_CONTROL_SCOREBOARD_STALL
        | PIPE_CONTROL_CS_STALL;
    *cmd++ = 0; // address
    *cmd++ = 0; // immediate data (low)
    *cmd++ = 0; // immediate data (high)
    *cmd++ = MI_NOOP;
    GfxEndCmd(&s_gfxDevice.pci, ring, cmd);

    // PIPE_CONTROL for flush
    cmd = GfxBeginCmd(ring, 6);
    *cmd++ = PIPE_CONTROL(5);
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
    GfxPrintStatistics();

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
        GfxPrintStatistics();
    }
}
