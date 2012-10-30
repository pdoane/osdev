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

    u8     *gfx_mem_base;
    u8     *gfx_mem_next;

    u8     *surface;
    u8     *cursor;
    u8     *render_cs;
    u8     *render_status;
} GfxDevice;

static GfxDevice s_gfxDevice;


// ------------------------------------------------------------------------------------------------
/*
static u32 gfx_addr(u8* phy_addr)
{
    return (u32)(phy_addr - s_gfxDevice.gfx_mem_base);
}*/

// ------------------------------------------------------------------------------------------------
/*
static void* gfx_alloc(uint size, uint align)
{
    // Align memory request
    u8* result = s_gfxDevice.gfx_mem_next;
    uintptr_t offset = (uintptr_t)result & (align - 1);
    if (offset)
    {
        result += align - offset;
    }

    s_gfxDevice.gfx_mem_next = result + size;
    return result;
}
*/

// ------------------------------------------------------------------------------------------------
static void gfx_print_port_state()
{
    rlog_print("HDMI_CTL_B: 0x%08X\n", gfx_read32(&s_gfxDevice.pci, HDMI_CTL_B));
    rlog_print("HDMI_CTL_C: 0x%08X\n", gfx_read32(&s_gfxDevice.pci, HDMI_CTL_C));
    rlog_print("HDMI_CTL_D: 0x%08X\n", gfx_read32(&s_gfxDevice.pci, HDMI_CTL_D));
}

// ------------------------------------------------------------------------------------------------
/*
static void gfx_print_pipe_state()
{
    //for (uint i = 0; i < 3; ++i)
    uint i = 0;
    {
        uint pipe = 0x1000 * i;
        rlog_print("PIPE %d\n", i);

        // Output Timing
        rlog_print("  CONF: %08x\n", gfx_read(PIPE_CONF_A + pipe));

        u32 htotal = gfx_read(PIPE_HTOTAL_A + pipe);
        uint hactive = (htotal & 0xffff) + 1;
        uint htotal_size = ((htotal >> 16) & 0xffff) + 1;
        rlog_print("  HTOTAL: %08x %d,%d\n", htotal, hactive, htotal_size);

        u32 hblank = gfx_read(PIPE_HBLANK_A + pipe);
        uint hblank_start = (hblank & 0xffff) + 1;
        uint hblank_end = ((hblank >> 16) & 0xffff) + 1;
        rlog_print("  HBLANK: %08x %d,%d\n", hblank, hblank_start, hblank_end);

        u32 hsync = gfx_read(PIPE_HSYNC_A + pipe);
        uint hsync_start = (hsync & 0xffff) + 1;
        uint hsync_end = ((hsync >> 16) & 0xffff) + 1;
        rlog_print("  HSYNC: %08x %d,%d\n", hsync, hsync_start, hsync_end);

        u32 vtotal = gfx_read(PIPE_VTOTAL_A + pipe);
        uint vactive = (vtotal & 0xffff) + 1;
        uint vtotal_size = ((vtotal >> 16) & 0xffff) + 1;
        rlog_print("  VTOTAL: %08x %d,%d\n", vtotal, vactive, vtotal_size);

        u32 vblank = gfx_read(PIPE_VBLANK_A + pipe);
        uint vblank_start = (vblank & 0xffff) + 1;
        uint vblank_end = ((vblank >> 16) & 0xffff) + 1;
        rlog_print("  VBLANK: %08x %d,%d\n", vblank, vblank_start, vblank_end);

        u32 vsync = gfx_read(PIPE_VSYNC_A + pipe);
        uint vsync_start = (vsync & 0xffff) + 1;
        uint vsync_end = ((vsync >> 16) & 0xffff) + 1;
        rlog_print("  VSYNC: %08x %d,%d\n", vsync, vsync_start, vsync_end);

        u32 srcsz = gfx_read(PIPE_SRCSZ_A + pipe);
        uint width = ((srcsz >> 16) & 0xfff) + 1;
        uint height = (srcsz & 0xffff) + 1;
        rlog_print("  SRCSZ: %08x %dx%d\n", srcsz, width, height);

        // Cursor Plane
        rlog_print("  CUR_CTL: %08x\n", gfx_read(CUR_CTL_A + pipe));
        rlog_print("  CUR_BASE: %08x\n", gfx_read(CUR_BASE_A + pipe));
        rlog_print("  CUR_POS: %08x\n", gfx_read(CUR_POS_A + pipe));

        // Primary Plane
        rlog_print("  PRI_CTL: %08x\n", gfx_read(PRI_CTL_A + pipe));
        rlog_print("  PRI_LINOFF: %08x\n", gfx_read(PRI_LINOFF_A + pipe));
        rlog_print("  PRI_STRIDE: %08x\n", gfx_read(PRI_STRIDE_A + pipe));
        rlog_print("  PRI_SURF: %08x\n", gfx_read(PRI_SURF_A + pipe));
    }
}
*/

