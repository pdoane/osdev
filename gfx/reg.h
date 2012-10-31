// ------------------------------------------------------------------------------------------------
// gfx/reg.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

#define MB (1024 * 1024)   // MWDD FIX: Hacky - need to find a proper home

// ------------------------------------------------------------------------------------------------
// Vol 1. Part 2. MMIO, Media Registers, and Programming Environment
// ------------------------------------------------------------------------------------------------

typedef u64 GfxAddress;    // Address in Gfx Virtual space

// ------------------------------------------------------------------------------------------------
// 2.1.2.1 Instruction Parser Mode
#define GTT_PAGE_SHIFT 12
#define GTT_PAGE_SIZE (1 << GTT_PAGE_SHIFT)

typedef union GttEntry
{
    struct GttEntry_Bits
    {
        u32 valid            :  1;
        u32 l3CacheControl   :  1;
        u32 llcCacheControl  :  1;
        u32 gfxDataType      :  1;
        u32 physStartAddrExt :  8;
        u32 physPageAddr     : 20;
    } bits;
    u32 word;
} GttEntry;

// ------------------------------------------------------------------------------------------------
// Vol 1. Part 3. Memory Interface and Commands for the Render Engine
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 1.1.1.1 ARB_MODE – Arbiter Mode Control register
#define ARB_MODE                        0x04030 // R/W

typedef struct RegArbMode_Data
{
    u16 gttAccessesGdr     : 1;
    u16 colorCacheGdrEna   : 1;
    u16 depthCacheGdrEna   : 1;
    u16 textureCacheGdrEna : 1;
    u16 vmcGdrEna          : 1;
    u16 as4ts              : 1; // Address swizzling for Tiled-Surfaces
    u16 reserved0          : 2;
    u16 cdps               : 1; // Color/Depth Port Share Bit
    u16 gampd_gdr          : 1; // GAM PD GDR
    u16 blb_gdr            : 1;
    u16 stc_gdr            : 1;
    u16 hiz_gdr            : 1;
    u16 dc_gdr             : 1;
    u16 gam2bgttt          : 1; // GAM to Bypass GTT Translation
} RegArbMode_Data;

typedef union RegArbMode
{
    struct RegArbMode_Bits
    {
        RegArbMode_Data data;
        RegArbMode_Data mask;
    } bits;
    u32 dword;
} RegArbMode;

// ------------------------------------------------------------------------------------------------
// 1.1.9 Instruction Parser Mode

#define INSTPM                          0x020c0     // R/W

// ------------------------------------------------------------------------------------------------
// 1.1.10.13 Render/Video Semaphore Sync

#define RVSYNC                          0x02040     // R/W

// ------------------------------------------------------------------------------------------------
// 1.1.10.14 Render/Blitter Semaphore Sync

#define RBSYNC                          0x02044     // R/W

// ------------------------------------------------------------------------------------------------
// 1.1.11.1 Ring Buffer Tail

#define RCS_RING_BUFFER_TAIL            0x02030     // R/W
#define VCS_RING_BUFFER_TAIL            0x12030     // R/W
#define BCS_RING_BUFFER_TAIL            0x22030     // R/W

// ------------------------------------------------------------------------------------------------
// 1.1.11.2 Ring Buffer Head

#define RCS_RING_BUFFER_HEAD            0x02034     // R/W
#define VCS_RING_BUFFER_HEAD            0x12034     // R/W
#define BCS_RING_BUFFER_HEAD            0x22034     // R/W

// ------------------------------------------------------------------------------------------------
// 1.1.11.3 Ring Buffer Start

#define RCS_RING_BUFFER_START           0x02038     // R/W
#define VCS_RING_BUFFER_START           0x12038     // R/W
#define BCS_RING_BUFFER_START           0x22038     // R/W

// ------------------------------------------------------------------------------------------------
// 1.1.11.4 Ring Buffer Control

#define RCS_RING_BUFFER_CTL             0x0203c     // R/W
#define VCS_RING_BUFFER_CTL             0x1203c     // R/W
#define BCS_RING_BUFFER_CTL             0x2203c     // R/W

// ------------------------------------------------------------------------------------------------
// Vol 1. Part 4. Blitter Engine
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 1.8.1 COLOR_BLT (Fill)

