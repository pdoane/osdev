// ------------------------------------------------------------------------------------------------
// gfx/reg.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Common macros

#define MI_INSTR(opcode, flags) \
    (((opcode) << 23) | (flags))
#define GFX_INSTR(subType, opcode, subOpcode, flags) \
    ((0x3 << 29) | (subType << 27) | ((opcode) << 24) | ((subOpcode) << 16) | (flags))

#define MASKED_ENABLE(x)                (((x) << 16) | (x))
#define MASKED_DISABLE(x)               ((x) << 16)

// ------------------------------------------------------------------------------------------------
// Vol 1. Part 1. Graphics Core
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 3.5.1 STATE_BASE_ADDRESS

#define STATE_BASE_ADDRESS              GFX_INSTR(0, 0x1, 0x1, 8)

#define BASE_ADDRESS_MODIFY             (1 << 0)

// DWORD 1 - General State Base Address
// DWORD 2 - Surface State Base Address
// DWORD 3 - Dynamic State Base Address
// DWORD 4 - Indirect State Base Address
// DWORD 5 - Instruction Base Address
// DWORD 6 - General State Access Upper Bound
// DWORD 7 - Dynamic State Access Upper Bound
// DWORD 8 - Indirect State Access Upper Bound
// DWORD 9 - Instruction Access Upper Bound

// ------------------------------------------------------------------------------------------------
// 3.8.1 PIPELINE_SELECT

#define PIPELINE_SELECT(x)              GFX_INSTR(1, 0x1, 0x4, x)

#define PIPELINE_3D                     0x0
#define PIPELINE_MEDIA                  0x1
#define PIPELINE_GPGPU                  0x2

// ------------------------------------------------------------------------------------------------
// Vol 1. Part 2. MMIO, Media Registers, and Programming Environment
// ------------------------------------------------------------------------------------------------

typedef u64 GfxAddress;    // Address in Gfx Virtual space

// ------------------------------------------------------------------------------------------------
// 2.1.2.1 GTT Page Table Entries

#define GTT_PAGE_SHIFT                  12
#define GTT_PAGE_SIZE                   (1 << GTT_PAGE_SHIFT)

#define GTT_ENTRY_VALID                 (1 << 0)
#define GTT_ENTRY_L3_CACHE_CONTROL      (1 << 1)
#define GTT_ENTRY_LLC_CACHE_CONTROL     (1 << 2)
#define GTT_ENTRY_GFX_DATA_TYPE         (1 << 3)
#define GTT_ENTRY_ADDR(x)               ((x) | ((x >> 28) & 0xff0))

// ------------------------------------------------------------------------------------------------
// 3. GFX MMIO - MCHBAR Aperture

#define GFX_MCHBAR                      0x140000

// ------------------------------------------------------------------------------------------------
// Vol 1. Part 3. Memory Interface and Commands for the Render Engine
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 1.1.1.1 ARB_MODE – Arbiter Mode Control register

#define ARB_MODE                        0x04030     // R/W

#define ARB_MODE_GGTAGDR                (1 << 0)    // GTT Accesses GDR
#define ARB_MODE_CCGDREN                (1 << 1)    // Color Cache GDR Enable Bit
#define ARB_MODE_DCGDREN                (1 << 2)    // Depth Cache GDR Enable Bit
#define ARB_MODE_TCGDREN                (1 << 3)    // Texture Cache GDR Enable Bit
#define ARB_MODE_VMC_GDR_EN             (1 << 4)    // VMC GDR Enable
#define ARB_MODE_AS4TS                  (1 << 5)    // Address Swizzling for Tiled Surfaces
#define ARB_MODE_CDPS                   (1 << 8)    // Color/Depth Port Share Bit
#define ARB_MODE_GAMPD_GDR              (1 << 9)    // GAM PD GDR
#define ARB_MODE_BLB_GDR                (1 << 10)   // BLB GDR
#define ARB_MODE_STC_GDR                (1 << 11)   // STC GDR
#define ARB_MODE_HIZ_GDR                (1 << 12)   // HIZ GDR
#define ARB_MODE_DC_GDR                 (1 << 13)   // DC GDR
#define ARB_MODE_GAM2BGTTT              (1 << 14)   // GAM to Bypass GTT Translation

