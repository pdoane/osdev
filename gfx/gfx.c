// ------------------------------------------------------------------------------------------------
// gfx/gfx.c
// ------------------------------------------------------------------------------------------------

#include "gfx/gfx.h"
#include "gfx/gfxpci.h"
#include "gfx/gtt.h"
#include "gfx/gfxmem.h"
#include "gfx/gfxdisplay.h"
#include "console/console.h"
#include "cpu/io.h"
#include "input/input.h"
#include "mem/vm.h"
#include "net/rlog.h"
#include "pci/registry.h"
#include "stdlib/string.h"
#include "time/pit.h"

#include "cursor.c"

#define DEVICE_HD3000       0x0162
#define DEVICE_PANTHERPOINT 0x1e00

// ------------------------------------------------------------------------------------------------
typedef struct GfxDevice
{
    bool    active;

    GfxPCI        pci;
    GfxGTT        gtt;
    GfxMemManager memManager;
    GfxDisplay    display;

    u8     *gfxMemBase;
    u8     *gfxMemNext;

    u8     *surface;
    u8     *cursor;
    u8     *renderCS;
    u8     *renderStatus;
} GfxDevice;

static GfxDevice s_gfxDevice;

// ------------------------------------------------------------------------------------------------
static void EnterForceWake()
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

    GfxWrite32(&s_gfxDevice.pci, FORCE_WAKE_MT, (1 << 16) | 1);
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
static void ExitForceWake()
{
    GfxWrite32(&s_gfxDevice.pci, FORCE_WAKE_MT, (1 << 16) | 0);
    GfxRead32(&s_gfxDevice.pci, ECOBUS);
}

// ------------------------------------------------------------------------------------------------
static u32 GfxAddr(u8 *phyAddr)
{
    return (u32)(phyAddr - s_gfxDevice.gfxMemBase);
}