#define COLOR_BLT                       ((2 << 29) | (0x40 << 22) | 3)

// ------------------------------------------------------------------------------------------------
// 1.10.1 BR00 - BLT Opcode and Control

#define WRITE_ALPHA                     (1 << 21)
#define WRITE_RGB                       (1 << 20)

// ------------------------------------------------------------------------------------------------
// 1.10.6 BR09 - Destination Address

// ------------------------------------------------------------------------------------------------
// 1.10.9 BR13 - BLT Raster OP, Control, and Destination Pitch

#define COLOR_DEPTH_8                   (0 << 24)
#define COLOR_DEPTH_16                  (1 << 24)
#define COLOR_DEPTH_32                  (3 << 24)

#define ROP_SHIFT                       16

// ------------------------------------------------------------------------------------------------
// 1.10.10 BR14 - Destination Width and Height

#define HEIGHT_SHIFT                    16

// ------------------------------------------------------------------------------------------------
// 1.10.12 BR16 - Pattern Expansion Background and Solid Pattern Color


// ------------------------------------------------------------------------------------------------
// 2.1.6.1 BCS Hardware Status Page Address

#define BCS_HWS_PGA                     0x04280

// ------------------------------------------------------------------------------------------------
// 2.1.8.1 Blitter/Render Semaphore Sync

#define BRSYNC                          0x22040     // R/W

// ------------------------------------------------------------------------------------------------
// 2.1.8.2 Blitter/Video Semaphore Sync

#define BVSYNC                          0x22044     // R/W


// ------------------------------------------------------------------------------------------------
// Vol 3. Part 1. VGA and Extended VGA Registers
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 1.2.1 Sequencer Index

#define SR_INDEX                        0x3c4
#define SR_DATA                         0x3c5

// ------------------------------------------------------------------------------------------------
// 1.2.3 Clocking Mode

#define SEQ_CLOCKING                    0x01
#define SCREEN_OFF                      (1 << 5)

// ------------------------------------------------------------------------------------------------
// Vol 3. Part 2. PCI Registers
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 1.25 MGGC0 - Mirror of GMCH Graphics Control Register
#define MGGC0                          0x50 // In PCI Config Space

typedef enum RegMGGCO_GMS
{
    RegMGGCO_GMS_32MB     = 0x05,
    RegMGGCO_GMS_48MB     = 0x06,
    RegMGGCO_GMS_64MB     = 0x07,
    RegMGGCO_GMS_128MB    = 0x08,
    RegMGGCO_GMS_256MB    = 0x09,
    RegMGGCO_GMS_96MB     = 0x0A,
    RegMGGCO_GMS_160MB    = 0x0B,
    RegMGGCO_GMS_224MB    = 0x0C,
    RegMGGCO_GMS_352MB    = 0x0D,
    RegMGGCO_GMS_0MB      = 0x00,
    RegMGGCO_GMS_32MB_1   = 0x01,
    RegMGGCO_GMS_64MB_1   = 0x02,
    RegMGGCO_GMS_96MB_1   = 0x03,
    RegMGGCO_GMS_128MB_1  = 0x04,
    RegMGGCO_GMS_448MB    = 0x0E,
    RegMGGCO_GMS_480MB    = 0x0F,
    RegMGGCO_GMS_512MB    = 0x10,
} RegMGGCO_GMS;

typedef enum RegMGGCO_GGMS
{
    RegMGGCO_GGMS_None = 0x0,
    RegMGGCO_GGMS_1MB  = 0x1,
    RegMGGCO_GGMS_2MB  = 0x2,
} RegMGGCO_GGMS;

typedef union RegMGGCO
{
    struct RegMGGCO_Bits
    {
        u16 lock               : 1;
        u16 igdVGADisable      : 1;
        u16 reserved0          : 1;
        u16 graphicsModeSelect : 5;  // RegMGGCO_GMS
        u16 gttMemSize         : 2;  // RegMGGCO_GGMS
        u16 reserved1          : 4;
        u16 vesatileAccModeEna : 1;
        u16 reserved2          : 1;
    } bits;
    u16 word;
} RegMGGCO;


