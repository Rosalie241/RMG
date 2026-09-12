#include <string.h>
#include "RSP.h"
#include "N64.h"
#include "arm_neon.h"
#include "GBI.h"

struct _N64Matrix
{
    s16 integer[4][4];
    u16 fraction[4][4];
};

// 16 s16 integer parts followed by 16 u16 fraction parts, no padding.
static_assert(sizeof(_N64Matrix) == 64, "unexpected _N64Matrix layout");

bool RSP_LoadMatrix( f32 mtx[4][4], u32 address )
{
    // Written to avoid overflowing the addition; address comes from a display
    // list and is not trustworthy.
    if (address > RDRAMSize || RDRAMSize - address < sizeof(_N64Matrix))
        return false;

    f32 recip = FIXED2FLOATRECIP16;

    // Copy into a local rather than overlaying the struct on RDRAM. Casting
    // u8* to a struct pointer violates strict aliasing, and vld1 on such a
    // pointer would also assume an alignment that an arbitrary display list
    // address does not guarantee. The local is naturally aligned.
    struct _N64Matrix n64Mat;
    memcpy(&n64Mat, RDRAM + address, sizeof(n64Mat));

    // Load recip
    float32_t _recip = recip;

    // Load integer
    int16x4x4_t _integer_s16;
    _integer_s16.val[0] = vld1_s16(n64Mat.integer[0]);
    _integer_s16.val[1] = vld1_s16(n64Mat.integer[1]);
    _integer_s16.val[2] = vld1_s16(n64Mat.integer[2]);
    _integer_s16.val[3] = vld1_s16(n64Mat.integer[3]);

    // Load fraction
    uint16x4x4_t _fraction_u16;
    _fraction_u16.val[0] = vld1_u16(n64Mat.fraction[0]);
    _fraction_u16.val[1] = vld1_u16(n64Mat.fraction[1]);
    _fraction_u16.val[2] = vld1_u16(n64Mat.fraction[2]);
    _fraction_u16.val[3] = vld1_u16(n64Mat.fraction[3]);

    // Reverse 16bit values --> j^1
    _integer_s16.val[0] = vrev32_s16 (_integer_s16.val[0]);                 // 0 1 2 3 --> 1 0 3 2
    _integer_s16.val[1] = vrev32_s16 (_integer_s16.val[1]);                 // 0 1 2 3 --> 1 0 3 2 
    _integer_s16.val[2] = vrev32_s16 (_integer_s16.val[2]);                 // 0 1 2 3 --> 1 0 3 2
    _integer_s16.val[3] = vrev32_s16 (_integer_s16.val[3]);                 // 0 1 2 3 --> 1 0 3 2
    _fraction_u16.val[0] = vrev32_u16 (_fraction_u16.val[0]);               // 0 1 2 3 --> 1 0 3 2
    _fraction_u16.val[1] = vrev32_u16 (_fraction_u16.val[1]);               // 0 1 2 3 --> 1 0 3 2 
    _fraction_u16.val[2] = vrev32_u16 (_fraction_u16.val[2]);               // 0 1 2 3 --> 1 0 3 2 
    _fraction_u16.val[3] = vrev32_u16 (_fraction_u16.val[3]);               // 0 1 2 3 --> 1 0 3 2
    
    // Expand to 32Bit int/uint
    int32x4x4_t _integer_s32;
    uint32x4x4_t _fraction_u32;
    _integer_s32.val[0] = vmovl_s16(_integer_s16.val[0]);         // _integer0_s32 = (i32)_integer0_s16
    _integer_s32.val[1] = vmovl_s16(_integer_s16.val[1]);         // _integer1_s32 = (i32)_integer1_s16
    _integer_s32.val[2] = vmovl_s16(_integer_s16.val[2]);         // _integer2_s32 = (i32)_integer2_s16
    _integer_s32.val[3] = vmovl_s16(_integer_s16.val[3]);         // _integer3_s32 = (i32)_integer3_s16
    _fraction_u32.val[0] = vmovl_u16(_fraction_u16.val[0]);      // _fraction0_u32 = (u32)_fraction0_u16
    _fraction_u32.val[1] = vmovl_u16(_fraction_u16.val[1]);      // _fraction1_u32 = (u32)_fraction1_u16
    _fraction_u32.val[2] = vmovl_u16(_fraction_u16.val[2]);      // _fraction2_u32 = (u32)_fraction2_u16
    _fraction_u32.val[3] = vmovl_u16(_fraction_u16.val[3]);      // _fraction3_u32 = (u32)_fraction3_u16
    
    // Convert to Float
    float32x4x4_t _integer_f32;
    float32x4x4_t _fraction_f32;
    _integer_f32.val[0] = vcvtq_f32_s32 (_integer_s32.val[0]);  // _integer0_f32 = (f32)_integer0_s32
    _integer_f32.val[1] = vcvtq_f32_s32 (_integer_s32.val[1]);  // _integer1_f32 = (f32)_integer1_s32 
    _integer_f32.val[2] = vcvtq_f32_s32 (_integer_s32.val[2]);  // _integer2_f32 = (f32)_integer2_s32 
    _integer_f32.val[3] = vcvtq_f32_s32 (_integer_s32.val[3]);  // _integer3_f32 = (f32)_integer3_s32 
    _fraction_f32.val[0] = vcvtq_f32_u32 (_fraction_u32.val[0]);// _fraction0_f32 = (f32)_fraction0_u32 
    _fraction_f32.val[1] = vcvtq_f32_u32 (_fraction_u32.val[1]);// _fraction1_f32 = (f32)_fraction1_u32 
    _fraction_f32.val[2] = vcvtq_f32_u32 (_fraction_u32.val[2]);// _fraction2_f32 = (f32)_fraction2_u32
    _fraction_f32.val[3] = vcvtq_f32_u32 (_fraction_u32.val[3]);// _fraction3_f32 = (f32)_fraction3_u32

    // Multiply and add
    _integer_f32.val[0] = vmlaq_n_f32(_integer_f32.val[0],_fraction_f32.val[0],_recip);// _integer0_f32 = _integer0_f32 + _fraction0_f32* _recip
    _integer_f32.val[1] = vmlaq_n_f32(_integer_f32.val[1],_fraction_f32.val[1],_recip);// _integer1_f32 = _integer1_f32 + _fraction1_f32* _recip
    _integer_f32.val[2] = vmlaq_n_f32(_integer_f32.val[2],_fraction_f32.val[2],_recip);// _integer2_f32 = _integer2_f32 + _fraction2_f32* _recip
    _integer_f32.val[3] = vmlaq_n_f32(_integer_f32.val[3],_fraction_f32.val[3],_recip);// _integer3_f32 = _integer3_f32 + _fraction3_f32* _recip

    // Store in mtx
    vst1q_f32(mtx[0], _integer_f32.val[0]);
    vst1q_f32(mtx[1], _integer_f32.val[1]);
    vst1q_f32(mtx[2], _integer_f32.val[2]);
    vst1q_f32(mtx[3], _integer_f32.val[3]);

    return true;
}