// ------------------------------------------------------------------------------------------------
// 1.1.5.1 Hardware Status Page Address

#define RCS_HWS_PGA                     0x04080     // R/W

// ------------------------------------------------------------------------------------------------
// 1.1.9 Instruction Parser Mode

#define INSTPM                          0x020c0     // R/W

// ------------------------------------------------------------------------------------------------
// 1.1.10.2 Render Mode Register for Software Interface

#define MI_MODE                         0x0209c     // R/W

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
// 1.2.5 MI_BATCH_BUFFER_END

#define MI_BATCH_BUFFER_END             MI_INSTR(0x0a, 0)

// ------------------------------------------------------------------------------------------------
// 1.2.7 MI_BATCH_BUFFER_START

#define MI_BATCH_BUFFER_START           MI_INSTR(0x31, 0)

// DWORD1 = batch buffer start address

// ------------------------------------------------------------------------------------------------
// 1.2.12 MI_NOOP

#define MI_NOOP                         MI_INSTR(0x00, 0)

// ------------------------------------------------------------------------------------------------
// 1.2.16 MI_SET_CONTEXT

#define MI_SET_CONTEXT                  MI_INSTR(0x18, 0)

// DWORD 1 = logical context address (4KB aligned)
#define MI_GTT_ADDR                     (1 << 8)
#define MI_EXT_STATE_SAVE               (1 << 3)
#define MI_EXT_STATE_RESTORE            (1 << 2)
#define MI_FORCE_RESTORE                (1 << 1)
#define MI_RESTORE_INHIBIT              (1 << 0)

// ------------------------------------------------------------------------------------------------
// 1.2.18 MI_STORE_DATA_INDEX

#define MI_STORE_DATA_INDEX             MI_INSTR(0x21, 1)

// DWORD 1 = offset
// DWORD 2 = data 0
// DWORD 3 = data 1

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

#define BCS_HWS_PGA                     0x04280     // R/W

// ------------------------------------------------------------------------------------------------
// 2.1.8.1 Blitter/Render Semaphore Sync

#define BRSYNC                          0x22040     // R/W

// ------------------------------------------------------------------------------------------------
// 2.1.8.2 Blitter/Video Semaphore Sync

#define BVSYNC                          0x22044     // R/W

// ------------------------------------------------------------------------------------------------
// Vol 1. Part 5. Video Codec Engine Command Streamer
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 1.1.2.2.1 VCS Hardware Status Page Address

#define VCS_HWS_PGA                     0x04180     // R/W

// ------------------------------------------------------------------------------------------------
// Vol 2. Part 1. 3D Pipeline
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 1.4.2 3DSTATE_CC_STATE_POINTERS

#define _3DSTATE_CC_STATE_POINTERS      GFX_INSTR(0x3, 0x0, 0xe, 0)

// DWORD 1 - Pointer to ColorCalcState (relative to Dynamic State Base Address)

// ------------------------------------------------------------------------------------------------
// 1.4.3 3DSTATE_BLEND_STATE_POINTERS

#define _3DSTATE_BLEND_STATE_POINTERS   GFX_INSTR(0x3, 0x0, 0x24, 0)

// DWORD 1 - Pointer to BlendState (relative to Dynamic State Base Address)

// ------------------------------------------------------------------------------------------------
// 1.4.4 3DSTATE_DEPTH_STENCIL_STATE_POINTERS

#define _3DSTATE_DEPTH_STENCIL_STATE_POINTERS   GFX_INSTR(0x3, 0x0, 0x24, 0)

// DWORD 1 - Pointer to DepthStencilState (relative to Dynamic State Base Address)