// ------------------------------------------------------------------------------------------------
// 1.27 BDSM - Base Data of Stolen Memory
#define BDSM                          0x5C // In PCI Config Space

typedef union RegBDSM
{
    struct RegBDSM_Bits
    {
        u32 lock               : 1;
        u32 reserved0          : 19;
        u32 address            : 12;
    } bits;
    u32 dword;
} RegBDSM;

// ------------------------------------------------------------------------------------------------
// 1.45 ASLS - ASL Storage
// Software scratch register (BIOS sets the opregion address in here)
//
#define ASLS                          0xFC // In PCI Config Space


// ------------------------------------------------------------------------------------------------
// Vol 3. Part 3. North Display Engine Registers
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 3.1.1 VGA Control

#define VGA_CONTROL                     0x41000     // R/W

#define VGA_DISABLE                     (1 << 31)

// ------------------------------------------------------------------------------------------------
// 3.7.1 ARB_CTL-Display Arbitration Control 1
#define ARB_CTL                         0x45000     // R/W
typedef union RegArbCtl
{
    struct RegArbCtl_Bits
    {
        u32 hpDataRequestLimit      : 7;
        u32 reserved0               : 1;
        u32 hpPageBreakLimit        : 5;
        u32 tiledAddressSwizzling   : 2;
        u32 tlbRequestInFlightLimit : 3;
        u32 tlbRequestLimit         : 3;
        u32 lpWriteRequestLimit     : 2;
        u32 hpQueueWatermark        : 3;
        u32 reserved1               : 3;
    } bits;
    u32 dword;
} RegArbCtl;


// ------------------------------------------------------------------------------------------------
// 4.1.1 Horizontal Total

#define PIPE_HTOTAL_A                   0x60000     // R/W
#define PIPE_HTOTAL_B                   0x61000     // R/W
#define PIPE_HTOTAL_C                   0x62000     // R/W

// ------------------------------------------------------------------------------------------------
// 4.1.2 Horizontal Blank

#define PIPE_HBLANK_A                   0x60004     // R/W
#define PIPE_HBLANK_B                   0x61004     // R/W
#define PIPE_HBLANK_C                   0x62004     // R/W

// ------------------------------------------------------------------------------------------------
// 4.1.3 Horizontal Sync

#define PIPE_HSYNC_A                    0x60008     // R/W
#define PIPE_HSYNC_B                    0x61008     // R/W
#define PIPE_HSYNC_C                    0x62008     // R/W

// ------------------------------------------------------------------------------------------------
// 4.1.4 Vertical Total

#define PIPE_VTOTAL_A                   0x6000c     // R/W
#define PIPE_VTOTAL_B                   0x6100c     // R/W
#define PIPE_VTOTAL_C                   0x6200c     // R/W

// ------------------------------------------------------------------------------------------------
// 4.1.5 Vertical Blank

#define PIPE_VBLANK_A                   0x60010     // R/W
#define PIPE_VBLANK_B                   0x61010     // R/W
#define PIPE_VBLANK_C                   0x62010     // R/W

// ------------------------------------------------------------------------------------------------
// 4.1.6 Vertical Sync

#define PIPE_VSYNC_A                    0x60014     // R/W
#define PIPE_VSYNC_B                    0x61014     // R/W
#define PIPE_VSYNC_C                    0x62014     // R/W

// ------------------------------------------------------------------------------------------------
// 4.1.7 Source Image Size

#define PIPE_SRCSZ_A                    0x6001c     // R/W
#define PIPE_SRCSZ_B                    0x6101c     // R/W
#define PIPE_SRCSZ_C                    0x6201c     // R/W

// ------------------------------------------------------------------------------------------------
// 5.1.3 Pipe Configuration

#define PIPE_CONF_A                     0x70008     // R/W
#define PIPE_CONF_B                     0x71008     // R/W
#define PIPE_CONF_C                     0x72008     // R/W

// ------------------------------------------------------------------------------------------------
// 5.2.1 Cursor Control

#define CUR_CTL_A                       0x70080     // R/W
#define CUR_CTL_B                       0x71080     // R/W
#define CUR_CTL_C                       0x72080     // R/W

