// ------------------------------------------------------------------------------------------------
// gfx/reg.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Shader Types

#define SHADER_VS                       0
#define SHADER_HS                       1
#define SHADER_DS                       2
#define SHADER_GS                       3
#define SHADER_PS                       4
#define SHADER_COUNT                    5

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

#define _3DSTATE_DEPTH_STENCIL_STATE_POINTERS       GFX_INSTR(0x3, 0x0, 0x24, 0)

// DWORD 1 - Pointer to DepthStencilState (relative to Dynamic State Base Address)

// ------------------------------------------------------------------------------------------------
// 1.4.5 3DSTATE_BINDING_TABLE_POINTERS

#define BINDING_TABLE_SIZE                          256

#define _3DSTATE_BINDING_TABLE_POINTERS_VS          GFX_INSTR(0x3, 0x0, 0x26, 0)
#define _3DSTATE_BINDING_TABLE_POINTERS_HS          GFX_INSTR(0x3, 0x0, 0x27, 0)
#define _3DSTATE_BINDING_TABLE_POINTERS_DS          GFX_INSTR(0x3, 0x0, 0x28, 0)
#define _3DSTATE_BINDING_TABLE_POINTERS_GS          GFX_INSTR(0x3, 0x0, 0x29, 0)
#define _3DSTATE_BINDING_TABLE_POINTERS_PS          GFX_INSTR(0x3, 0x0, 0x2a, 0)

// DWORD 1 - Pointer to BindingTableState (relative to Surface State Base Address)

// ------------------------------------------------------------------------------------------------
// 1.5 3DSTATE_SAMPLER_STATE_POINTERS

#define SAMPLER_TABLE_SIZE                          16

#define _3DSTATE_SAMPLER_STATE_POINTERS_VS          GFX_INSTR(0x3, 0x0, 0x2b, 0)
#define _3DSTATE_SAMPLER_STATE_POINTERS_HS          GFX_INSTR(0x3, 0x0, 0x2c, 0)
#define _3DSTATE_SAMPLER_STATE_POINTERS_DS          GFX_INSTR(0x3, 0x0, 0x2d, 0)
#define _3DSTATE_SAMPLER_STATE_POINTERS_GS          GFX_INSTR(0x3, 0x0, 0x2e, 0)
#define _3DSTATE_SAMPLER_STATE_POINTERS_PS          GFX_INSTR(0x3, 0x0, 0x2f, 0)

// DWORD 1 - Pointer to SamplerState table (relative to Dynamic State Base Address)

// ------------------------------------------------------------------------------------------------
// 1.6.1 3DSTATE_VIEWPORT_STATE_POINTERS_CC

#define VIEWPORT_TABLE_SIZE                         16

#define _3DSTATE_VIEWPORT_STATE_POINTERS_CC         GFX_INSTR(0x3, 0x0, 0x23, 0)

// DWORD 1 - Pointer to CCViewport table (relative to Dynamic State Base Address)

// ------------------------------------------------------------------------------------------------
// 1.6.2 3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP

#define _3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP    GFX_INSTR(0x3, 0x0, 0x21, 0)

// DWORD 1 - Pointer to SFClipViewport table (relative to Dynamic State Base Address)

// ------------------------------------------------------------------------------------------------
// 1.6.3 3DSTATE_SCISSOR_STATE_POINTERS

#define _3DSTATE_SCISSOR_STATE_POINTERS             GFX_INSTR(0x3, 0x0, 0x0f, 0)

// DWORD 1 - Pointer to ScissorRect table (relative to Dynamic State Base Address)

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
// 2.2.1 3DSTATE_INDEX_BUFFER

#define _3DSTATE_INDEX_BUFFER           GFX_INSTR(0x3, 0x0, 0x0a, 1)

// INDEX_FMT
#define INDEX_FMT_BYTE                  0x0
#define INDEX_FMT_WORD                  0x1
#define INDEX_FMT_DWORD                 0x2
#define INDEX_FMT_MASK                  0x3

// DWORD 0
#define IB_OBJ_CONTROL_STATE_SHIFT      12          // MEMORY_OBJECT_CONTROL_STATE
#define IB_CUT_INDEX                    (1 << 10)
#define IB_FORMAT_SHIFT                 8           // INDEX_FMT

// DWORD 1 - Buffer Starting Address (must be in linear memory)
// DWORD 2 - Buffer Ending Address

// ------------------------------------------------------------------------------------------------
// 2.3.1 3DSTATE_VERTEX_BUFFERS

#define _3DSTATE_VERTEX_BUFFERS(n)      GFX_INSTR(0x3, 0x0, 0x08, 4*(n) - 1)

// DWORD 1..n - VERTEX_BUFFER_STATE

// ------------------------------------------------------------------------------------------------
// 2.3.2 VERTEX_BUFFER_STATE

// DWORD 0
#define VB_INDEX_SHIFT                  26
#define VB_INDEX_MASK                   0x3f
#define VB_INSTANCE_DATA                (1 << 20)
#define VB_OBJ_CONTROL_STATE_SHIFT      16          // MEMORY_OBJECT_CONTROL_STATE
#define VB_ADDRESS_MODIFY               (1 << 14)
#define VB_NULL                         (1 << 13)
#define VB_FETCH_INVALIDATE             (1 << 12)
#define VB_PITCH_SHIFT                  0
#define VB_PITCH_MASK                   0xfff

// DWORD 1 - Start Address
// DWORD 2 - End Address (inclusive)
// DWORD 3 - Instance Step Rate

// ------------------------------------------------------------------------------------------------
// 2.4.1 3DSTATE_VERTEX_ELEMENTS

#define _3DSTATE_VERTEX_ELEMENTS(n)     GFX_INSTR(0x3, 0x0, 0x09, 2*(n) - 1)

// DWORD 1..n - VERTEX_ELEMENT_STATE

// ------------------------------------------------------------------------------------------------
// 2.4.2 VERTEX_ELEMENT_STATE

// VFCOMP_CONTROL
#define VFCOMP_NOSTORE                  0x0
#define VFCOMP_STORE_SRC                0x1
#define VFCOMP_STORE_0                  0x2
#define VFCOMP_STORE_1_FP               0x3
#define VFCOMP_STORE_1_INT              0x4
#define VFCOMP_STORE_VID                0x5
#define VFCOMP_STORE_IID                0x6
#define VFCOMP_MASK                     0x7

// DWORD 0
#define VE_INDEX_SHIFT                  26
#define VE_INDEX_MASK                   0x3f
#define VE_VALID                        (1 << 25)
#define VE_FORMAT_SHIFT                 16          // SURFACE_FORMAT
#define VE_EDGE_FLAG                    (1 << 15)
#define VE_OFFSET_SHIFT                 0
#define VE_OFFSET_MASK                  0x7ff

// DWORD 1
#define VE_COMP0_SHIFT                  28          // VFCOMP_CONTROL
#define VE_COMP1_SHIFT                  24          // VFCOMP_CONTROL
#define VE_COMP2_SHIFT                  20          // VFCOMP_CONTROL
#define VE_COMP3_SHIFT                  16          // VFCOMP_CONTROL

// ------------------------------------------------------------------------------------------------
// 2.5.1 3DPRIMITIVE

#define _3DPRIMITIVE                    GFX_INSTR(0x3, 0x3, 0x00, 5)