// ------------------------------------------------------------------------------------------------
// 1.10.4 PIPE_CONTROL Command

#define PIPE_CONTROL                                GFX_INSTR(0x3, 0x2, 0x0, 3)

// DWORD 1 - flags
#define PIPE_CONTROL_DEPTH_CACHE_FLUSH              (1 << 0)
#define PIPE_CONTROL_SCOREBOARD_STALL               (1 << 1)
#define PIPE_CONTROL_STATE_CACHE_INVALIDATE         (1 << 2)
#define PIPE_CONTROL_CONST_CACHE_INVALIDATE         (1 << 3)
#define PIPE_CONTROL_VF_CACHE_INVALIDATE            (1 << 4)
#define PIPE_CONTROL_DC_FLUSH                       (1 << 5)
#define PIPE_CONTROL_PIPE_CONTROL_FLUSH             (1 << 7)
#define PIPE_CONTROL_NOTIFY                         (1 << 8)
#define PIPE_CONTROL_INDIRECT_STATE_DISABLE         (1 << 9)
#define PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE       (1 << 10)
#define PIPE_CONTROL_INSTR_CACHE_INVALIDATE         (1 << 11)
#define PIPE_CONTROL_RENDER_TARGET_CACHE_FLUSH      (1 << 12)
#define PIPE_CONTROL_DEPTH_STALL                    (1 << 13)
#define PIPE_CONTROL_WRITE_IMM                      (1 << 14)
#define PIPE_CONTROL_WRITE_PS_DEPTH_COUNT           (2 << 14)
#define PIPE_CONTROL_WRITE_TIMESTAMP                (3 << 14)
#define PIPE_CONTROL_GENERIC_MEDIA_STATE_CLEAR      (1 << 16)
#define PIPE_CONTROL_TLB_INVALIDATE                 (1 << 18)
#define PIPE_CONTROL_GLOBAL_SNAPSHOT_COUNT_RESET    (1 << 19)
#define PIPE_CONTROL_CS_STALL                       (1 << 20)
#define PIPE_CONTROL_STORE_DATA_INDEX               (1 << 21)
#define PIPE_CONTROL_MMIO_WRITE_IMM                 (1 << 23)
#define PIPE_CONTROL_USE_GGTT                       (1 << 24)

// DWORD 2 - address
// DWORD 3 - immediate data (low)
// DWORD 4 - immediate data (high)

// ------------------------------------------------------------------------------------------------
// 12.2 Pixel Pipeline State

// Compare Func (used by BlendState and DepthStencilState)
#define COMPARE_FUNC_ALWAYS             0x0
#define COMPARE_FUNC_NEVER              0x1
#define COMPARE_FUNC_LT                 0x2
#define COMPARE_FUNC_EQ                 0x3
#define COMPARE_FUNC_LE                 0x4
#define COMPARE_FUNC_GT                 0x5
#define COMPARE_FUNC_NE                 0x6
#define COMPARE_FUNC_GE                 0x7

// ------------------------------------------------------------------------------------------------
// 12.2.1 COLOR_CALC_STATE

// Flags
#define CC_STENCIL_REF_MASK             0xff
#define CC_FRONT_FACE_STENCIL_REF_SHIFT 24
#define CC_BACK_FACE_STENCIL_REF_SHIFT  16
#define CC_ROUND_DISABLE                (1 << 15)
#define CC_ALPHA_REF_FLOAT              (1 << 0)

typedef struct ColorCalcState
{
    u32 flags;
    union ColorCalcState_AlphaRef
    {
        float floatVal;
        u32 intVal;         // ref value stored in high byte
    } alphaRef;
    float constR;
    float constG;
    float constB;
    float constA;
} ColorCalcState;

// ------------------------------------------------------------------------------------------------
// 12.2.2 DEPTH_STENCIL_STATE

