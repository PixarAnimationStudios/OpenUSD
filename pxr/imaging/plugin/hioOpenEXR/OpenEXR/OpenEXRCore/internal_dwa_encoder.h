//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMF_INTERNAL_DWA_HELPERS_H_HAS_BEEN_INCLUDED
#    error "only include internal_dwa_helpers.h"
#endif
#include <limits.h>
#include <float.h>

#ifdef _WIN32
#    include <intrin.h>
#elif defined(__x86_64__)
#    include <x86intrin.h>
#endif

#if defined(__has_builtin)
#    if __has_builtin(__builtin_popcount)
#        define USE_POPCOUNT 1
#    endif
#    if __has_builtin(__builtin_clz)
#        define USE_CLZ 1
#    endif
#endif
#ifndef USE_POPCOUNT
#    define USE_POPCOUNT 0
#endif

#ifndef USE_CLZ
#    ifdef _MSC_VER
static int __inline __builtin_clz(uint32_t v)
{
#ifdef __BMI1__
    return __lzcnt(v);
#else
    unsigned long r;
    _BitScanReverse(&r, v);
    return 31 - r;
#endif
}
#        define USE_CLZ 1
#    else
#        define USE_CLZ 0
#    endif
#endif

//
// Base 'class' for encoding using the lossy DCT scheme
//

typedef struct _LossyDctEncoder
{
    const uint16_t* _toNonlinear;

    uint64_t _numAcComp, _numDcComp;

    DctCoderChannelData* _channel_encode_data[3];
    int                  _channel_encode_data_count;

    int   _width, _height;
    float _quantBaseError;

    //
    // Pointers to the buffers where AC and DC
    // DCT components should be packed for
    // lossless compression downstream
    //

    uint8_t* _packedAc;
    uint8_t* _packedDc;

    //
    // Our "quantization tables" - the example JPEG tables,
    // normalized so that the smallest value in each is 1.0.
    // This gives us a relationship between error in DCT
    // components
    //

    float _quantTableY[64];
    uint16_t _hquantTableY[64];

    float _quantTableCbCr[64];
    uint16_t _hquantTableCbCr[64];
} LossyDctEncoder;

static exr_result_t LossyDctEncoder_base_construct (
    LossyDctEncoder* e,
    float            quantBaseError,
    uint8_t*         packedAc,
    uint8_t*         packedDc,
    const uint16_t*  toNonlinear,
    int              width,
    int              height);

static exr_result_t LossyDctEncoder_construct (
    LossyDctEncoder*     e,
    float                quantBaseError,
    DctCoderChannelData* rowPtrs,
    uint8_t*             packedAc,
    uint8_t*             packedDc,
    const uint16_t*      toNonlinear,
    int                  width,
    int                  height);

static exr_result_t LossyDctEncoderCsc_construct (
    LossyDctEncoder*     e,
    float                quantBaseError,
    DctCoderChannelData* rowPtrsR,
    DctCoderChannelData* rowPtrsG,
    DctCoderChannelData* rowPtrsB,
    uint8_t*             packedAc,
    uint8_t*             packedDc,
    const uint16_t*      toNonlinear,
    int                  width,
    int                  height);

static exr_result_t LossyDctEncoder_execute (
    void* (*alloc_fn) (size_t), void (*free_fn) (void*), LossyDctEncoder* e);

static void
LossyDctEncoder_rleAc (LossyDctEncoder* e, uint16_t* block, uint16_t** acPtr);

/**************************************/

exr_result_t
LossyDctEncoder_base_construct (
    LossyDctEncoder* e,
    float            quantBaseError,
    uint8_t*         packedAc,
    uint8_t*         packedDc,
    const uint16_t*  toNonlinear,
    int              width,
    int              height)
{
    //
    // Here, we take the generic JPEG quantization tables and
    // normalize them by the smallest component in each table.
    // This gives us a relationship amongst the DCT components,
    // in terms of how sensitive each component is to
    // error.
    //
    // A higher normalized value means we can quantize more,
    // and a small normalized value means we can quantize less.
    //
    // Eventually, we will want an acceptable quantization
    // error range for each component. We find this by
    // multiplying some user-specified level (_quantBaseError)
    // by the normalized table (_quantTableY, _quantTableCbCr) to
    // find the acceptable quantization error range.
    //
    // The quantization table is not needed for decoding, and
    // is not transmitted. So, if you want to get really fancy,
    // you could derive some content-dependent quantization
    // table, and the decoder would not need to be changed. But,
    // for now, we'll just use static quantization tables.
    //
    int jpegQuantTableY[] = {
        16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,
        14, 13, 16, 24, 40,  57,  69,  56,  14, 17, 22, 29, 51,  87,  80,  62,
        18, 22, 37, 56, 68,  109, 103, 77,  24, 35, 55, 64, 81,  104, 113, 92,
        49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};

    int jpegQuantTableYMin = 10;

    int jpegQuantTableCbCr[] = {
        17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};

    int jpegQuantTableCbCrMin = 17;

    e->_quantBaseError = quantBaseError;
    e->_width          = width;
    e->_height         = height;
    e->_toNonlinear    = toNonlinear;
    e->_numAcComp      = 0;
    e->_numDcComp      = 0;
    e->_packedAc       = packedAc;
    e->_packedDc       = packedDc;
    if (e->_quantBaseError < 0) e->_quantBaseError = 0;

    for (int idx = 0; idx < 64; ++idx)
    {
        e->_quantTableY[idx] =
            (e->_quantBaseError * (float) (jpegQuantTableY[idx]) /
             (float) (jpegQuantTableYMin));
        e->_hquantTableY[idx] = float_to_half (e->_quantTableY[idx]);

        e->_quantTableCbCr[idx] =
            (e->_quantBaseError * (float) (jpegQuantTableCbCr[idx]) /
             (float) (jpegQuantTableCbCrMin));
        e->_hquantTableCbCr[idx] = float_to_half (e->_quantTableCbCr[idx]);
    }

    e->_channel_encode_data[0]    = NULL;
    e->_channel_encode_data[1]    = NULL;
    e->_channel_encode_data[2]    = NULL;
    e->_channel_encode_data_count = 0;

    return EXR_ERR_SUCCESS;
}