#define CUR_GAMMA_ENABLE                (1 << 26)   // Gamma Enable
#define CUR_MODE_ARGB                   (1 << 5)    // 32bpp ARGB
#define CUR_MODE_64_32BPP               (7 << 0)    // 64 x 64 32bpp

// ------------------------------------------------------------------------------------------------
// 5.2.2 Cursor Base

#define CUR_BASE_A                      0x70084     // R/W
#define CUR_BASE_B                      0x71084     // R/W
#define CUR_BASE_C                      0x72084     // R/W

// ------------------------------------------------------------------------------------------------
// 5.2.3 Cursor Position

#define CUR_POS_A                       0x70088     // R/W
#define CUR_POS_B                       0x71088     // R/W
#define CUR_POS_C                       0x72088     // R/W

// ------------------------------------------------------------------------------------------------
// 5.3.1 Primary Control

#define PRI_CTL_A                       0x70180     // R/W
#define PRI_CTL_B                       0x71180     // R/W
#define PRI_CTL_C                       0x72180     // R/W

#define PRI_PLANE_ENABLE                (1 << 31)
#define PRI_PLANE_32BPP                 (6 << 26)

// ------------------------------------------------------------------------------------------------
// 5.3.2 Primary Linear Offset

#define PRI_LINOFF_A                    0x70184     // R/W
#define PRI_LINOFF_B                    0x71184     // R/W
#define PRI_LINOFF_C                    0x72184     // R/W

// ------------------------------------------------------------------------------------------------
// 5.3.3 Primary Stride

#define PRI_STRIDE_A                    0x70188     // R/W
#define PRI_STRIDE_B                    0x71188     // R/W
#define PRI_STRIDE_C                    0x72188     // R/W

// ------------------------------------------------------------------------------------------------
// 5.3.4 Primary Surface Base Address

#define PRI_SURF_A                      0x7019c     // R/W
#define PRI_SURF_B                      0x7119c     // R/W
#define PRI_SURF_C                      0x7219c     // R/W

// ------------------------------------------------------------------------------------------------
// 5.3.5 Primary Tiled Offset

#define PRI_TILEOFF_A                   0x701a4     // R/W
#define PRI_TILEOFF_B                   0x711a4     // R/W
#define PRI_TILEOFF_C                   0x721a4     // R/W

// ------------------------------------------------------------------------------------------------
// Vol 3. Part 4. South Display Engine Registers
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 3.6.1 HDMI Port Control

#define HDMI_CTL_B                      0xe1140     // R/W
#define HDMI_CTL_C                      0xe1150     // R/W
#define HDMI_CTL_D                      0xe1160     // R/W

#define PORT_DETECTED                   (1 << 2)    // RO


// ------------------------------------------------------------------------------------------------
// Vol 3. Part 4. South Display Engine Registers
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 3.6.1 HDMI Port Control

#define HDMI_CTL_B                      0xe1140     // R/W
#define HDMI_CTL_C                      0xe1150     // R/W
#define HDMI_CTL_D                      0xe1160     // R/W

#define PORT_DETECTED                   (1 << 2)    // RO

// ------------------------------------------------------------------------------------------------
// IGD OpRegion Specification
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 2.2.1 OpRegion Header
typedef struct OpRegionHeader
{
    char sign[0x10];
    u32  size;
    u32  over;
    char sver[0x20];
    char vver[0x10];
    char gver[0x10];
    union OpRegionHeader_MBox
    {
        struct OpRegionHeader_MBox_Bits
        {
            u32 acpi  :  1;
            u32 swsci :  1;
            u32 asle  :  1;
            u32 rsvd  : 29;
        } bits;
        u32 dword;
    } mbox;
    u32  dmod;
    u8   rsv1[0xA0];
} OpRegionHeader;

// ------------------------------------------------------------------------------------------------
// 3.1 Mailbox #1: Public ACPI Methods Mailbox
#define OPREGION_MAILBOX1_OFFSET 0x0100