// 3DPRIM_TOPOLOGY (not defined in documentation, cut index table in 2.2.1 seems to correspond to ordering)
#define _3DPRIM_POINTLIST               0x01
#define _3DPRIM_LINELIST                0x02
#define _3DPRIM_LINESTRIP               0x03
#define _3DPRIM_TRILIST                 0x04
#define _3DPRIM_TRISTRIP                0x05
#define _3DPRIM_TRIFAN                  0x06
#define _3DPRIM_QUADLIST                0x07
#define _3DPRIM_QUADSTRIP               0x08
#define _3DPRIM_LINELIST_ADJ            0x09
#define _3DPRIM_LINESTRIP_ADJ           0x0a
#define _3DPRIM_TRILIST_ADJ             0x0b
#define _3DPRIM_TRISTRIP_ADJ            0x0c
#define _3DPRIM_TRISTRIP_REVERSE        0x0d
#define _3DPRIM_POLYGON                 0x0e
#define _3DPRIM_RECTLIST                0x0f
#define _3DPRIM_LINELOOP                0x10
#define _3DPRIM_POINTLIST_BF            0x11
#define _3DPRIM_LINESTRIP_CONT          0x12
#define _3DPRIM_LINESTRIP_BF            0x13
#define _3DPRIM_LINESTRIP_CONT_BF       0x14
#define _3DPRIM_TRIFAN_NOSTIPPLE        0x15
#define _3DPRIM_PATCHLIST_n             // ???
#define _3DPRIM_MASK                    0x3f

// DWORD 0
#define PRIM_INDIRECT_PARAMETER         (1 << 10)
#define PRIM_PREDICATE                  (1 << 8)

// DWORD 1
#define PRIM_END_OFFSET                 (1 << 9)
#define PRIM_RANDOM                     (1 << 8)
#define PRIM_TOPOLOGY_SHIFT             0           // 3DPRIM_TOPOLOGY

// DWORD 2 - Vertex Count Per Instance
// DWORD 3 - Start Vertex Location
// DWORD 4 - Instance Count
// DWORD 5 - Start Instance Location
// DWORD 6 - Base Vertex Location

// ------------------------------------------------------------------------------------------------
// 3/4 Vertex/Hull Shader Stages

// SAMPLER_USAGE
#define SAMPLER_USAGE_NONE              0x0
#define SAMPLER_USAGE_1_4               0x1
#define SAMPLER_USAGE_5_8               0x2
#define SAMPLER_USAGE_9_12              0x3
#define SAMPLER_USAGE_13_16             0x4
#define SAMPLER_USAGE_MASK              0x7

// 3DSTATE_CONSTANT(Body)
typedef struct ConstantBufferBody
{
    u16 bufLen[4];
    u32 buffers[4];
} ConstantBufferBody;

// CONST_ALLOC
#define CONST_ALLOC_OFFSET_SHIFT        16
#define CONST_ALLOC_OFFSET_MASK         0xf
#define CONST_ALLOC_SIZE_SHIFT          0
#define CONST_ALLOC_SIZE_MASK           03f

// ------------------------------------------------------------------------------------------------
// 3DSTATE_CONSTANT_*S

#define _3DSTATE_CONSTANT_VS            GFX_INSTR(0x3, 0x0, 0x15, 5)
#define _3DSTATE_CONSTANT_HS            GFX_INSTR(0x3, 0x0, 0x19, 5)
#define _3DSTATE_CONSTANT_DS            GFX_INSTR(0x3, 0x0, 0x1a, 5)
#define _3DSTATE_CONSTANT_GS            GFX_INSTR(0x3, 0x0, 0x16, 5)

// DWORD 1..6 - ConstantBufferBody

// ------------------------------------------------------------------------------------------------
// 3.2.1.4 3DSTATE_PUSH_CONSTANT_ALLOC_*S

#define _3DSTATE_PUSH_CONSTANT_ALLOC_VS GFX_INSTR(0x3, 0x1, 0x12, 0)
#define _3DSTATE_PUSH_CONSTANT_ALLOC_HS GFX_INSTR(0x3, 0x1, 0x13, 0)
#define _3DSTATE_PUSH_CONSTANT_ALLOC_DS GFX_INSTR(0x3, 0x1, 0x14, 0)
#define _3DSTATE_PUSH_CONSTANT_ALLOC_GS GFX_INSTR(0x3, 0x1, 0x15, 0)

// DWORD 1 - CONST_ALLOC

// ------------------------------------------------------------------------------------------------
// 3.2.1.2 3DSTATE_VS

#define _3DSTATE_VS                     GFX_INSTR(0x03, 0x0, 0x10, 4)

// DWORD 1 - Kernel Start Pointer (relative to Instruction Base Address)

// DWORD 2
#define VS_SINGLE_VERTEX_DISPATCH       (1 << 31)
#define VS_VECTOR_MASK                  (1 << 30)
#define VS_SAMPLER_COUNT_SHIFT          27          // SAMPLER_USAGE
#define VS_BINDING_COUNT_SHIFT          18
#define VS_BINDING_COUNT_MASK           0xff
#define VS_FLOATING_POINT_ALT           (1 << 16)
#define VS_ILLEGAL_OPCODE_EX            (1 << 13)
#define VS_SOFTWARE_EX                  (1 << 7)

// DWORD 3
#define VS_SCRATCH_SPACE_BASE_SHIFT     (1 << 10)
#define VS_SCRATCH_SPACE_BASE_MASK      0x3fffff
#define VS_PER_THREAD_SPACE_SHIFT       0           // Power of 2 bytes + 1KB
#define VS_PER_THREAD_SPACE_MASK        0xf

// DWORD 4
#define VS_DISPATCH_GRF_SHIFT           20
#define VS_DISPATCH_GRF_MASK            0x1f
#define VS_URB_READ_LENGTH_SHIFT        11
#define VS_URB_READ_LENGTH_MASK         0x3f
#define VS_URB_READ_OFFSET_SHIFT        4
#define VS_URB_READ_OFFSET_MASK         0x3f

// DWORD 5
#define VS_MAX_THREAD_SHIFT             25
#define VS_MAX_THREAD_MASK              0x7f
#define VS_STATISTICS                   (1 << 10)
#define VS_CACHE_DISABLE                (1 << 1)
#define VS_ENABLE                       (1 << 0)

// ------------------------------------------------------------------------------------------------
// 4.4 3DSTATE_HS

#define _3DSTATE_HS                     GFX_INSTR(0x03, 0x0, 0x1b, 5)

// DWORD 1
#define HS_SAMPLER_COUNT_SHIFT          27          // SAMPLER_USAGE
#define HS_BINDING_COUNT_SHIFT          18
#define HS_BINDING_COUNT_MASK           0xff
#define HS_FLOATING_POINT_ALT           (1 << 16)
#define HS_FLOATING_POINT_ALT           (1 << 16)
#define HS_ILLEGAL_OPCODE_EX            (1 << 13)
#define HS_SOFTWARE_EX                  (1 << 7)
#define HS_MAX_THREAD_SHIFT             0
#define HS_MAX_THREAD_MASK              0x7f

// DWORD 2
#define HS_ENABLE                       (1 << 31)
#define HS_STATISTICS                   (1 << 29)
#define HS_INSTANCE_COUNT_SHIFT         0
#define HS_INSTANCE_COUNT_MASK          0xf

// DWORD 3 - Kernel Start Pointer (relative to Instruction Base Address)

// DWORD 4
#define HS_SCRATCH_SPACE_BASE_SHIFT     (1 << 10)
#define HS_SCRATCH_SPACE_BASE_MASK      0x3fffff
#define HS_PER_THREAD_SPACE_SHIFT       0           // Power of 2 bytes + 1KB
#define HS_PER_THREAD_SPACE_MASK        0xf

// DWORD 5
#define HS_SINGLE_PROGRAM_FLOW          (1 << 27)
#define HS_VECTOR_MASK                  (1 << 26)
#define HS_INCLUDE_VERTEX_HANDLES       (1 << 24)
#define HS_DISPATCH_GRF_SHIFT           19
#define HS_DISPATCH_GRF_MASK            0x1f
#define HS_URB_READ_LENGTH_SHIFT        11
#define HS_URB_READ_LENGTH_MASK         0x3f
#define HS_URB_READ_OFFSET_SHIFT        4
#define HS_URB_READ_OFFSET_MASK         0x3f

// DWORD 6 - Semaphore Handle

// ------------------------------------------------------------------------------------------------
// 5.1 3DSTATE_TE

#define _3DSTATE_TE                     GFX_INSTR(0x3, 0x0, 0x1c, 2)