/**************************************/

//
// Single channel lossy DCT encoder
//

exr_result_t
LossyDctEncoder_construct (
    LossyDctEncoder*     e,
    float                quantBaseError,
    DctCoderChannelData* rowPtrs,
    uint8_t*             packedAc,
    uint8_t*             packedDc,
    const uint16_t*      toNonlinear,
    int                  width,
    int                  height)
{
    exr_result_t rv;

    rv = LossyDctEncoder_base_construct (
        e, quantBaseError, packedAc, packedDc, toNonlinear, width, height);
    e->_channel_encode_data[0]    = rowPtrs;
    e->_channel_encode_data_count = 1;

    return rv;
}

/**************************************/

//
// RGB channel lossy DCT encoder
//
exr_result_t
LossyDctEncoderCsc_construct (
    LossyDctEncoder*     e,
    float                quantBaseError,
    DctCoderChannelData* rowPtrsR,
    DctCoderChannelData* rowPtrsG,
    DctCoderChannelData* rowPtrsB,
    uint8_t*             packedAc,
    uint8_t*             packedDc,
    const uint16_t*      toNonlinear,
    int                  width,
    int                  height)
{
    exr_result_t rv;

    rv = LossyDctEncoder_base_construct (
        e, quantBaseError, packedAc, packedDc, toNonlinear, width, height);
    e->_channel_encode_data[0]    = rowPtrsR;
    e->_channel_encode_data[1]    = rowPtrsG;
    e->_channel_encode_data[2]    = rowPtrsB;
    e->_channel_encode_data_count = 3;

    return rv;
}

/**************************************/

//
// Precomputing the bit count runs faster than using
// the builtin instruction, at least in one case..
//
// Precomputing 8-bits is no slower than 16-bits,
// and saves a fair bit of overhead..
//

#if USE_POPCOUNT
static inline int
countSetBits (uint16_t src)
{
    return __builtin_popcount (src);
}
#else
// courtesy hacker's delight
static inline int countSetBits(uint32_t x)
{
    uint64_t y;
    y = x * 0x0002000400080010ULL;
    y = y & 0x1111111111111111ULL;
    y = y * 0x1111111111111111ULL;
    y = y >> 60;
    return y;
}
#endif

#if USE_CLZ
static inline int
countLeadingZeros(uint16_t src)
{
    return __builtin_clz (src);
}
#else
// courtesy hacker's delight
static int inline countLeadingZeros( uint32_t x )
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return 32 - countSetBits(x);
}
#endif

//
// Take a DCT coefficient, as well as an acceptable error. Search
// nearby values within the error tolerance, that have fewer
// bits set.
//
// -ffast-math -funsafe-math-optimizations (gcc), fast-math more aggressive
//
// clang -ffp-model=fast (~same as unsafe-math-opts)
//                  aggressive (~-ffast-math)
// -fno-math-errno
// -f[no-]honor-nans, honor-infinities
// -ffp-contract=[on|off|fast]
// -f[no-]associative-math <- can really help with vectorization
// -f[no-]reciprocal-math
//
// #pragma float_control(push|pop)
// #pragma float_control(precise, on|off)
// #pragma clang fp reassociate(on|off)
// #pragma clang fp reciprocal(on|off)
// #pragma STDC_FP_CONTRACT ON|OFF|DEFAULT
//

#define TEST_QUANT_ALTERNATE_LARGE(x)                                   \
    alt = (x);                                                          \
    bits = countSetBits (alt);                                          \
    if (bits < smallbits)                                               \
    {                                                                   \
        delta = half_to_float ((uint16_t)alt) - srcFloat;               \
        if (delta < errTol)                                             \
        {                                                               \
            smallbits = bits; smalldelta = delta; smallest = alt;       \
        }                                                               \
    }                                                                   \
    else if (bits == smallbits)                                         \
    {                                                                   \
        delta = half_to_float ((uint16_t)alt) - srcFloat;               \
        if (delta < smalldelta)                                         \
        {                                                               \
            smallest = alt;                                             \
            smalldelta = delta;                                         \
            smallbits = bits;                                           \
        }                                                               \
    }