// ------------------------------------------------------------------------------------------------
/*
static void gfx_print_ring_state()
{
    rlog_print("  BCS_HWS_PGA: 0x%08X\n", gfx_read(BCS_HWS_PGA));

    rlog_print("  BCS_RING_BUFFER_TAIL: 0x%08X\n", gfx_read(BCS_RING_BUFFER_TAIL));
    rlog_print("  BCS_RING_BUFFER_HEAD: 0x%08X\n", gfx_read(BCS_RING_BUFFER_HEAD));
    rlog_print("  BCS_RING_BUFFER_START: 0x%08X\n", gfx_read(BCS_RING_BUFFER_START));
    rlog_print("  BCS_RING_BUFFER_CTL: 0x%08X\n", gfx_read(BCS_RING_BUFFER_CTL));
}
*/

// ------------------------------------------------------------------------------------------------
void gfx_init(uint id, PCI_DeviceInfo* info)
{
    if (!(((info->class_code << 8) | info->subclass) == PCI_DISPLAY_VGA &&
        info->prog_intf == 0))
    {
        return;
    }

    if ((info->vendor_id != VENDOR_INTEL) ||
        (info->device_id != DEVICE_HD3000))
    {
        console_print("Graphics Controller not recognised!\n");
        return;
    }


    memset(&s_gfxDevice, 0, sizeof(s_gfxDevice));
    s_gfxDevice.pci.id = id;
}

/*
static void enter_force_wake()
{
    rlog_print("Try entering force wake...\n");

    while ((gfx_read(FORCE_WAKE_ACK) & 0x1) != 0)
        ;

    rlog_print("ACK cleared...\n");

    gfx_write(FORCE_WAKE, 1);
    gfx_read(0xa180);

    rlog_print("Wake written...\n");

    while ((gfx_read(FORCE_WAKE_ACK) & 0x1) == 0)
        ;

    rlog_print("Wake Ack...\n");
}

static void exit_force_wake()
{
    // If cacheable, need to do force the post
    gfx_write(FORCE_WAKE, 0);
}
*/

// ------------------------------------------------------------------------------------------------
bool ValidateChipset()
{
    // Check we are on Pather Point Chipset
    uint isaBridgeId = PCI_MAKE_ID(0, 0x1f, 0);  // Assume location of ISA Bridge
    u16 classCode     = ((u16)pci_in8(isaBridgeId, PCI_CONFIG_CLASS_CODE) << 8) | (u16)pci_in8(isaBridgeId, PCI_CONFIG_SUBCLASS);
    if (classCode != PCI_BRIDGE_ISA)
    {
        console_print("Isa Bridge not found at expected location! (Found: 0x%X, Expected: 0x%X)\n", classCode, PCI_BRIDGE_ISA);
        return false;
    }

    u16 deviceId = pci_in16(isaBridgeId, PCI_CONFIG_DEVICE_ID) & 0xFF00;
    if (deviceId != DEVICE_PANTHERPOINT)
    {
        console_print("Chipset is not expect panther point (needed for display handling)! (Found: 0x%X, Expected: 0x%X)\n", deviceId, DEVICE_PANTHERPOINT);
        return false;
    }

    return true;
}