// TE_PARTITIONING
#define TE_PARTITIONING_INT             0x0
#define TE_PARTITIONING_ODD_FRAC        0x1
#define TE_PARTITIONING_EVEN_FRAC       0x2
#define TE_PARTITIONING_MASK            0x3

// TE_OUTPUT
#define TE_OUTPUT_POINT                 0x0
#define TE_OUTPUT_LINE                  0x1
#define TE_OUTPUT_TRI_CW                0x2
#define TE_OUTPUT_TRI_CCW               0x3

// TE_DOMAIN
#define TE_DOMAIN_QUAD                  0x0
#define TE_DOMAIN_TRI                   0x1
#define TE_DOMAIN_ISOLINE               0x2
#define TE_DOMAIN_MASK                  0x3

// DWORD 1
#define TE_PARTITIONING_SHIFT           12          // TE_PARTITIONING
#define TE_OUTPUT_SHIFT                 8           // TE_OUTPUT
#define TE_DOMAIN_SHIFT                 4
#define TE_SW_TESS                      (1 << 1)
#define TE_ENABLE                       (1 << 0)

// DWORD 2 - Max TessFactor Odd (float)
// DWORD 3 - Max TessFactor Not Odd (float)

// ------------------------------------------------------------------------------------------------
// 6.1 3DSTATE_DS

#define _3DSTATE_DS                     GFX_INSTR(0x3, 0x0, 0x1d, 4)

// DWORD 1 - Kernel Start Pointer (relative teo Instruction Base Address)

// DWORD 2
#define DS_SINGLE_POINT_DISPATCH        (1 << 31)
#define DS_VECTOR_MASK                  (1 << 30)
#define DS_SAMPLER_COUNT_SHIFT          27          // SAMPLER_USAGE
#define DS_BINDING_COUNT_SHIFT          18
#define DS_BINDING_COUNT_MASK           0xff
#define DS_FLOATING_POINT_ALT           (1 << 16)
#define DS_ILLEGAL_OPCODE_EX            (1 << 13)
#define DS_SOFTWARE_EX                  (1 << 7)

// DWORD 3
#define DS_SCRATCH_SPACE_BASE_SHIFT     (1 << 10)
#define DS_SCRATCH_SPACE_BASE_MASK      0x3fffff
#define DS_PER_THREAD_SPACE_SHIFT       0           // Power of 2 bytes + 1KB
#define DS_PER_THREAD_SPACE_MASK        0xf

// DWORD 4
#define DS_DISPATCH_GRF_SHIFT           20
#define DS_DISPATCH_GRF_MASK            0x1f
#define DS_URB_READ_LENGTH_SHIFT        11
#define DS_URB_READ_LENGTH_MASK         0x3f
#define DS_URB_READ_OFFSET_SHIFT        4
#define DS_URB_READ_OFFSET_MASK         0x3f

// DWORD 5
#define DS_MAX_THREAD_SHIFT             25
#define DS_MAX_THREAD_MASK              0x7f
#define DS_STATISTICS                   (1 << 10)
#define DS_COMPUTE_W                    (1 << 2)
#define DS_CACHE_DISABLE                (1 << 1)
#define DS_ENABLE                       (1 << 0)

// ------------------------------------------------------------------------------------------------
// 7.2.1.1 3DSTATE_GS

#define _3DSTATE_GS                     GFX_INSTR(0x3, 0x0, 0x11, 5)

// DWORD 1 - Kernel Start Pointer (relative teo Instruction Base Address)

// DWORD 2
#define GS_SINGLE_PROGRAM_FLOW          (1 << 31)
#define GS_VECTOR_MASK                  (1 << 30)
#define GS_SAMPLER_COUNT_SHIFT          27          // SAMPLER_USAGE
#define GS_BINDING_COUNT_SHIFT          18
#define GS_BINDING_COUNT_MASK           0xff
#define GS_THREAD_HIGH_PRIORITY         (1 << 17)
#define GS_FLOATING_POINT_ALT           (1 << 16)
#define GS_ILLEGAL_OPCODE_EX            (1 << 13)
#define GS_MASK_STACK_EX                (1 << 11)
#define GS_SOFTWARE_EX                  (1 << 7)

// DWORD 3
#define GS_SCRATCH_SPACE_BASE_SHIFT     (1 << 10)
#define GS_SCRATCH_SPACE_BASE_MASK      0x3fffff
#define GS_PER_THREAD_SPACE_SHIFT       0           // Power of 2 bytes + 1KB
#define GS_PER_THREAD_SPACE_MASK        0xf

// DWORD 4
#define GS_OUTPUT_VTX_SIZE_SHIFT        23          // multiples of 32B - 1
#define GS_OUTPUT_VTX_SIZE_MASK         0x3f
#define GS_OUTPUT_TOPOLOGY_SHIFT        17          // 3DPRIM_TOPOLOGY
#define GS_URB_READ_LENGTH_SHIFT        11
#define GS_URB_READ_LENGTH_MASK         0x3f
#define GS_URB_INCLUDE_VTX_HANDLES      (1 << 10)
#define GS_URB_READ_OFFSET_SHIFT        4
#define GS_URB_READ_OFFSET_MASK         0x3f
#define GS_DISPATCH_GRF_SHIFT           0
#define GS_DISPATCH_GRF_MASK            0xf

// DWORD 5
#define GS_MAX_THREAD_SHIFT             25
#define GS_MAX_THREAD_MASK              0x7f
#define GS_CONTROL_STREAM_ID            (1 << 24)
#define GS_CONTROL_HEADER_SIZE_SHIFT    20
#define GS_CONTROL_HEADER_SIZE_MASK     0xf
#define GS_INSTANCE_CONTROL_SHIFT       15
#define GS_INSTANCE_CONTROL_MASK        0x1f
#define GS_DEFAULT_STREAM_ID_SHIFT      13
#define GS_DEFAULT_STREAM_ID_MASK       0x3
#define GS_DISPATCH_DUAL_OBJECT         (1 << 12)
#define GS_DISPATCH_DUAL_INSTANCE       (1 << 11)
#define GS_STATISTICS                   (1 << 10)
#define GS_INVOCATIONS_INC_SHIFT        5
#define GS_INVOCATIONS_INC_MASK         0x1f
#define GS_INCLUDE_PID                  (1 << 4)
#define GS_HINT                         (1 << 3)
#define GS_REORDE                       (1 << 2)
#define GS_DISCARD_ADJACENCY            (1 << 1)
#define GS_ENABLE                       (1 << 0)

// DWORD 6 - Semaphore Handle

// ------------------------------------------------------------------------------------------------
// 8.4 3DSTATE_STREAMOUT

#define _3DSTATE_STREAMOUT              GFX_INSTR(0x3, 0x0, 0x1e, 1)

// DWORD 1
#define SO_ENABLE                       (1 << 31)
#define SO_RENDERING_DISABLE            (1 << 30)
#define SO_RENDER_STREAM_SHIFT          27
#define SO_RENDER_STREAM_MASK           0x3
#define SO_REORDER_TRAILING             (1 << 26)
#define SO_STATISTICS                   (1 << 25)
#define SO_BUFFER3                      (1 << 11)
#define SO_BUFFER2                      (1 << 10)
#define SO_BUFFER1                      (1 << 9)
#define SO_BUFFER0                      (1 << 8)

// DWORD 2
#define SO_STREAM_READ_LEN_MASK         0x1f
#define SO_STREAM3_READ_OFFSET          (1 << 29)
#define SO_STREAM3_READ_LEN_SHIFT       24
#define SO_STREAM2_READ_OFFSET          (1 << 21)
#define SO_STREAM2_READ_LEN_SHIFT       16
#define SO_STREAM1_READ_OFFSET          (1 << 13)
#define SO_STREAM1_READ_LEN_SHIFT       8
#define SO_STREAM0_READ_OFFSET          (1 << 5)
#define SO_STREAM0_READ_LEN_SHIFT       0