#define TEST_QUANT_ALTERNATE_SMALL(x)                                   \
    alt = (x);                                                          \
    bits = countSetBits (alt);                                          \
    if (bits < smallbits)                                               \
    {                                                                   \
        delta = srcFloat - half_to_float ((uint16_t)alt);               \
        if (delta < errTol)                                             \
        {                                                               \
            smallbits = bits; smalldelta = delta; smallest = alt;       \
        }                                                               \
    }                                                                   \
    else if (bits == smallbits)                                         \
    {                                                                   \
        delta = srcFloat - half_to_float ((uint16_t)alt);               \
        if (delta < smalldelta)                                         \
        {                                                               \
            smallest = alt;                                             \
            smalldelta = delta;                                         \
            smallbits = bits;                                           \
        }                                                               \
    }

static uint32_t handleQuantizeDenormTol (
    uint32_t abssrc, uint32_t tolSig, float errTol, float srcFloat)
{
    const uint32_t tsigshift = (32 - countLeadingZeros (tolSig));
    const uint32_t npow2 = (1 << tsigshift);
    const uint32_t lowermask = npow2 - 1;
    const uint32_t mask = ~lowermask;
    const uint32_t mask2 = mask ^ npow2;

    uint32_t alt, smallest = abssrc;
    int bits, smallbits = countSetBits(abssrc);
    float delta, smalldelta = errTol;

    TEST_QUANT_ALTERNATE_SMALL(abssrc & mask2);
    TEST_QUANT_ALTERNATE_SMALL(abssrc & mask);
    TEST_QUANT_ALTERNATE_LARGE((abssrc + npow2) & mask);
    TEST_QUANT_ALTERNATE_LARGE((abssrc + (npow2 << 1)) & mask);

    return smallest;
}

static uint32_t handleQuantizeGeneric (
    uint32_t abssrc, uint32_t tolSig, float errTol, float srcFloat)
{
    // classic would do clz(significand - 1) but here we are trying to
    // construct a mask, so want to ensure for an power of 2, we
    // actually get the next (i.e. 2 returns 4)
    const uint32_t tsigshift = (32 - countLeadingZeros (tolSig));
    const uint32_t npow2 = (1 << tsigshift);
    const uint32_t lowermask = npow2 - 1;
    const uint32_t mask = ~lowermask;
    const uint32_t mask2 = mask ^ npow2;
    const uint32_t srcMaskedVal = abssrc & lowermask;
    const uint32_t extrabit = (tolSig > srcMaskedVal);

    const uint32_t mask3 = mask2 ^ (((npow2 << 1) * (extrabit)) |
                                    ((npow2 >> 1) * (!extrabit)));

    uint32_t alt, smallest = abssrc;
    int bits, smallbits = countSetBits(abssrc);
    float delta, smalldelta = errTol;

    if (extrabit)
    {
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask3);
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask2);

        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask);
    }
    else if ((abssrc & npow2) != 0)
    {
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask2);
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask3);

        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask);
    }
    else
    {
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask2);

        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask);
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask3);
    }
    TEST_QUANT_ALTERNATE_LARGE((abssrc + npow2) & mask);

    return smallest;
}

// use same signature so we can get tail / sibling call optimisation
// (can force with clang?), but notice we are sending in absolute src
// value and the shifted tolerance significand instead
static uint32_t handleQuantizeEqualExp (
    uint32_t abssrc, uint32_t tolSig, float errTol, float srcFloat)
{
    const uint32_t npow2 = 0x0800;
    const uint32_t lowermask = npow2 - 1;
    const uint32_t mask = ~lowermask;
    const uint32_t mask2 = mask ^ npow2;

    const uint32_t srcMaskedVal = abssrc & lowermask;
    const uint32_t extrabit = (tolSig > srcMaskedVal);

    const uint32_t mask3 = mask2 ^ (((npow2 << 1) * (extrabit)) |
                                    ((npow2 >> 1) * (!extrabit)));

    // not yet clear how to narrow down below 3 values...
    uint32_t alt, smallest = abssrc;
    int bits, smallbits = countSetBits(abssrc);
    float delta, smalldelta = errTol;

    // doing in this order mask2, mask, +npow guarantees sorting of values
    // so can avoid a couple of conditionals in the macros
    if (srcMaskedVal == abssrc)
    {
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask3);
    }
    else
    {
        uint32_t alt0 = (abssrc & mask2);
        uint32_t alt1 = (abssrc & mask);
        if (alt0 == alt1) alt0 = (abssrc & mask3);

        TEST_QUANT_ALTERNATE_SMALL(alt0);
        TEST_QUANT_ALTERNATE_SMALL(alt1);
    }
    TEST_QUANT_ALTERNATE_LARGE((abssrc + npow2) & mask);

    return smallest;
}