typedef struct OpRegionMailbox1ACPI
{
    u32 drdy;            // Driver Ready
    u32 csts;            // STATUS
    u32 cevt;            // Current Event
    u8  rsv2[0x14];
    u32 didl[8];         // Supported Display Devices ID List (_DOD)
    u32 cpdl[8];         // Current Attached (or Present) Display Devices List
    u32 cadl[8];         // Current Active Display DevicesLists (_DCS)
    u32 nadl[8];         // Next Active Devices List (_DGS use)
    u32 aslp;            // ASL Sleep Time Out
    u32 tidx;            // Toggle Table Index
    u32 chpd;            // Current Hotplug Enable Indicator
    u32 clid;            // Current Lid State Indicator
    u32 cdck;            // Current Docking State Indicator
    u32 sxsw;            // Request ASL to issue Display Switch notification on Sx State resume
    u32 evts;            // Events Supported by ASL
    u32 cnot;            // Current OS Notification
    u32 nrdy;            // Driver Status
    u8  rsv3[0x40];
} OpRegionMailbox1ACPI;

// ------------------------------------------------------------------------------------------------
// 2.2.1 OpRegion Header
#define OPREGION_MAILBOX2_OFFSET 0x0200

typedef struct OpRegionMailbox2SWSCI
{
    u32 scic;             // SWSCI Command/Status/Data
    u32 parm;             // Parameters
    u32 dslp;             // Driver Sleep Time Out
    u8  rsv4[0xF4];
} OpRegionMailbox2SWSCI;

// ------------------------------------------------------------------------------------------------
// 2.2.1 OpRegion Header
#define OPREGION_MAILBOX3_OFFSET 0x0300
typedef struct OpRegionMailbox3ASLE
{
    u32 ardy;             // Driver Readiness
    u32 aslc;             // ASLE Interrupt Command/Status
    u32 tche;             // Technology Enable Indicator
    u32 alsi;             // Current ALS Luminance Reading (in Lux)
    u32 bclp;             // Requested Backlight Brightness
    u32 pfit;             // Panel Fitting State or Request
    u32 cblv;             // Current Brightness Level
    u16 bclm[20];         // Backlight Brightness Levels Duty Cycle Mapping Table
    u32 cpfm;             // Current Panel Fitting Mode
    u32 epfm;             // Enabled Panel Fitting Modes
    struct PLUT
    {
        u8 lutHeader;     // MWDD FIX: Prob need to break down to bits
        struct PanelId
        {
            u16 manufacturingId;
            u16 productId;
            u32 serialNumbers;
            u8  weekOfManufacture;
            u8  yearOfManufacture;
        } panelId;
        u8 lutTable[7][9]; // MWDD FIX: Do I have rows/cols backwards?
    } plut;               // Panel LUT & Identifer
    u32 pfmb;             // PWM Frequency and Minimum Brightness
    u32 ccdv;             // Color Correction Default Values
    u8  rsv4[0xF4];
} OpRegionMailbox3ASLE;

// ------------------------------------------------------------------------------------------------
// 2.2.1 OpRegion Header
#define OPREGION_VBT_OFFSET 0x0500


// ------------------------------------------------------------------------------------------------
// Registers not in the Spec (Found in Linux Driver)
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// Force Wake
// Bring the card out of D6 state
#define ECOBUS                          0xA180
#define FORCE_WAKE_MT                   0xA188 
#define FORCE_WAKE                      0xA18C 
#define FORCE_WAKE_MT_ACK               0x130040 
#define FORCE_WAKE_ACK                  0x130090 

// ------------------------------------------------------------------------------------------------
// Fence registers.  Mentioned lots of times
// and the base address is in Vol2 Part3: MFX, but the defintion is not
#define FENCE_BASE                      0x100000
#define FENCE_COUNT                     16
typedef struct RegFence
{
    struct RegFence_Bits
    {
        u64 valid              : 1;
        u64 ytile              : 1;
        u64 rsvd               : 10;
        u64 startPage          : 20;
        u64 pitch              : 12;
        u64 endPage            : 20;  // inclusive
    } bits;
    u64 qword;
} RegFence;

// ------------------------------------------------------------------------------------------------
// Tile Ctrl - control register for cpu gtt access
#define TILE_CTL                         0x101000     // R/W
typedef struct RegTileCtl
{
    struct RegTileCtl_Bits
    {
        u32 swzctl              : 2;
        u32 tlbPrefetchDis      : 1;
        u32 backSnoopDis        : 1;
        u32 unknown             : 28;
    } bits;
    u32 dword;
} RegTileCtl;