// ------------------------------------------------------------------------------------------------
// 8.5 3DSTATE_SO_DECL_LIST

#define _3DSTATE_SO_DECL_LIST(n)        GFX_INSTR(0x3, 0x1, 0x17, (n)*2 + 1)

// DWORD 1
#define SO_DECLS_STREAM_OFFSETS_MASK    0xf
#define SO_DECLS_STREAM3_OFFSETS_SHIFT  12
#define SO_DECLS_STREAM2_OFFSETS_SHIFT  8
#define SO_DECLS_STREAM1_OFFSETS_SHIFT  4
#define SO_DECLS_STREAM0_OFFSETS_SHIFT  0

// DWORD 2
#define SO_DECLS_STREAM_ENTRIES_MASK    0xff
#define SO_DECLS_STREAM3_ENTRIES_SHIFT  24
#define SO_DECLS_STREAM2_ENTRIES_SHIFT  16
#define SO_DECLS_STREAM1_ENTRIES_SHIFT  8
#define SO_DECLS_STREAM0_ENTRIES_SHIFT  0

// ------------------------------------------------------------------------------------------------
// 8.5.1 SO_DECL

// WORD
#define SO_DECL_OUTPUT_SLOT_SHIFT       12
#define SO_DECL_OUTPUT_SLOT_MASK        0x3
#define SO_DECL_HOLE_FLAG               (1 << 11)
#define SO_DECL_REG_INDEX_SHIFT         4
#define SO_DECL_REG_INDEX_MASK          0x3f
#define SO_DECL_COMP_MASK_SHIFT         0           // xyzw bitfield

// ------------------------------------------------------------------------------------------------
// 8.6 3DSTATE_SO_BUFFER

#define _3DSTATE_SO_BUFFER              GFX_INSTR(0x3, 0x1, 0x18, 2)

// DWORD 1
#define SO_BUF_INDEX_SHIFT              29
#define SO_BUF_INDEX_MASK               0x3
#define SO_BUF_OBJ_CONTROL_STATE_SHIFT  25      // MEMORY_OBJECT_CONTROL_STATE
#define SO_BUF_PITCH_SHIFT              0
#define SO_BUF_PITCH_MASK               0xfff

// DWORD 2 - Surface Base Address
// DWORD 3 - End Base Address

// ------------------------------------------------------------------------------------------------
// 9.3.1.1 3DSTATE_CLIP

#define _3DSTATE_CLIP                   GFX_INSTR(0x3, 0x0, 0x12, 2)

// CULL_MODE
#define CULL_BOTH                       0x0
#define CULL_NONE                       0x1
#define CULL_FRONT                      0x2
#define CULL_BACK                       0x3
#define CULL_MASK                       0x3

// CLIP_MODE
#define CLIP_NORMAL                     0x0
#define CLIP_REJECT_ALL                 0x3
#define CLIP_ACCEPT_ALL                 0x4
#define CLIP_MASK                       0x7

// DWORD 1
#define CLIP_FRONT_CCW                  (1 << 20)
#define CLIP_SUBPIXEL_4_BITS            (1 << 19)
#define CLIP_EARLY_CULL                 (1 << 18)
#define CLIP_CULL_SHIFT                 16          // CULL_MODE
#define CLIP_STATISTICS                 (1 << 10)
#define CLIP_USER_CULL_SHIFT            0
#define CLIP_USER_CULL_MASK             0xff

// DWORD 2
#define CLIP_ENABLE                     (1 << 31)
#define CLIP_API_DX                     (1 << 30)   // Value not documented, 0 is OGL
#define CLIP_VIEWPORT_XY                (1 << 28)
#define CLIP_VIEWPORT_Z                 (1 << 27)
#define CLIP_GUARDBAND                  (1 << 26)
#define CLIP_USER_CLIP_SHIFT            16
#define CLIP_USER_CLIP_MASK             0xff
#define CLIP_MODE_SHIFT                 13          // CLIP_MODE
#define CLIP_PERSP_DIVIDE               (1 << 9)
#define CLIP_NON_PERSP_BARY             (1 << 8)
#define CLIP_TRI_STRIP_LIST_PVTX_SHIFT  4
#define CLIP_TRI_STRIP_LIST_PVTX_MASK   0x3
#define CLIP_LINE_STRIP_LIST_PVTX_SHIFT 2
#define CLIP_LINE_STRIP_LIST_PVTX_MASK  0x3
#define CLIP_TRI_FAN_PVTX_SHIFT         0
#define CLIP_TRI_FAN_PVTX_MASK          0x3

// DWORD 3
#define CLIP_MIN_POINT_WIDTH_SHIFT      17          // Unsigned 8.3
#define CLIP_MIN_POINT_WIDTH_MASK       0x7ff
#define CLIP_MAX_POINT_WIDTH_SHIFT      6           // Unsigned 8.3
#define CLIP_MAX_POINT_WIDTH_MASK       0x7ff
#define CLIP_FORCE_ZERO_RTA_INDEX       (1 << 5)
#define CLIP_MAX_VP_INDEX_SHIFT         0
#define CLIP_MAX_VP_INDE_MASK           0xf

// ------------------------------------------------------------------------------------------------
// 10.3.5.1 3DSTATE_DRAWING_RECTANGLE

#define _3DSTATE_DRAWING_RECTANGLE      GFX_INSTR(0x3, 0x1, 0x00, 2)

// DWORD 1
#define DRAWING_RECT_Y_MIN_SHIFT        16
#define DRAWING_RECT_Y_MIN_MASK         0xffff
#define DRAWING_RECT_X_MIN_SHIFT        0
#define DRAWING_RECT_X_MIN_MASK         0xffff

// DWORD 2
#define DRAWING_RECT_Y_MAX_SHIFT        16
#define DRAWING_RECT_Y_MAX_MASK         0xffff
#define DRAWING_RECT_X_MAX_SHIFT        0
#define DRAWING_RECT_X_MAX_MASK         0xffff

// DWORD 3
#define DRAWING_RECT_Y_ORIGIN_SHIFT     16
#define DRAWING_RECT_Y_ORIGIN_MASK      0xffff
#define DRAWING_RECT_X_ORIGIN_SHIFT     0
#define DRAWING_RECT_X_ORIGIN_MASK      0xffff

// ------------------------------------------------------------------------------------------------
// 10.3.13 3DSTATE_SF

#define _3DSTATE_SF                     GFX_INSTR(0x3, 0x0, 0x13, 5)

// DEPTH_FORMAT
#define DFMT_D32_FLOAT_S8X24_UINT       0x0
#define DFMT_D32_FLOAT                  0x1
#define DFMT_D24_UNORM_S8_UINT          0x2
#define DFMT_D24_UNORM_X8_UINT          0x3
#define DFMT_D16_UNORM                  0x5
#define DFMT_MASK                       0x7

// FILL_MODE
#define FILL_SOLID                      0x0
#define FILL_WIREFRAME                  0x1
#define FILL_POINT                      0x2
#define FILL_MASK                       0x3

// DWORD 1
#define SF_FORMAT_SHIFT                 12          // DEPTH_FORMAT
#define SF_LEGACY_GLOBAL_DEPTH_BIAS     (1 << 11)
#define SF_STATISTICS                   (1 << 10)
#define SF_GLOBAL_DEPTH_OFFSET_SOLID    (1 << 9)
#define SF_GLOBAL_DEPTH_OFFSET_WF       (1 << 8)
#define SF_GLOBAL_DEPTH_OFFSET_POINT    (1 << 7)
#define SF_FRONT_FACE_FILL_SHIFT        5           // FILL_MODE
#define SF_BACK_FACE_FILL_SHIFT         3           // FILL_MODE
#define SF_VIEW_TRANSFORM               (1 << 1)
#define SF_FRONT_WINDING                (1 << 0)