static uint32_t handleQuantizeCloseExp (
    uint32_t abssrc, uint32_t tolSig, float errTol, float srcFloat)
{
    const uint32_t npow2 = 0x0400;
    const uint32_t lowermask = npow2 - 1;
    const uint32_t mask = ~lowermask;
    const uint32_t mask2 = mask ^ npow2;

    const uint32_t srcMaskedVal = abssrc & lowermask;
    const uint32_t extrabit = (tolSig > srcMaskedVal);

    const uint32_t mask3 = mask2 ^ (((npow2 << 1) * (extrabit)) |
                                    ((npow2 >> 1) * (!extrabit)));

    uint32_t alternates[3];

    if ((abssrc & npow2) == 0) // by definition, src&mask2 == src&mask
    {
        if (extrabit)
        {
            alternates[0] = (abssrc & mask3);
            alternates[1] = (abssrc & mask);
        }
        else
        {
            alternates[0] = (abssrc & mask);
            alternates[1] = (abssrc & mask3);
        }
    }
    else
    {
        if (extrabit)
        {
            alternates[0] = (abssrc & mask3);
            alternates[1] = (abssrc & mask2);
            float alt1delta = srcFloat - half_to_float ((uint16_t)alternates[1]);
            if (alt1delta >= errTol)
            {
                alternates[1] = (abssrc & mask);
            }
        }
        else
        {
            alternates[0] = (abssrc & mask2);
            alternates[1] = (abssrc & mask3);
            float alt0delta = srcFloat - half_to_float ((uint16_t)alternates[0]);
            if (alt0delta >= errTol)
                alternates[0] = (abssrc & mask);
        }
    }
    alternates[2] = ((abssrc + npow2) & mask);

    uint32_t alt, smallest = abssrc;
    int bits, smallbits = countSetBits(abssrc);
    float delta, smalldelta = errTol;

    TEST_QUANT_ALTERNATE_SMALL(alternates[0]);
    TEST_QUANT_ALTERNATE_SMALL(alternates[1]);
    TEST_QUANT_ALTERNATE_LARGE(alternates[2]);

    return smallest;
}

static inline uint32_t handleQuantizeLargerSig (
    uint32_t abssrc, uint32_t npow2, uint32_t mask, float errTol, float srcFloat)
{
    // in this case, only need to test two scenarios:
    //
    // can't fully zero out the masked region, so go to "0.5" of that
    // region and then test the rounded value...
    const uint32_t mask2 = (mask ^ (npow2 | (npow2 >> 1)));

    uint32_t alt0 = (abssrc & mask2);
    uint32_t alt1 = ((abssrc + npow2) & mask);

    int bits0 = countSetBits (alt0);
    int bits1 = countSetBits (alt1);

    float delta;

    if (bits1 < bits0)
    {
        delta = half_to_float ((uint16_t)alt1) - srcFloat; // alt1 >= srcFloat
        // bits1 < bits0 and if ok, just return
        if (delta < errTol)
            return alt1;
        delta = srcFloat - half_to_float ((uint16_t)alt0); // alt0 <= srcFloat
        if (delta < errTol)
            return alt0;
    }
    else if (bits1 == bits0)
    {
        delta = srcFloat - half_to_float ((uint16_t)alt0);
        float delta1 = half_to_float ((uint16_t)alt1) - srcFloat;
        if (delta < errTol)
            return (delta1 < delta) ? alt1 : alt0;

        if (delta1 < errTol)
            return alt1;
    }
    else
    {
        delta = srcFloat - half_to_float ((uint16_t)alt0);
        // bits0 < bits1 so if ok, just return
        if (delta < errTol)
            return alt0;

        // fallback...
        // in this case, alt1 rounding could have made
        // bits1 larger than src, test for that
        int srcbits = countSetBits (abssrc);
        if (bits1 < srcbits)
        {
            delta = half_to_float ((uint16_t)alt1) - srcFloat;
            if (delta < errTol)
                return alt1;
        }
    }
    return abssrc;
}

static inline uint32_t handleQuantizeSmallerSig (
    uint32_t abssrc, uint32_t npow2, uint32_t mask, float errTol, float srcFloat)
{
    // in this case, only need to test two cases:
    //
    // base truncation and rounded truncation
    uint32_t alt0 = (abssrc & mask);
    uint32_t alt1 = ((abssrc + npow2) & mask);

    int bits0 = countSetBits (alt0);
    int bits1 = countSetBits (alt1);

    float delta;

    if (bits1 < bits0)
    {
        delta = half_to_float ((uint16_t)alt1) - srcFloat; // alt1 >= srcFloat
        // bits1 < bits0 and if ok, just return
        if (delta < errTol)
            return alt1;
        delta = srcFloat - half_to_float ((uint16_t)alt0); // alt0 <= srcFloat
        if (delta < errTol)
            return alt0;
    }
    else if (bits1 == bits0)
    {
        delta = srcFloat - half_to_float ((uint16_t)alt0);
        float delta1 = half_to_float ((uint16_t)alt1) - srcFloat;
        if (delta < errTol)
            return (delta1 < delta) ? alt1 : alt0;

        if (delta1 < errTol)
            return alt1;
    }
    else
    {
        delta = srcFloat - half_to_float ((uint16_t)alt0);
        // bits0 < bits1 so if ok, just return
        if (delta < errTol)
            return alt0;

        // fallback...
        // in this case, alt1 rounding could have made
        // bits1 larger than src, test for that
        int srcbits = countSetBits (abssrc);
        if (bits1 < srcbits)
        {
            delta = half_to_float ((uint16_t)alt1) - srcFloat;
            if (delta < errTol)
                return alt1;
        }
    }
    return abssrc;
}