// Stencil Op
#define STENCIL_OP_KEEP                     0x0
#define STENCIL_OP_ZERO                     0x1
#define STENCIL_OP_REPLACE                  0x2
#define STENCIL_OP_INC_SAT                  0x3
#define STENCIL_OP_DEC_SAT                  0x4
#define STENCIL_OP_INC                      0x5
#define STENCIL_OP_DEC                      0x6
#define STENCIL_OP_INV                      0x7

// Stencil Flags
#define STENCIL_ENABLE                      (1 << 31)
#define STENCIL_FUNC_SHIFT                  28          // COMPARE_FUNC
#define STENCIL_FAIL_OP_SHIFT               25
#define STENCIL_DEPTH_FAIL_OP_SHIFT         22
#define STENCIL_PASS_OP_SHIFT               19
#define STENCIL_BUFFER_WRITE                (1 << 18)
#define STENCIL_DOUBLE_SIDED                (1 << 15)
#define STENCIL_BACK_FUNC_SHIFT             12
#define STENCIL_BACK_FAIL_OP_SHIFT          9
#define STENCIL_BACK_DEPTH_FAIL_OP_SHIFT    6
#define STENCIL_BACK_PASS_OP_SHIFT          3

// Stencil Masks
#define STENCIL_TEST_MASK_SHIFT             24
#define STENCIL_WRITE_MASK_SHIFT            16
#define STENCIL_BACK_TEST_MASK_SHIFT        8
#define STENCIL_BACK_WRITE_MASK_SHIFT       0

// Depth Flags
#define DEPTH_TEST_ENABLE                   (1 << 31)
#define DEPTH_FUNC_SHIFT                    27          // COMPARE_FUNC
#define DEPTH_WRITE_ENABLE                  (1 << 26)

typedef struct DepthStencilState
{
    u32 stencilFlags;
    u32 stencilMasks;
    u32 depthFlags;
} DepthStencilState;

// ------------------------------------------------------------------------------------------------
// 12.2.3 BLEND_STATE

// Blend Function
#define BLEND_FUNC_ADD                  0
#define BLEND_FUNC_SUB                  1
#define BLEND_FUNC_REV_SUB              2
#define BLEND_FUNC_MIN                  3
#define BLEND_FUNC_MAX                  4
#define BLEND_FUNC_MASK                 0x7

// Blend Factor
#define BLEND_FACTOR_ONE                0x01
#define BLEND_FACTOR_SRC_COLOR          0x02
#define BLEND_FACTOR_SRC_ALPHA          0x03
#define BLEND_FACTOR_DST_ALPHA          0x04
#define BLEND_FACTOR_DST_COLOR          0x05
#define BLEND_FACTOR_SRC_ALPHA_SAT      0x06
#define BLEND_FACTOR_CONST_COLOR        0x07
#define BLEND_FACTOR_CONST_ALPHA        0x08
#define BLEND_FACTOR_SRC1_COLOR         0x09
#define BLEND_FACTOR_SRC1_ALPHA         0x0a
#define BLEND_FACTOR_ZERO               0x11
#define BLEND_FACTOR_INV_SRC_COLOR      0x12
#define BLEND_FACTOR_INV_SRC_ALPHA      0x13
#define BLEND_FACTOR_INV_DST_ALPHA      0x14
#define BLEND_FACTOR_INV_DST_COLOR      0x15
#define BLEND_FACTOR_INV_CONST_COLOR    0x17
#define BLEND_FACTOR_INV_CONST_ALPHA    0x18
#define BLEND_FACTOR_INV_SRC1_COLOR     0x19
#define BLEND_FACTOR_INV_SRC1_ALPHA     0x1a
#define BLEND_FACTOR_MASK               0x1f