// DWORD 2
#define SF_ANTI_ALIAS                   (1 << 31)
#define SF_CULL_SHIFT                   29          // CULL_MODE
#define SF_LINE_WIDTH_SHIFT             18          // Unsigned 3.7
#define SF_LINE_WIDTH_MASK              0x3ff
#define SF_LINE_END_AA_WIDTH_SHIFT      16
#define SF_SCISSOR                      (1 << 11)
#define SF_MULTISAMPLE_SHIFT            (1 << 8)

// DWORD 3
#define SF_LAST_PIXEL                   (1 << 31)
#define SF_TRI_STRIP_LIST_PVTX_SHIFT    29
#define SF_TRI_STRIP_LIST_PVTX_MASK     0x3
#define SF_LINE_STRIP_LIST_PVTX_SHIFT   27
#define SF_LINE_STRIP_LIST_PVTX_MASK    0x3
#define SF_TRI_FAN_PVTX_SHIFT           25
#define SF_TRI_FAN_PVTX_MASK            0x3
#define SS_AA_LINE_DISTANCE             (1 << 14)
#define SF_SUBPIXEL_4_BITS              (1 << 12)
#define SF_USE_POINT_WIDTH_STATE        (1 << 11)
#define SF_POINT_WIDTH_SHIFT            0           // Unsigned 8.3
#define SF_POINT_WIDTH_MASK             0x7ff

// DWORD 4 - Global Depth Offset Constant (float)
// DWORD 5 - Global Depth Offset Scale (float)
// DWORD 6 - Global Depth Offset Clamp (float)

// ------------------------------------------------------------------------------------------------
// 10.3.14 3DSTATE_SBE

#define _3DSTATE_SBE                    GFX_INSTR(0x3, 0x0, 0x1f, 0xc)

// ATTR_CONST
#define ATTR_CONST_0000                 0x0
#define ATTR_CONST_0001_FLOAT           0x1
#define ATTR_CONST_1111_FLOAT           0x2
#define ATTR_PRIM_ID                    0x3
#define ATTR_CONST_MASK                 0x3

// ATTR_INPUT
#define ATTR_INPUT                      0x0
#define ATTR_INPUT_FACING               0x1
#define ATTR_INPUT_W                    0x2
#define ATTR_INPUT_FACING_W             0x3
#define ATTR_INPUT_MASK                 0x3

// SBE_ATTR
#define SBE_ATTR_OVERRIDE_W             (1 << 15)
#define SBE_ATTR_OVERRIDE_Z             (1 << 14)
#define SBE_ATTR_OVERRIDE_Y             (1 << 13)
#define SBE_ATTR_OVERRIDE_X             (1 << 12)
#define SBE_ATTR_CONST_SOUCE_SHIFT      9           // ATTR_CONST
#define SBE_ATTR_INPUT_SHIFT            6           // ATTR_INPUT
#define SBE_ATTR_SOURCE_INDEX_SHIFT     0
#define SBE_ATTR_SOURCE_INDEX_MASK      0x1f

// DWORD 1
#define SBE_ATTR_SWIZZLE_HIGH_BANK      (1 << 28)
#define SBE_SF_OUTPUT_COUNT_SHIFT       22
#define SBE_SF_OUTPUT_COUNT_MASK        0x3f
#define SBE_ATTR_SWIZZLE                (1 << 21)
#define SBE_POINT_SPRITE_ORIGIN_LL      (1 << 20)
#define SBE_URB_READ_LENGTH_SHIFT       11
#define SBE_URB_READ_LENGTH_MASK        0x3f
#define SBE_URB_READ_OFFSET_SHIFT       4
#define SBE_URB_READ_OFFSET_MASK        0x3f

// DWORD 2-9 - 16 SBE_ATTR elements
// DWORD 10 - Point sprite Texture Coordinate Enable
// DWORD 11 - Constant Interpolation Enable
// DWORD 12 - WrapShortest Enables Attributes 0-7
// DWORD 13 - WrapShortest Enables Attributes 8-15

// ------------------------------------------------------------------------------------------------
// 10.3.15 SF_CLIP_VIEWPORT

typedef struct SFClipViewport
{
    float scaleX;
    float scaleY;
    float scaleZ;
    float transX;
    float transY;
    float transZ;
    float pad0[2];
    float guardbandMinX;
    float guardbandMaxX;
    float guardbandMinY;
    float guardbandMaxY;
    float pad1[4];
} SFClipViewport;

// ------------------------------------------------------------------------------------------------
// 10.3.16 SCISSOR_RECT

typedef struct ScissorRect
{
    u16 minX;
    u16 minY;
    u16 maxX;
    u16 maxY;
} ScissorRect;

// ------------------------------------------------------------------------------------------------
// 12.2 Pixel Pipeline State

// COMPARE_FUNC
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
#define CC_FRONT_FACE_STENCIL_REF_SHIFT 24
#define CC_BACK_FACE_STENCIL_REF_SHIFT  16
#define CC_STENCIL_REF_MASK             0xff
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

// STENCIL_OP
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
#define STENCIL_FAIL_OP_SHIFT               25          // STENCIL_OP
#define STENCIL_DEPTH_FAIL_OP_SHIFT         22          // STENCIL_OP
#define STENCIL_PASS_OP_SHIFT               19          // STENCIL_OP
#define STENCIL_BUFFER_WRITE                (1 << 18)
#define STENCIL_DOUBLE_SIDED                (1 << 15)
#define STENCIL_BACK_FUNC_SHIFT             12          // COMPARE_FUNC
#define STENCIL_BACK_FAIL_OP_SHIFT          9           // STENCIL_OP
#define STENCIL_BACK_DEPTH_FAIL_OP_SHIFT    6           // STENCIL_OP
#define STENCIL_BACK_PASS_OP_SHIFT          3           // STENCIL_OP

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

// BLEND_FUNC
#define BLEND_FUNC_ADD                  0
#define BLEND_FUNC_SUB                  1
#define BLEND_FUNC_REV_SUB              2
#define BLEND_FUNC_MIN                  3
#define BLEND_FUNC_MAX                  4
#define BLEND_FUNC_MASK                 0x7

// BLEND_FACTOR
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

// LOGIC_OP
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

// COLOR_CLAMP
#define COLOR_CLAMP_UNORM               0x0
#define COLOR_CLAMP_SNORM               0x1
#define COLOR_CLAMP_RTFORMAT            0x2
#define COLOR_CLAMP_MASK                0x3

// Flags 0
#define BLEND_COLOR                     (1 << 31)   // Only BLEND_COLOR or BLEND_LOGIC can be enabled
#define BLEND_INDEPENDENT_ALPHA         (1 << 30)
#define BLEND_FUNC_ALPHA_SHIFT          26          // BLEND_FUNC
#define BLEND_SRC_ALPHA_SHIFT           20          // BLEND_FACTOR
#define BLEND_DST_ALPHA_SHIFT           15          // BLEND_FACTOR
#define BLEND_FUNC_COLOR_SHIFT          11          // BLEND_FUNC
#define BLEND_SRC_COLOR_SHIFT           5           // BLEND_FACTOR
#define BLEND_DST_COLOR_SHIFT           0           // BLEND_FACTOR

// Flags 1
#define BLEND_ALPHA_TO_COVERAGE         (1 << 31)
#define BLEND_ALPHA_TO_ONE              (1 << 30)   // Errata - must be disabled
#define BLEND_ALPHA_TO_COVERAGE_DITHER  (1 << 29)
#define BLEND_DISABLE_ALPHA             (1 << 27)   // Errata - must be set to 0 if not present in the render target
#define BLEND_DISABLE_RED               (1 << 26)   // Errata - must be set to 0 if not present in the render target
#define BLEND_DISABLE_GREEN             (1 << 25)   // Errata - must be set to 0 if not present in the render target
#define BLEND_DISABLE_BLUE              (1 << 24)   // Errata - must be set to 0 if not present in the render target
#define BLEND_LOGIC                     (1 << 22)   // Only BLEND_COLOR or BLEND_LOGIC can be enabled
#define BLEND_LOGIC_OP_SHIFT            18          // LOGIC_OP
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
// 12.2.4 CC_VIEWPORT