static inline uint32_t handleQuantizeEqualSig (
    uint32_t abssrc, uint32_t npow2, uint32_t mask, float errTol, float srcFloat)
{
    // 99.99% of the time, mask is the best choice but for a very few
    // 16-bit float to 32-bit float where even though the significands
    // of the shifted tolerance we will need mask2, so have a
    // different implementation than the basic choose 2 of the larger
    // / smaller cases
    uint32_t alt0 = (abssrc & mask);
    uint32_t alt1 = ((abssrc + npow2) & mask);

    // this costs us not much extra if it works (99.99% of the
    // time) as we would compute this immediately assuming
    // the mask almost always makes the bits smaller...
    float delta0 = srcFloat - half_to_float ((uint16_t)alt0);
    if (delta0 >= errTol)
    {
        const uint32_t mask2 = (mask ^ (npow2 | (npow2 >> 1)));

        alt0 = (abssrc & mask2);
        delta0 = srcFloat - half_to_float ((uint16_t)alt0);

        // avoid a re-check against the tolerance below...
        if (delta0 >= errTol)
        {
            float delta1 = half_to_float ((uint16_t)alt1) - srcFloat;
            if (delta1 < errTol)
            {
                int bits1 = countSetBits (alt1);
                int srcbits = countSetBits (abssrc);
                if (bits1 < srcbits)
                    return alt1;
            }
            return abssrc;
        }
    }

    int bits0 = countSetBits (alt0);
    int bits1 = countSetBits (alt1);

    // bits0 is either the same as src (i.e. mask didn't mask any bits)
    // or smaller than src, so do not need to check against that
    //
    // bits1 because we add npow2 may not actually end up smaller...
    if (bits1 < bits0)
    {
        float delta1 = half_to_float ((uint16_t)alt1) - srcFloat;
        // bits1 < bits0 and if ok, just return
        if (delta1 < errTol) return alt1;
    }
    else if (bits1 == bits0)
    {
        float delta1 = half_to_float ((uint16_t)alt1) - srcFloat;
        if (delta1 < delta0) return alt1;
    }

    // bits0 < bits1 and ok or alt1 failed
    return alt0;
}

static uint32_t handleQuantizeDefault (
    uint32_t abssrc, uint32_t tolSig, float errTol, float srcFloat)
{
    // classic would do clz(significand - 1) but here we are trying to
    // construct a mask, so want to ensure for an power of 2, we
    // actually get the next (i.e. 2 returns 4)
    const uint32_t tsigshift = (32 - countLeadingZeros (tolSig));
    const uint32_t npow2 = (1 << tsigshift);
    const uint32_t lowermask = npow2 - 1;
    const uint32_t mask = ~lowermask;
    const uint32_t srcMaskedVal = abssrc & lowermask;

    if (srcMaskedVal > tolSig)
        return handleQuantizeLargerSig (abssrc, npow2, mask, errTol, srcFloat);
    else if (srcMaskedVal < tolSig)
        return handleQuantizeSmallerSig (abssrc, npow2, mask, errTol, srcFloat);

    return handleQuantizeEqualSig (abssrc, npow2, mask, errTol, srcFloat);
}