// Logic Op
#define LOGIC_OP_CLEAR                  0x0
#define LOGIC_OP_NOR                    0x1
#define LOGIC_OP_AND_INV                0x2
#define LOGIC_OP_COPY_INV               0x3
#define LOGIC_OP_AND_REV                0x4
#define LOGIC_OP_INV                    0x5
#define LOGIC_OP_XOR                    0x6
#define LOGIC_OP_NAND                   0x7
#define LOGIC_OP_AND                    0x8
#define LOGIC_OP_EQUIV                  0x9
#define LOGIC_OP_NOOP                   0xa
#define LOGIC_OP_OR_INV                 0xb
#define LOGIC_OP_COPY                   0xc
#define LOGIC_OP_OR_REV                 0xd
#define LOGIC_OP_OR                     0xe
#define LOGIC_OP_SET                    0xf
#define LOGIC_OP_MASK                   0xf

// Color Clamp
#define COLOR_CLAMP_UNORM               0x0
#define COLOR_CLAMP_SNORM               0x1
#define COLOR_CLAMP_RTFORMAT            0x2
#define COLOR_CLAMP_MASK                0x3

// DWORD 0
#define BLEND_COLOR                     (1 << 31)   // Only BLEND_COLOR or BLEND_LOGIC can be enabled
#define BLEND_INDEPENDENT_ALPHA         (1 << 30)
#define BLEND_FUNC_ALPHA_SHIFT          26
#define BLEND_SRC_ALPHA_SHIFT           20
#define BLEND_DST_ALPHA_SHIFT           15
#define BLEND_FUNC_COLOR_SHIFT          11
#define BLEND_SRC_COLOR_SHIFT           5
#define BLEND_DST_COLOR_SHIFT           0

// DWORD 1
#define BLEND_ALPHA_TO_COVERAGE         (1 << 31)
#define BLEND_ALPHA_TO_ONE              (1 << 30)   // Errata - must be disabled
#define BLEND_ALPHA_TO_COVERAGE_DITHER  (1 << 29)
#define BLEND_DISABLE_ALPHA             (1 << 27)   // Errata - must be set to 0 if not present in the render target
#define BLEND_DISABLE_RED               (1 << 26)   // Errata - must be set to 0 if not present in the render target
#define BLEND_DISABLE_GREEN             (1 << 25)   // Errata - must be set to 0 if not present in the render target
#define BLEND_DISABLE_BLUE              (1 << 24)   // Errata - must be set to 0 if not present in the render target
#define BLEND_LOGIC                     (1 << 22)   // Only BLEND_COLOR or BLEND_LOGIC can be enabled
#define BLEND_LOGIC_FUNC_SHIFT          18
#define BLEND_ALPHA_TEST                (1 << 16)
#define BLEND_ALPHA_FUNC                (1 << 13)   // COMPARE_FUNC
#define BLEND_COLOR_DITHER              (1 << 12)
#define BLEND_DITHER_X_SHIFT            10
#define BLEND_DITHER_Y_SHIFT            8
#define BLEND_COLOR_CLAMP_RANGE_SHIFT   2
#define BLEND_PRE_COLOR_CLAMP           (1 << 1)
#define BLEND_POST_COLOR_CLAMP          (1 << 0)

typedef struct BlendState
{
    u32 flags0;
    u32 flags1;
} BlendState;

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

#define MGGC0                           0x50        // In PCI Config Space

#define GGC_LOCK                        (1 << 0)
#define GGC_IVD                         (1 << 1)    // IGD VGA Disable
#define GGC_GMS_SHIFT                   3           // Graphics Mode Select
#define GGC_GMS_MASK                    0x1f
#define GGC_GGMS_SHIFT                  8           // GTT Graphics Memory Size
#define GGC_GGMS_MASK                   0x3
#define GGC_VAMEN                       (1 << 14)   // Versatile Acceleration Mode Enable