typedef struct CCViewport
{
    float minDepth;
    float maxDepth;
} CCViewport;

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
// Vol 4. Part 1. Subsystems ad Cores - Shared Functions
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// 2.12.2 Surface State

// SURFTYPE
#define SURFTYPE_1D                     0x0
#define SURFTYPE_2D                     0x1
#define SURFTYPE_3D                     0x2
#define SURFTYPE_CUBE                   0x3
#define SURFTYPE_BUFFER                 0x4
#define SURFTYPE_STRBUF                 0x5
#define SURFTYPE_NULL                   0x7
#define SURFTYPE_MASK                   0x7

// FMT (SURFACE_FORMAT)
// VF = can be used by VF Unit
#define FMT_R32G32B32A32_FLOAT          0x000       // VF
#define FMT_R32G32B32A32_SINT           0x001       // VF
#define FMT_R32G32B32A32_UINT           0x002       // VF
#define FMT_R32G32B32A32_UNORM          0x003       // VF
#define FMT_R32G32B32A32_SNORM          0x004       // VF
#define FMT_R64G64_FLOAT                0x005       // VF
#define FMT_R32G32B32X32_FLOAT          0x006
#define FMT_R32G32B32A32_SSCALED        0x007       // VF
#define FMT_R32G32B32A32_USCALED        0x008       // VF
#define FMT_R32G32B32A32_SFIXED         0x020
#define FMT_R64G64_PASSTHRU             0x021
#define FMT_R32G32B32_FLOAT             0x040       // VF
#define FMT_R32G32B32_SINT              0x041       // VF
#define FMT_R32G32B32_UINT              0x042       // VF
#define FMT_R32G32B32_UNORM             0x043       // VF
#define FMT_R32G32B32_SNORM             0x044       // VF
#define FMT_R32G32B32_SSCALED           0x045       // VF
#define FMT_R32G32B32_USCALED           0x046       // VF
#define FMT_R32G32B32_SFIXED            0x050
#define FMT_R16G16B16A16_UNORM          0x080       // VF
#define FMT_R16G16B16A16_SNORM          0x081       // VF
#define FMT_R16G16B16A16_SINT           0x082       // VF
#define FMT_R16G16B16A16_UINT           0x083       // VF
#define FMT_R16G16B16A16_FLOAT          0x084       // VF
#define FMT_R32G32_FLOAT                0x085       // VF
#define FMT_R32G32_SINT                 0x086       // VF
#define FMT_R32G32_UINT                 0x087       // VF
#define FMT_R32_FLOAT_X8X24_TYPELESS    0x088
#define FMT_X32_TYPELESS_G8X24_UINT     0x089
#define FMT_L32A32_FLOAT                0x08A
#define FMT_R32G32_UNORM                0x08B       // VF
#define FMT_R32G32_SNORM                0x08C       // VF
#define FMT_R64_FLOAT                   0x08D       // VF
#define FMT_R16G16B16X16_UNORM          0x08E
#define FMT_R16G16B16X16_FLOAT          0x08F
#define FMT_A32X32_FLOAT                0x090
#define FMT_L32X32_FLOAT                0x091
#define FMT_I32X32_FLOAT                0x092
#define FMT_R16G16B16A16_SSCALED        0x093       // VF
#define FMT_R16G16B16A16_USCALED        0x094       // VF
#define FMT_R32G32_SSCALED              0x095       // VF
#define FMT_R32G32_USCALED              0x096       // VF
#define FMT_R32G32_SFIXED               0x0A0
#define FMT_R64_PASSTHRU                0x0A1
#define FMT_B8G8R8A8_UNORM              0x0C0       // VF
#define FMT_B8G8R8A8_UNORM_SRGB         0x0C1
#define FMT_R10G10B10A2_UNORM           0x0C2       // VF
#define FMT_R10G10B10A2_UNORM_SRGB      0x0C3
#define FMT_R10G10B10A2_UINT            0x0C4       // VF
#define FMT_R10G10B10_SNORM_A2_UNORM    0x0C5       // VF
#define FMT_R8G8B8A8_UNORM              0x0C7       // VF
#define FMT_R8G8B8A8_UNORM_SRGB         0x0C8
#define FMT_R8G8B8A8_SNORM              0x0C9       // VF
#define FMT_R8G8B8A8_SINT               0x0CA       // VF
#define FMT_R8G8B8A8_UINT               0x0CB       // VF
#define FMT_R16G16_UNORM                0x0CC       // VF
#define FMT_R16G16_SNORM                0x0CD       // VF
#define FMT_R16G16_SINT                 0x0CE       // VF
#define FMT_R16G16_UINT                 0x0CF       // VF
#define FMT_R16G16_FLOAT                0x0D0       // VF
#define FMT_B10G10R10A2_UNORM           0x0D1
#define FMT_B10G10R10A2_UNORM_SRGB      0x0D2
#define FMT_R11G11B10_FLOAT             0x0D3       // VF
#define FMT_R32_SINT                    0x0D6       // VF
#define FMT_R32_UINT                    0x0D7       // VF
#define FMT_R32_FLOAT                   0x0D8       // VF
#define FMT_R24_UNORM_X8_TYPELESS       0x0D9
#define FMT_X24_TYPELESS_G8_UINT        0x0DA
#define FMT_L32_UNORM                   0x0DD
#define FMT_A32_UNORM                   0x0DE
#define FMT_L16A16_UNORM                0x0DF
#define FMT_I24X8_UNORM                 0x0E0
#define FMT_L24X8_UNORM                 0x0E1
#define FMT_A24X8_UNORM                 0x0E2
#define FMT_I32_FLOAT                   0x0E3
#define FMT_L32_FLOAT                   0x0E4
#define FMT_A32_FLOAT                   0x0E5
#define FMT_X8B8_UNORM_G8R8_SNORM       0x0E6
#define FMT_A8X8_UNORM_G8R8_SNORM       0x0E7
#define FMT_B8X8_UNORM_G8R8_SNORM       0x0E8
#define FMT_B8G8R8X8_UNORM              0x0E9
#define FMT_B8G8R8X8_UNORM_SRGB         0x0EA
#define FMT_R8G8B8X8_UNORM              0x0EB
#define FMT_R8G8B8X8_UNORM_SRGB         0x0EC
#define FMT_R9G9B9E5_SHAREDEXP          0x0ED
#define FMT_B10G10R10X2_UNORM           0x0EE
#define FMT_L16A16_FLOAT                0x0F0
#define FMT_R32_UNORM                   0x0F1       // VF
#define FMT_R32_SNORM                   0x0F2       // VF
#define FMT_R10G10B10X2_USCALED         0x0F3       // VF
#define FMT_R8G8B8A8_SSCALED            0x0F4       // VF
#define FMT_R8G8B8A8_USCALED            0x0F5       // VF
#define FMT_R16G16_SSCALED              0x0F6       // VF
#define FMT_R16G16_USCALED              0x0F7       // VF
#define FMT_R32_SSCALED                 0x0F8       // VF
#define FMT_R32_USCALED                 0x0F9       // VF
#define FMT_B5G6R5_UNORM                0x100
#define FMT_B5G6R5_UNORM_SRGB           0x101
#define FMT_B5G5R5A1_UNORM              0x102
#define FMT_B5G5R5A1_UNORM_SRGB         0x103
#define FMT_B4G4R4A4_UNORM              0x104
#define FMT_B4G4R4A4_UNORM_SRGB         0x105
#define FMT_R8G8_UNORM                  0x106       // VF
#define FMT_R8G8_SNORM                  0x107       // VF
#define FMT_R8G8_SINT                   0x108       // VF
#define FMT_R8G8_UINT                   0x109       // VF
#define FMT_R16_UNORM                   0x10A       // VF
#define FMT_R16_SNORM                   0x10B       // VF
#define FMT_R16_SINT                    0x10C       // VF
#define FMT_R16_UINT                    0x10D       // VF
#define FMT_R16_FLOAT                   0x10E       // VF
#define FMT_A8P8_UNORM_PALETTE0         0x10F
#define FMT_A8P8_UNORM_PALETTE1         0x110
#define FMT_I16_UNORM                   0x111
#define FMT_L16_UNORM                   0x112
#define FMT_A16_UNORM                   0x113
#define FMT_L8A8_UNORM                  0x114
#define FMT_I16_FLOAT                   0x115
#define FMT_L16_FLOAT                   0x116
#define FMT_A16_FLOAT                   0x117
#define FMT_L8A8_UNORM_SRGB             0x118
#define FMT_R5G5_SNORM_B6_UNORM         0x119
#define FMT_B5G5R5X1_UNORM              0x11A
#define FMT_B5G5R5X1_UNORM_SRGB         0x11B
#define FMT_R8G8_SSCALED                0x11C       // VF
#define FMT_R8G8_USCALED                0x11D       // VF
#define FMT_R16_SSCALED                 0x11E       // VF
#define FMT_R16_USCALED                 0x11F       // VF
#define FMT_P8A8_UNORM_PALETTE0         0x122
#define FMT_P8A8_UNORM_PALETTE1         0x123
#define FMT_A1B5G5R5_UNORM              0x124
#define FMT_A4B4G4R4_UNORM              0x125
#define FMT_L8A8_UINT                   0x126
#define FMT_L8A8_SINT                   0x127
#define FMT_R8_UNORM                    0x140       // VF
#define FMT_R8_SNORM                    0x141       // VF
#define FMT_R8_SINT                     0x142
#define FMT_R8_UINT                     0x143
#define FMT_A8_UNORM                    0x144
#define FMT_I8_UNORM                    0x145
#define FMT_L8_UNORM                    0x146
#define FMT_P4A4_UNORM_PALETTE0         0x147
#define FMT_A4P4_UNORM_PALETTE0         0x148
#define FMT_R8_SSCALED                  0x149       // VF
#define FMT_R8_USCALED                  0x14A       // VF
#define FMT_P8_UNORM_PALETTE0           0x14B
#define FMT_L8_UNORM_SRGB               0x14C
#define FMT_P8_UNORM_PALETTE1           0x14D
#define FMT_P4A4_UNORM_PALETTE1         0x14E
#define FMT_A4P4_UNORM_PALETTE1         0x14F
#define FMT_Y8_UNORM                    0x150
#define FMT_L8_UINT                     0x152
#define FMT_L8_SINT                     0x153
#define FMT_I8_UINT                     0x154
#define FMT_I8_SINT                     0x155
#define FMT_DXT1_RGB_SRGB               0x180
#define FMT_R1_UNORM                    0x181
#define FMT_YCRCB_NORMAL                0x182
#define FMT_YCRCB_SWAPUVY               0x183
#define FMT_P2_UNORM_PALETTE0           0x184
#define FMT_P2_UNORM_PALETTE1           0x185
#define FMT_BC1_UNORM                   0x186
#define FMT_BC2_UNORM                   0x187
#define FMT_BC3_UNORM                   0x188
#define FMT_BC4_UNORM                   0x189
#define FMT_BC5_UNORM                   0x18A
#define FMT_BC1_UNORM_SRGB              0x18B
#define FMT_BC2_UNORM_SRGB              0x18C
#define FMT_BC3_UNORM_SRGB              0x18D
#define FMT_MONO8                       0x18E
#define FMT_YCRCB_SWAPUV                0x18F
#define FMT_YCRCB_SWAPY                 0x190
#define FMT_DXT1_RGB                    0x191
#define FMT_FXT1                        0x192
#define FMT_R8G8B8_UNORM                0x193       // VF
#define FMT_R8G8B8_SNORM                0x194       // VF
#define FMT_R8G8B8_SSCALED              0x195       // VF
#define FMT_R8G8B8_USCALED              0x196       // VF
#define FMT_R64G64B64A64_FLOAT          0x197       // VF
#define FMT_R64G64B64_FLOAT             0x198       // VF
#define FMT_BC4_SNORM                   0x199
#define FMT_BC5_SNORM                   0x19A
#define FMT_R16G16B16_FLOAT             0x19B       // VF
#define FMT_R16G16B16_UNORM             0x19C       // VF
#define FMT_R16G16B16_SNORM             0x19D       // VF
#define FMT_R16G16B16_SSCALED           0x19E       // VF
#define FMT_R16G16B16_USCALED           0x19F       // VF
#define FMT_BC6H_SF16                   0x1A1
#define FMT_BC7_UNORM                   0x1A2
#define FMT_BC7_UNORM_SRGB              0x1A3
#define FMT_BC6H_UF16                   0x1A4
#define FMT_PLANAR_420_8                0x1A5
#define FMT_R8G8B8_UNORM_SRGB           0x1A8
#define FMT_ETC1_RGB8                   0x1A9
#define FMT_ETC2_RGB8                   0x1AA
#define FMT_EAC_R11                     0x1AB
#define FMT_EAC_RG11                    0x1AC
#define FMT_EAC_SIGNED_R11              0x1AD
#define FMT_EAC_SIGNED_RG11             0x1AE
#define FMT_ETC2_SRGB8                  0x1AF
#define FMT_R16G16B16_UINT              0x1B0
#define FMT_R16G16B16_SINT              0x1B1
#define FMT_R32_SFIXED                  0x1B2
#define FMT_R10G10B10A2_SNORM           0x1B3
#define FMT_R10G10B10A2_USCALED         0x1B4
#define FMT_R10G10B10A2_SSCALED         0x1B5
#define FMT_R10G10B10A2_SINT            0x1B6
#define FMT_B10G10R10A2_SNORM           0x1B7
#define FMT_B10G10R10A2_USCALED         0x1B8
#define FMT_B10G10R10A2_SSCALED         0x1B9
#define FMT_B10G10R10A2_UINT            0x1BA
#define FMT_B10G10R10A2_SINT            0x1BB
#define FMT_R64G64B64A64_PASSTHRU       0x1BC
#define FMT_R64G64B64_PASSTHRU          0x1BD
#define FMT_ETC2_RGB8_PTA               0x1C0
#define FMT_ETC2_SRGB8_PTA              0x1C1
#define FMT_ETC2_EAC_RGBA8              0x1C2
#define FMT_ETC2_EAC_SRGB8_A8           0x1C3
#define FMT_R8G8B8_UINT                 0x1C8
#define FMT_R8G8B8_SINT                 0x1C9
#define FMT_RAW                         0x1FF