static uint16_t algoQuantize (
    uint32_t src, uint32_t herrTol, float errTol, float srcFloat)
{
    uint32_t sign = src & 0x8000;
    uint32_t abssrc = src & 0x7FFF;

    srcFloat = fabsf(srcFloat);

    uint32_t srcExpBiased = src & 0x7C00;
    uint32_t tolExpBiased = herrTol & 0x7C00;

    // if nan / inf, just bail and return src
    if (srcExpBiased == 0x7C00)
        return src;

    // can't possibly beat 0 bits
    if (srcFloat < errTol)
        return 0;

    uint32_t expDiff = (srcExpBiased - tolExpBiased) >> 10;
    uint32_t tolSig = (((herrTol & 0x3FF) | (1 << 10)) >> expDiff);

    if (tolExpBiased == 0)
    {
        if (expDiff == 0 || expDiff == 1)
        {
            tolSig = (herrTol & 0x3FF);
            if (tolSig == 0)
                return src;
            return sign | handleQuantizeGeneric (abssrc, tolSig, errTol, srcFloat);
        }

        tolSig = (herrTol & 0x3FF);
        if (tolSig == 0)
            return src;

        tolSig >>= expDiff;
        if (tolSig == 0)
            tolSig = 1;

        return sign | handleQuantizeDenormTol (abssrc, tolSig, errTol, srcFloat);
    }

    if (tolSig == 0)
        return src;

    // we want to try to find a number that has the fewest bits that
    // is within the specified tolerance. To do so without a lookup
    // table, we shift (multiply if you will) the tolerance to be
    // within the same exponent range as the source value.
    //
    // we then need to consider a few bit scenarios
    //
    //
    // all b bits should be preserved (or the delta would be too large)
    // all a bits should be discarded (0'ed)
    // s (first bit where the significand of the tolerance is on)
    // p (next power of 2 above s - npow2 above)
    // x (an extra bit above power of 2)
    //
    // we need to consider a bit mask of:
    // ..bbbbxps aaaaa..
    // ..bbbb110 00000.. (mask as above)
    // ..bbbb100 00000..
    // ..bbbb101 00000..
    // ..bbbb000 00000..
    // ..bbbb001 00000.. (mutually exclusive with previous)
    // ..bbbb1+0 00000.. (add 1 at p bit, then same mask as first case)
    //
    // so if you collapse the mutually exclusive one, that gives 5
    // choices, although they don't always apply, so only need 4 with
    // a bit of conditional tests:
    //
    // uint16_t inexp = (npow2 > 0x0200);
    // uint16_t mask2 = (mask ^ npow2) * inexp;
    // uint16_t extrabit = (tolSig > srcMaskedVal);
    // uint16_t mask3 = (mask ^ npow2);
    // mask3 ^= ((npow2 << 1) * extrabit);
    // mask3 |= ((npow2 >> 1) * (! extrabit));
    //
    // (src & mask)
    // (src & mask2)
    // (src & mask3)
    // ((src + npow2) & mask);
    //
    // So one of those 4 is always one of our choices, but just
    // blindly computing all 4 is about as expensive as doing the
    // table lookup and having a sorted set by number of bits to
    // return the first from, so we're not done yet.
    //
    // however, if the tolerance is small relative to the source, can reduce
    // the ones we need to test, as some of these are always invalid when in
    // that scenario:
    //
    // if the portion of the source significand is larger than the tolerance
    // significand, we can't simply truncate, or would be out of range of the
    // tolerance, so only left with 2 scenarios:
    //
    // ..bbbb101 00000.. (truncation with preservation of sig bit for "0.5")
    // ..bbbb1+0 00000.. (add 1 to round up, then same mask)
    //
    // if the portion of the source significand is strictly less than
    // the tolerance significand, we can do the base truncation and
    // the "round up" may still be within the tolerance, but the
    // deeper truncations will be out of range, so again only need 2
    // (but different 2):
    //
    // ..bbbb110 00000.. (mask as above)
    // ..bbbb1+0 00000.. (add 1 to round up, then same mask)
    //
    // if the significand is equal to the tolerance, depending on the
    // translation back to 32 bit to compare against the 32 bit
    // tolerance (if we could make that a 16-bit value, we could make
    // different decisions), we have to test all 3 of those values:
    //
    // ..bbbb101 00000.. (truncation with preservation of sig bit for "0.5")
    // ..bbbb110 00000.. (mask as above)
    // ..bbbb1+0 00000.. (add 1 to round up, then same mask)
    //
    // 99.99% of the time, the mask or round will be a good choice,
    // but only in a few combinations of tolerance will the truncation
    // be needed because the mask truncation will be out of range,
    // which is not surprising given we're just shifting the
    // significand of the half-float tolerance, where the tolerance is
    // against the original 32-bit value, but can be quickly tested
    // and swapped for fewer comparisons than testing all 3 values
    //
    // when the exponent of the tolerance is close to the value of
    // src, it is a bit harder to reason about, as the masking
    // operations will be changing the exponent, and maybe preserving
    // 1 bit of the significand. for example, when the two numbers are
    // within the same exponent, it is ok to reduce to just 3 values:
    //
    // ..bbbb100 00000.. (mask2 as above)
    // ..bbbb110 00000.. (mask as above)
    // ..bbbb1+0 00000.. (add 1 to round up, then same mask)
    //
    // However that does not handle all scenarios for all (expected)
    // tolerances, so a few other cased are contemplated in each
    // scenario

    // first, handle the default case of a 'large' diff between src
    // and tolerance, or a denorm src
    if (expDiff > 1 || srcExpBiased == 0)
        return sign | handleQuantizeDefault (abssrc, tolSig, errTol, srcFloat);

    if (expDiff == 0)
        return sign | handleQuantizeEqualExp (abssrc, tolSig, errTol, srcFloat);
    return sign | handleQuantizeCloseExp (abssrc, tolSig, errTol, srcFloat);
}