// ------------------------------------------------------------------------------------------------
void gfx_start()
{
    if (!s_gfxDevice.pci.id)
    {
        console_print("Graphics not supported\n");
        return;
    }
    
    if (!ValidateChipset())
    {
        return;
    }

    gfx_init_pci(&s_gfxDevice.pci);
    gfx_init_gtt(&s_gfxDevice.gtt, &s_gfxDevice.pci);
    gfx_init_mem_manager(&s_gfxDevice.memManager, &s_gfxDevice.gtt, &s_gfxDevice.pci);
    gfx_init_display(&s_gfxDevice.display);

    // Assume Dimms are same size, so bit 6 swizzling is on.

    // MWDD FIX: Enable MSI

    // Disable the VGA Plane
    gfx_disable_vga(&s_gfxDevice.pci);

    // Log initial port state
    gfx_print_port_state();

/*
    enter_force_wake();
    {
        rlog_print("Setting Ring...\n");

        // Setup Blitter Ring Buffer
        gfx_write(BCS_RING_BUFFER_TAIL, 0);     // Number of quad words
        gfx_write(BCS_RING_BUFFER_HEAD, 0);
        gfx_write(BCS_RING_BUFFER_START, 8 * MB);
        gfx_write(BCS_RING_BUFFER_CTL,
              (0 << 12)         // # of pages - 1
            | 1                 // Ring Buffer Enable
            );
        rlog_print("...done\n");

    }
    exit_force_wake();
    gfx_print_ring_state();
*/



    /*

    uint gfx_mem_size = 512 * 1024 * 1024;       // TODO: how to know size of GTT?
    uint gfx_mem_align = 256 * 1024;             // TODO: Max alignment needed for primary surface
    s_gfxDevice.gfx_mem_base = vm_alloc_align(gfx_mem_size, gfx_mem_align);
    s_gfxDevice.gfx_mem_next = s_gfxDevice.gfx_mem_base;

    // Map memory uncached
    vm_map_pages(s_gfxDevice.gfx_mem_base, gfx_mem_size, PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLE);

    // Allocate Surface - 256KB aligned, +512 PTEs
    uint surface_mem_size = 16 * 1024 * 1024;         // TODO: compute appropriate surface size
    s_gfxDevice.surface = gfx_alloc(surface_mem_size, 256 * 1024);
    memset(s_gfxDevice.surface, 0x77, 720 * 400 * 4);

    // Allocate Cursor - 64KB aligned, +2 PTEs
    uint cursor_mem_size = 64 * 64 * sizeof(u32) + 8 * 1024;
    s_gfxDevice.cursor = gfx_alloc(cursor_mem_size, 64 * 1024);
    memcpy(s_gfxDevice.cursor, cursor_image.pixel_data, 64 * 64 * sizeof(u32));

    // Allocate Render Engine Command Stream - 4KB aligned
    uint rcs_mem_size = 4 * 1024;
    s_gfxDevice.render_cs = gfx_alloc(rcs_mem_size, 4 * 1024);

    // Allocate RCS Hardware Status Page - 4KB aligned
    s_gfxDevice.render_status = gfx_alloc(4 * 1024, 4 * 1024);
    memset(s_gfxDevice.render_status, 0, 4 * 1024);



    rlog_print("VGA Plane disabled\n");

    // Setup Virtual Memory
    u8* phys_page = s_gfxDevice.gfx_mem_base;
    for (uint i = 0; i < 512 * 1024; ++i)
    {
        uintptr_t addr = (uintptr_t)phys_page;

        // Mark as Uncached and Valid
        s_gfxDevice.gtt_addr[i] = addr | ((addr >> 28) & 0xff0) | (1 << 1) | (1 << 0);

        phys_page += 4096;
    }

    // Setup Primary Plane
    uint width = 720;                       // TODO: mode support
    uint height = 400;
    uint stride = (width * sizeof(u32) + 63) & ~63;   // 64-byte aligned

    gfx_write(PRI_CTL_A, PRI_PLANE_ENABLE | PRI_PLANE_32BPP);
    gfx_write(PRI_LINOFF_A, 0);
    gfx_write(PRI_STRIDE_A, stride);
    gfx_write(PRI_SURF_A, gfx_addr(s_gfxDevice.surface));

    // Setup Cursor Plane
    gfx_write(CUR_CTL_A, CUR_MODE_ARGB | CUR_MODE_64_32BPP);
    gfx_write(CUR_BASE_A, gfx_addr(s_gfxDevice.cursor));

    // Pipe State
    gfx_print_pipe_state();

    
    // Set up H/W Status Page


    
    // Initalise ring buffer


    
    // First command has to be a flush

    // BCS Data
    uint rop = 0xf0;                        // P
    u32 blt_addr = gfx_addr(s_gfxDevice.surface);
    uint blt_height = 200;
    uint blt_width = width * sizeof(u32);

    volatile u32 *pCmd = (volatile u32 *)s_gfxDevice.blitter_cs;
    *pCmd++ = COLOR_BLT | WRITE_ALPHA | WRITE_RGB;
    *pCmd++ = COLOR_DEPTH_32 | (rop << ROP_SHIFT) | stride;
    *pCmd++ = (blt_height << HEIGHT_SHIFT) | blt_width;
    *pCmd++ = blt_addr;
    *pCmd++ = 0xffffffff;
    *pCmd++ = 0;

    // Blitter Virtual Memory Control
    gfx_write(BCS_HWS_PGA, gfx_addr(s_gfxDevice.blitter_status));

    // Setup Blitter Ring Buffer
    gfx_write(BCS_RING_BUFFER_TAIL, 3);     // Number of quad words
    gfx_write(BCS_RING_BUFFER_HEAD, 0);
    gfx_write(BCS_RING_BUFFER_START, gfx_addr(s_gfxDevice.blitter_cs));
    gfx_write(BCS_RING_BUFFER_CTL,
          (0 << 12)         // # of pages - 1
        | 1                 // Ring Buffer Enable
        );

    gfx_print_ring_state();
*/
    s_gfxDevice.active = true;
}

// ------------------------------------------------------------------------------------------------
void gfx_poll()
{
    if (!s_gfxDevice.active)
    {
        return;
    }

    static u32 last_cursor_pos = 0;

    // Update cursor position
    u32 cursor_pos = (g_mouse_y << 16) | g_mouse_x;
    if (cursor_pos != last_cursor_pos)
    {
        last_cursor_pos = cursor_pos;
        gfx_write32(&s_gfxDevice.pci, CUR_POS_A, cursor_pos);
    }
}