// MEDIA_BOUNDARY
#define MEDIA_BOUNDARY_NORMAL           0x0
#define MEDIA_BOUNDARY_PROGRESSIVE      0x2
#define MEDIA_BOUNDARY_INTERLACED       0x3
#define MEDIA_BOUNDARY_MASK             0x3

// RTROTATE
#define RTROTATE_0                      0x0
#define RTROTATE_90                     0x1
#define RTROTATE_270                    0x3
#define RTROTATE_MASK                   0x3

// MULTISAMPLECOUNT
#define MULTISAMPLECOUNT_1              0x0
#define MULTISAMPLECOUNT_4              0x2
#define MULTISAMPLECOUNT_8              0x3
#define MULTISAMPLECOUNT_MASK           0x7

// Flags 0
#define SURFACE_TYPE_SHIFT              (1 << 29)   // SURFTYPE
#define SURFACE_ARRAY                   (1 << 28)
#define SURFACE_FORMAT_SHIFT            18          // SURFACE_FORMAT
#define SURFACE_VERT_ALIGN_SHIFT        16          // Values not doucmented
#define SURFACE_HALIGN_8                (1 << 15)
#define SURFACE_TILED                   (1 << 14)
#define SURFACE_TILE_YMAJOR             (1 << 13)
#define SURFACE_VERT_LINE_STRIDE        (1 << 12)
#define SURFACE_VERT_LINE_STRIDE_OFFSET (1 << 11)
#define SURFACE_ARRAY_LOD0              (1 << 10)
#define SURFACE_RENDER_CACHE_RW         (1 << 8)
#define SURFACE_MEDIA_BOUNDARY_SHIFT    6           // MEDIA_BOUNDARY
#define SURFACE_CUBE_NEG_X              (1 << 5)
#define SURFACE_CUBE_POS_X              (1 << 4)
#define SURFACE_CUBE_NEG_Y              (1 << 3)
#define SURFACE_CUBE_POS_Y              (1 << 2)
#define SURFACE_CUBE_NEG_Z              (1 << 1)
#define SURFACE_CUBE_POS_Z              (1 << 0)