static void
quantizeCoeffAndZigXDR (
    uint16_t* restrict    halfZigCoeff,
    const float* restrict dctvals,
    const float* restrict tolerances,
    const uint16_t* restrict halftols)
{
    //static const int remap[] = {
    //    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    //    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    //    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    //    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
    // inv_remap computed from original zigzag lookup remap so we can
    // deposit into a random destination instead of pulling from a source
    // and save a temporary and loop
    static const int inv_remap[] = {
        0, 1, 5, 6, 14, 15, 27, 28, 2, 4, 7, 13, 16, 26, 29, 42,
        3, 8, 12, 17, 25, 30, 41, 43, 9, 11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63};

    // manually unrolling seems to help on at least x86
    for ( int i = 0; i < 64; i += 4 )
    {
        uint16_t       src0     = float_to_half (dctvals[i+0]);
        uint16_t       src1     = float_to_half (dctvals[i+1]);
        uint16_t       src2     = float_to_half (dctvals[i+2]);
        uint16_t       src3     = float_to_half (dctvals[i+3]);
        const float    errTol0  = tolerances[i+0];
        const float    errTol1  = tolerances[i+1];
        const float    errTol2  = tolerances[i+2];
        const float    errTol3  = tolerances[i+3];
        const uint16_t herrTol0 = halftols[i+0];
        const uint16_t herrTol1 = halftols[i+1];
        const uint16_t herrTol2 = halftols[i+2];
        const uint16_t herrTol3 = halftols[i+3];
        src0 = algoQuantize (src0, herrTol0, errTol0, half_to_float (src0));
        src1 = algoQuantize (src1, herrTol1, errTol1, half_to_float (src1));
        src2 = algoQuantize (src2, herrTol2, errTol2, half_to_float (src2));
        src3 = algoQuantize (src3, herrTol3, errTol3, half_to_float (src3));

        halfZigCoeff[inv_remap[i+0]] = one_from_native16 (src0);
        halfZigCoeff[inv_remap[i+1]] = one_from_native16 (src1);
        halfZigCoeff[inv_remap[i+2]] = one_from_native16 (src2);
        halfZigCoeff[inv_remap[i+3]] = one_from_native16 (src3);
    }
//    for ( int i = 0; i < 64; ++i )
//    {
//        uint16_t       src     = float_to_half (dctvals[i]);
//        const float    errTol  = tolerances[i];
//        const uint16_t herrTol = halftols[i];
//        src = algoQuantize (src, herrTol, errTol, half_to_float (src));
//        halfZigCoeff[inv_remap[i]] = one_from_native16 (src);
//    }
}

/**************************************/

//
// Given three channels of source data, encoding by first applying
// a color space conversion to a YCbCr space.  Otherwise, if we only
// have one channel, just encode it as is.
//
// Other numbers of channels are somewhat unexpected at this point
//
exr_result_t
LossyDctEncoder_execute (
    void* (*alloc_fn) (size_t), void (*free_fn) (void*), LossyDctEncoder* e)
{
    int                  numComp = e->_channel_encode_data_count;
    DctCoderChannelData* chanData[3];

    int numBlocksX = (int) (ceilf ((float) e->_width / 8.0f));
    int numBlocksY = (int) (ceilf ((float) e->_height / 8.0f));

    uint16_t halfZigCoef[64];

    uint16_t* currAcComp            = (uint16_t*) e->_packedAc;
    size_t    tmpHalfBufferElements = 0;
    uint16_t* tmpHalfBuffer         = NULL;
    uint16_t* tmpHalfBufferPtr      = NULL;

    e->_numAcComp = 0;
    e->_numDcComp = 0;

    //
    // Allocate a temp half buffer to quantize into for
    // any FLOAT source channels.
    //

    for (int chan = 0; chan < numComp; ++chan)
    {
        chanData[chan] = e->_channel_encode_data[chan];
        if (chanData[chan]->_type == EXR_PIXEL_FLOAT)
            tmpHalfBufferElements += (size_t) e->_width * (size_t) e->_height;
    }

    if (tmpHalfBufferElements)
    {
        tmpHalfBuffer =
            (uint16_t*) alloc_fn (tmpHalfBufferElements * sizeof (uint16_t));
        if (!tmpHalfBuffer) return EXR_ERR_OUT_OF_MEMORY;
        tmpHalfBufferPtr = tmpHalfBuffer;
    }

    //
    // Run over all the float scanlines, quantizing,
    // and re-assigning _rowPtr[y]. We need to translate
    // FLOAT XDR to HALF XDR.
    //

    for (int chan = 0; chan < numComp; ++chan)
    {
        if (chanData[chan]->_type != EXR_PIXEL_FLOAT) continue;

        for (int y = 0; y < e->_height; ++y)
        {
            const float* srcXdr = (const float*) chanData[chan]->_rows[y];

            for (int x = 0; x < e->_width; ++x)
            {
                //
                // Clamp to half ranges, instead of just casting. This
                // avoids introducing Infs which end up getting zeroed later
                //
                float src = one_to_native_float (srcXdr[x]);
                if (src > 65504.f)
                    src = 65504.f;
                else if (src < -65504.f)
                    src = -65504.f;
                tmpHalfBufferPtr[x] = one_from_native16 (float_to_half (src));
            }

            chanData[chan]->_rows[y] = (uint8_t*) tmpHalfBufferPtr;
            tmpHalfBufferPtr += e->_width;
        }
    }

    //
    // Pack DC components together by common plane, so we can get
    // a little more out of differencing them. We'll always have
    // one component per block, so we can computed offsets.
    //

    chanData[0]->_dc_comp = (uint16_t*) e->_packedDc;
    for (int chan = 1; chan < numComp; ++chan)
        chanData[chan]->_dc_comp =
            chanData[chan - 1]->_dc_comp + numBlocksX * numBlocksY;

    for (int blocky = 0; blocky < numBlocksY; ++blocky)
    {
        for (int blockx = 0; blockx < numBlocksX; ++blockx)
        {
            uint16_t              h;
            const float* restrict quantTable;
            const uint16_t* restrict hquantTable;

            for (int chan = 0; chan < numComp; ++chan)
            {
                //
                // Break the source into 8x8 blocks. If we don't
                // fit at the edges, mirror.
                //
                // Also, convert from linear to nonlinear representation.
                // Our source is assumed to be XDR, and we need to convert
                // to NATIVE prior to converting to float.
                //
                // If we're converting linear -> nonlinear, assume that the
                // XDR -> NATIVE conversion is built into the lookup. Otherwise,
                // we'll need to explicitly do it.
                //

                for (int y = 0; y < 8; ++y)
                {
                    for (int x = 0; x < 8; ++x)
                    {
                        int vx = 8 * blockx + x;
                        int vy = 8 * blocky + y;

                        if (vx >= e->_width)
                            vx = e->_width - (vx - (e->_width - 1));

                        if (vx < 0) vx = e->_width - 1;

                        if (vy >= e->_height)
                            vy = e->_height - (vy - (e->_height - 1));

                        if (vy < 0) vy = e->_height - 1;

                        h = ((const uint16_t*) (chanData[chan]->_rows)[vy])[vx];

                        if (e->_toNonlinear) { h = e->_toNonlinear[h]; }
                        else { h = one_to_native16 (h); }

                        chanData[chan]->_dctData[y * 8 + x] = half_to_float (h);
                    } // x
                }     // y
            }         // chan

            //
            // Color space conversion
            //

            if (numComp == 3)
            {
                csc709Forward64 (
                    chanData[0]->_dctData,
                    chanData[1]->_dctData,
                    chanData[2]->_dctData);
            }

            quantTable = e->_quantTableY;
            hquantTable = e->_hquantTableY;
            for (int chan = 0; chan < numComp; ++chan)
            {
                //
                // Forward DCT
                //
                dctForward8x8 (chanData[chan]->_dctData);

                //
                // Quantize to half, zigzag, and convert to XDR
                //
                quantizeCoeffAndZigXDR (halfZigCoef, chanData[chan]->_dctData,
                                        quantTable, hquantTable);

                //
                // Save the DC component separately, to be compressed on
                // its own.
                //

                *(chanData[chan]->_dc_comp)++ = halfZigCoef[0];
                e->_numDcComp++;

                //
                // Then RLE the AC components (which will record the count
                // of the resulting number of items)
                //

                LossyDctEncoder_rleAc (e, halfZigCoef, &currAcComp);
                quantTable = e->_quantTableCbCr;
                hquantTable = e->_hquantTableCbCr;
            } // chan
        }     // blockx
    }         // blocky

    if (tmpHalfBuffer) free_fn (tmpHalfBuffer);

    return EXR_ERR_SUCCESS;
}