// This matches the IVB graphics documentation, not the IVB CPU documentation
#define GMS_32MB                        0x05
#define GMS_48MB                        0x06
#define GMS_64MB                        0x07
#define GMS_128MB                       0x08
#define GMS_256MB                       0x09
#define GMS_96MB                        0x0A
#define GMS_160MB                       0x0B
#define GMS_224MB                       0x0C
#define GMS_352MB                       0x0D
#define GMS_0MB                         0x00
#define GMS_32MB_1                      0x01
#define GMS_64MB_1                      0x02
#define GMS_96MB_1                      0x03
#define GMS_128MB_1                     0x04
#define GMS_448MB                       0x0E
#define GMS_480MB                       0x0F
#define GMS_512MB                       0x10

#define GGMS_None                       0x00
#define GGMS_1MB                        0x01
#define GGMS_2MB                        0x02

// ------------------------------------------------------------------------------------------------
// 1.27 BDSM - Base Data of Stolen Memory

#define BDSM                            0x5C // In PCI Config Space

#define BDSM_LOCK                       (1 << 0)
#define BDSM_ADDR_MASK                  (0xfff << 20)

// ------------------------------------------------------------------------------------------------
// 1.45 ASLS - ASL Storage
// Software scratch register (BIOS sets the opregion address in here)

#define ASLS                            0xFC // In PCI Config Space

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

#define ARB_CTL_HP_DATA_REQUEST_LIMIT_MASK          0x7f
#define ARB_CTL_HP_PAGE_BREAK_LIMIT_SHIFT           8
#define ARB_CTL_HP_PAGE_BREAK_LIMIT_MASK            0x1f
#define ARB_CTL_TILED_ADDRESS_SWIZZLING             (1 << 13)
#define ARB_CTL_TLB_REQUEST_IN_FLIGHT_LIMIT_SHIFT   16
#define ARB_CTL_TLB_REQUEST_IN_FLIGHT_LIMIT_MASK    0x7
#define ARB_CTL_TLB_REQUEST_LIMIT_SHIFT             20
#define ARB_CTL_TLB_REQUEST_LIMIT_MASK              0x7
#define ARB_CTL_LP_WRITE_REQUEST_LIMIT_SHIFT        24
#define ARB_CTL_LP_WRITE_REQUEST_LIMIT_MASK         0x3
#define ARB_CTL_HP_QUEUE_WATERMARK_SHIFT            26
#define ARB_CTL_HP_QUEUE_WATERMARK_MASK             0x7

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
// Desktop 3rd Generation Intel® Core™ Processor Family and Desktop Intel® Pentium® Processor Family
// Datasheet - Volume 2
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 2.16.2-3 Address Decode Channel Registers

#define MAD_DIMM_CH0			        0x5004
#define MAD_DIMM_CH1			        0x5008

#define MAD_DIMM_A_SIZE_SHIFT           0
#define MAD_DIMM_A_SIZE_MASK            0xff
#define MAD_DIMM_B_SIZE_SHIFT           8
#define MAD_DIMM_B_SIZE_MASK            0xff
#define MAD_DIMM_AB_SIZE_MASK           0xffff
#define MAD_DIMM_A_SELECT               (1 << 16)
#define MAD_DIMM_A_DUAL_RANK            (1 << 17)
#define MAD_DIMM_B_DUAL_RANK            (1 << 18)
#define MAD_DIMM_A_X16                  (1 << 19)
#define MAD_DIMM_B_X16                  (1 << 20)
#define MAD_DIMM_RANK_INTERLEAVE        (1 << 21)
#define MAD_DIMM_ENH_INTERLEAVE         (1 << 22)
#define MAD_DIMM_ECC_MODE               (3 << 24) 

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
// and the base address is in Vol2 Part3: MFX, but the definition is not

#define FENCE_BASE                      0x100000
#define FENCE_COUNT                     16

/*
typedef union RegFence
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
*/

// ------------------------------------------------------------------------------------------------
// Tile Ctrl - control register for cpu gtt access

#define TILE_CTL                         0x101000     // R/W

#define TILE_CTL_SWIZZLE                (1 << 0)
#define TILE_CTL_TLB_PREFETCH_DISABLE   (1 << 2)
#define TILE_CTL_BACKSNOOP_DISABLE      (1 << 3)