// Pitch/Depth
#define SURFACE_DEPTH_SHIFT             21
#define SURFACE_DEPTH_MASK              0x3ff
#define SURFACE_PITCH_SHIFT             0
#define SURFACE_PITCH_MASK              0x3ffff

// Flags 1 - minimum array element for SURFTYPE_STRBUF
#define SURFACE_ROTATION_SHIFT          29          // RTROTATE
#define SURFACE_MIN_ELEMENT_SHIFT       18
#define SURFACE_MIN_ELEMENT_MASK        0x7ff
#define SURFACE_RT_VIEW_EXTENT_SHIFT    7
#define SURFACE_RT_VIEW_EXTENT_MASK     0x7ff
#define SURFACE_MS_DEPTH                (1 << 6)
#define SURFACE_MS_COUNT_SHIFT          3           // MULTISAMPLECOUNT
#define SURFACE_MS_PALETTE_INDEX_SHIFT  0
#define SURFACE_MS_PALETTE_INDEX_MASK   0x7

// Flags 2
#define SURFACE_X_OFFSET_SHIFT          25
#define SURFACE_X_OFFSET_MASK           0x7f
#define SURFACE_Y_OFFSET_SHIFT          20
#define SURFACE_Y_OFFSET_MASK           0xf
#define SURFACE_OBJ_CONTROL_STATE_SHIFT 16          // MEMORY_OBJECT_CONTROL_STATE
#define SURFACE_MIN_LOD_SHIFT           4
#define SURFACE_MIN_LOD_MASK            0xf
#define SURFACE_MIP_COUNT_SHIFT         0
#define SURFACE_MIP_COUNT_MASK          0xf

// Flags 3 - Surface Format == PLANAR
#define SURFACE_X_OFFSET_UV_SHIFT       16
#define SURFACE_X_OFFSET_UV_MASK        0x3fff
#define SURFACE_Y_OFFSET_UV_SHIFT       0
#define SURFACE_Y_OFFSET_UV_MASK        0x3fff

// Flags 3 - Surface Format != PLANAR && SURFACE_MCS
// MCS Base Address (4KB aligned)
#define SURFACE_MCS_PITCH_SHIFT         3
#define SUFFACE_MCS_PITCH_MASK          0x1ff
#define SURFACE_MCS                     (1 << 0)

// Flags 3 - Surface Format != PLANAR && !SURFACE_MCS
// Surface Append Counter Address
#define SURFACE_APPEND_COUNTER          (1 << 1)

// Flags 4
#define SURFACE_RED_CLEAR_COLOR         (1 << 31)
#define SURFACE_GREEN_CLEAR_COLOR       (1 << 30)
#define SURFACE_BLUE_CLEAR_COLOR        (1 << 29)
#define SURFACE_ALPHA_CLEAR_COLOR       (1 << 28)
#define SURFACE_RSRC_MIN_LOD_SHIFT      0
#define SURFACE_RSRC_MIN_LOD_MASK       0xfff

typedef struct SurfaceState
{
    u32 flags0;
    u32 baseAddr;
    u16 height;
    u16 width;
    u32 pitchDepth;
    u32 flags1;
    u32 flags2;
    u32 flags3;
    u32 flags4;
} SurfaceState;

// ------------------------------------------------------------------------------------------------
// 2.12.3 Sampler State

// MIP_FILTER
#define MIP_FILTER_NONE                 0x0
#define MIP_FILTER_NEAREST              0x1
#define MIP_FILTER_LINEAR               0x3
#define MIP_FILTER_MASK                 0x3

// MAP_FILTER
#define MAP_FILTER_NONE                 0x0
#define MAP_FILTER_LINEAR               0x1
#define MAP_FILTER_ANISO                0x2
#define MAP_FILTER_MONO                 0x6
#define MAP_FILTER_MASK                 0x7

// ANISO_RATIO
#define ANISO_RATIO_2                   0x0
#define ANISO_RATIO_4                   0x1
#define ANISO_RATIO_6                   0x2
#define ANISO_RATIO_8                   0x3
#define ANISO_RATIO_10                  0x4
#define ANISO_RATIO_12                  0x5
#define ANISO_RATIO_14                  0x6
#define ANISO_RATIO_16                  0x7
#define ANISO_RATIO_MASK                0x7

// TRIQUAL
#define TRIQUAL_FULL                    0x0
#define TRIQUAL_MED                     0x2
#define TRIQUAL_LOW                     0x3
#define TRIQUAL_MASK                    0x3

// TEXTURE_ADDRESS
#define TEXTURE_ADDRESS_WRAP            0x0
#define TEXTURE_ADDRESS_MIRROR          0x1
#define TEXTURE_ADDRESS_CLAMP           0x2
#define TEXTURE_ADDRESS_CUBE            0x3
#define TEXTURE_ADDRESS_CLAMP_BORDER    0x4
#define TEXTURE_ADDRESS_MIRROR_ONCE     0x5
#define TEXTURE_ADDRESS_MASK            0x7

// Flags 0
#define SAMPLER_DISABLE                 (1 << 31)
#define SAMPLER_BORDER_COLOR_DX9        (1 << 29)   // Must be set for P4A4_UNORM or A4P4_UNORM
#define SAMPLER_LOD_PRECLAMP_OGL        (1 << 28)
#define SAMPLER_BASE_MIP_SHIFT          22          // Unsigned 4.1 [0.0, 14.0]
#define SAMPLER_MIP_FILTER_SHIFT        20          // MIP_FILTER
#define SAMPLER_MAG_FILTER_SHIFT        17          // MAP_FILTER
#define SAMPLER_MIN_FILTER_SHIFT        14          // MAP_FILTER
#define SAMPLER_LOD_BIAS_SHIFT          1           // Signed 4.8 [-16.0, 16.0)
#define SAMPLER_ANISO_EWA               (1 << 0)

// Flags 1
#define SAMPLER_MIN_LOD_SHIFT           20          // Unsigned 4.8 [0.0, 14.0]
#define SAMPLER_MAX_LOD_SHIFT           8           // Unsigned 4.8 [0.0, 14.0]
#define SAMPLER_COMPARISON_FUNC_SHIFT   1           // COMPARE_FUNC
#define SAMPLER_CUBE_MODE_OVERRIDE      (1 << 0)    // Must not be set

// Flags 2
#define SAMPLER_CHROMA_KEY              (1 << 25)
#define SAMPLER_CHROMA_KEY_INDEX_SHIFT  23
#define SAMPLER_CHROMA_KEY_MODE         (1 << 22)
#define SAMPLER_MAX_ANISO_SHIFT         19          // ANISO_RATIO
#define SAMPLER_ADDRESS_ROUND_SHIFT     13
#define SAMPLER_TRILINEAR_SHIFT         11          // TRIQUAL
#define SAMPLER_NON_NORMALIZED          (1 << 10)
#define SAMPLER_ADDRESS_U_SHIFT         6
#define SAMPLER_ADDRESS_V_SHIFT         3
#define SAMPLER_ADDRESS_W_SHIFT         0

typedef struct SamplerState
{
    u32 flags0;
    u32 flags1;
    u32 borderColorAddr;                // SAMPLER_BORDER_COLOR_STATE relative to Dynamic State Base Address
    u32 flags2;
} SamplerState;

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