/**************************************/

//
// RLE the zig-zag of the AC components + copy over
// into another tmp buffer
//
// Try to do a simple RLE scheme to reduce run's of 0's. This
// differs from the jpeg EOB case, since EOB just indicates that
// the rest of the block is zero. In our case, we have lots of
// NaN symbols, which shouldn't be allowed to occur in DCT
// coefficients - so we'll use them for encoding runs.
//
// If the high byte is 0xff, then we have a run of 0's, of length
// given by the low byte. For example, 0xff03 would be a run
// of 3 0's, starting at the current location.
//
// block is our block of 64 coefficients
// acPtr a pointer to back the RLE'd values into.
//
// This will advance the counter, _numAcComp.
//

void
LossyDctEncoder_rleAc (LossyDctEncoder* e, uint16_t* block, uint16_t** acPtr)
{
    int       dctComp   = 1;
    uint16_t  rleSymbol = 0x0;
    uint16_t* curAC     = *acPtr;

    while (dctComp < 64)
    {
        uint16_t runLen = 1;

        //
        // If we don't have a 0, output verbatim
        //

        if (block[dctComp] != rleSymbol)
        {
            *curAC++ = block[dctComp];
            e->_numAcComp++;

            dctComp += runLen;
            continue;
        }

        //
        // We're sitting on a 0, so see how big the run is.
        //

        while ((dctComp + runLen < 64) &&
               (block[dctComp + runLen] == rleSymbol))
        {
            runLen++;
        }

        //
        // If the run len is too small, just output verbatim
        // otherwise output our run token
        //
        // Originally, we wouldn't have a separate symbol for
        // "end of block". But in some experimentation, it looks
        // like using 0xff00 for "end of block" can save a bit
        // of space.
        //

        if (runLen == 1)
        {
            runLen   = 1;
            *curAC++ = block[dctComp];
            e->_numAcComp++;

            //
            // Using 0xff00 for "end of block"
            //
        }
        else if (runLen + dctComp == 64)
        {
            //
            // Signal EOB
            //

            *curAC++ = 0xff00;
            e->_numAcComp++;
        }
        else
        {
            //
            // Signal normal run
            //

            *curAC++ = (uint16_t) 0xff00 | runLen;
            e->_numAcComp++;
        }

        //
        // Advance by runLen
        //

        dctComp += runLen;
    }
    *acPtr = curAC;
}