// ------------------------------------------------------------------------------------------------
static void *GfxAlloc(uint size, uint align)
{
    // Align memory request
    u8 *result = s_gfxDevice.gfxMemNext;
    uintptr_t offset = (uintptr_t)result & (align - 1);
    if (offset)
    {
        result += align - offset;
    }

    s_gfxDevice.gfxMemNext = result + size;
    return result;
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
static void GfxPrintRingState()
{
    EnterForceWake();
    {
        RlogPrint("  RCS_HWS_PGA: 0x%08X\n", GfxRead32(&s_gfxDevice.pci, RCS_HWS_PGA));

        RlogPrint("  RCS_RING_BUFFER_TAIL: 0x%08X\n", GfxRead32(&s_gfxDevice.pci, RCS_RING_BUFFER_TAIL));
        RlogPrint("  RCS_RING_BUFFER_HEAD: 0x%08X\n", GfxRead32(&s_gfxDevice.pci, RCS_RING_BUFFER_HEAD));
        RlogPrint("  RCS_RING_BUFFER_START: 0x%08X\n", GfxRead32(&s_gfxDevice.pci, RCS_RING_BUFFER_START));
        RlogPrint("  RCS_RING_BUFFER_CTL: 0x%08X\n", GfxRead32(&s_gfxDevice.pci, RCS_RING_BUFFER_CTL));

        RlogPrint("  %08x\n", *(u32 *)s_gfxDevice.renderStatus);
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

    s_gfxDevice.gfxMemBase = s_gfxDevice.pci.apertureBar;
    s_gfxDevice.gfxMemNext = s_gfxDevice.gfxMemBase + 4 * GTT_PAGE_SIZE;

    // Allocate Surface - 256KB aligned, +512 PTEs
    uint surfaceMemSize = 16 * MB;         // TODO: compute appropriate surface size
    s_gfxDevice.surface = GfxAlloc(surfaceMemSize, 256 * KB);
    memset(s_gfxDevice.surface, 0x77, 720 * 400 * 4);

    // Allocate Cursor - 64KB aligned, +2 PTEs
    uint cursorMemSize = 64 * 64 * sizeof(u32) + 8 * KB;
    s_gfxDevice.cursor = GfxAlloc(cursorMemSize, 64 * KB);
    memcpy(s_gfxDevice.cursor, cursor_image.pixel_data, 64 * 64 * sizeof(u32));

    // Setup Primary Plane
    uint width = 720;                       // TODO: mode support
    //uint height = 400;
    uint stride = (width * sizeof(u32) + 63) & ~63;   // 64-byte aligned

    GfxWrite32(&s_gfxDevice.pci, PRI_CTL_A, PRI_PLANE_ENABLE | PRI_PLANE_32BPP);
    GfxWrite32(&s_gfxDevice.pci, PRI_LINOFF_A, 0);
    GfxWrite32(&s_gfxDevice.pci, PRI_STRIDE_A, stride);
    GfxWrite32(&s_gfxDevice.pci, PRI_SURF_A, GfxAddr(s_gfxDevice.surface));

    // Setup Cursor Plane
    GfxWrite32(&s_gfxDevice.pci, CUR_CTL_A, CUR_MODE_ARGB | CUR_MODE_64_32BPP);
    GfxWrite32(&s_gfxDevice.pci, CUR_BASE_A, GfxAddr(s_gfxDevice.cursor));

    // Allocate Render Engine Command Stream - 4KB aligned
    uint rcsMemSize = 4 * KB;
    s_gfxDevice.renderCS = GfxAlloc(rcsMemSize, 4 * KB);

    // Allocate RCS Hardware Status Page - 4KB aligned
    s_gfxDevice.renderStatus = GfxAlloc(4 * KB, 4 * KB);
    memset(s_gfxDevice.renderStatus, 0, 4 * KB);

    // Log initial port state
    GfxPrintPortState();

    // MWDD FIX: Enable MSI

    EnterForceWake();
    {
        RlogPrint("Setting Ring...\n");

        volatile u32 *pCmd = (volatile u32 *)s_gfxDevice.renderCS;
        *pCmd++ = (0x21 << 23) | (1);
        *pCmd++ = 0;
        *pCmd++ = 0x12345678;
        *pCmd++ = 0;
        u32 tail = (u8*)pCmd - s_gfxDevice.renderCS;

        GfxWrite32(&s_gfxDevice.pci, RCS_HWS_PGA, GfxAddr(s_gfxDevice.renderStatus));

        // Setup Render Ring Buffer
        GfxWrite32(&s_gfxDevice.pci, RCS_RING_BUFFER_TAIL, 0);
        GfxWrite32(&s_gfxDevice.pci, RCS_RING_BUFFER_HEAD, 0);
        GfxWrite32(&s_gfxDevice.pci, RCS_RING_BUFFER_START, GfxAddr(s_gfxDevice.renderCS));
        GfxWrite32(&s_gfxDevice.pci, RCS_RING_BUFFER_CTL,
              (0 << 12)         // # of pages - 1
            | 1                 // Ring Buffer Enable
            );
        RlogPrint("...done\n");

        // Update Tail
        GfxWrite32(&s_gfxDevice.pci, RCS_RING_BUFFER_TAIL, tail);
        RlogPrint("...tail updated\n");

    }
    ExitForceWake();
    GfxPrintRingState();

    /*

    // First command has to be a flush

    // RCS Data
    uint rop = 0xf0;                        // P
    u32 bltAddr = GfxAddr(s_gfxDevice.surface);
    uint bltHeight = 200;
    uint bltWidth = width * sizeof(u32);

    volatile u32 *pCmd = (volatile u32 *)s_gfxDevice.blitterCS;
    *pCmd++ = COLOR_BLT | WRITE_ALPHA | WRITE_RGB;
    *pCmd++ = COLOR_DEPTH_32 | (rop << ROP_SHIFT) | stride;
    *pCmd++ = (bltHeight << HEIGHT_SHIFT) | bltWidth;
    *pCmd++ = bltAddr;
    *pCmd++ = 0xffffffff;
    *pCmd++ = 0;

    // Blitter Virtual Memory Control
    GfxWrite32(BCS_HWS_PGA, GfxAddr(s_gfxDevice.blitterStatus));

    // Setup Blitter Ring Buffer
    GfxWrite32(BCS_RING_BUFFER_TAIL, 3);     // Number of quad words
    GfxWrite32(BCS_RING_BUFFER_HEAD, 0);
    GfxWrite32(BCS_RING_BUFFER_START, GfxAddr(s_gfxDevice.blitterCS));
    GfxWrite32(BCS_RING_BUFFER_CTL,
          (0 << 12)         // # of pages - 1
        | 1                 // Ring Buffer Enable
        );

    GfxPrintRingState();
*/
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
        GfxPrintRingState();
    }
}
