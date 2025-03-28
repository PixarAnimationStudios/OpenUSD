//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "nanocolor.h"
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#ifdef __SSE2__
#include <xmmintrin.h>
#include <smmintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

// Internal data structure to hold computed color space data, and the initial
// decsriptor.
struct NcColorSpace {
    NcColorSpaceDescriptor desc;
    float K0, phi;
    NcM33f rgbToXYZ;
};

static void _InitColorSpace(NcColorSpace* cs);

static float _FromLinear(const NcColorSpace* cs, float t) {
    const float gamma = cs->desc.gamma;
    if (t < cs->K0 / cs->phi)
        return t * cs->phi;
    const float a = cs->desc.linearBias;
    return (1.f + a) * powf(t, 1.f / gamma) - a;
}

static float _ToLinear(const NcColorSpace* cs, float t) {
    const float gamma = cs->desc.gamma;
    if (t < cs->K0)
        return t / cs->phi;
    const float a = cs->desc.linearBias;
    return powf((t + a) / (1.f + a), gamma);
}

static const char _g22_adobergb[] = "g22_adobergb_scene";
static const char _g18_rec709[] = "g18_rec709_scene";
static const char _g22_ap1[] = "g22_ap1_scene";
static const char _g22_rec709[] = "g22_rec709_scene";
static const char _identity[] = "identity";
static const char _lin_adobergb[] = "lin_adobergb_scene";
static const char _lin_ap0[] = "lin_ap0_scene";
static const char _lin_ap1[] = "lin_ap1_scene";
static const char _lin_p3d65[] = "lin_p3d65_scene";
static const char _lin_rec709[] = "lin_rec709_scene";
static const char _lin_rec2020[] = "lin_rec2020_scene";
static const char _raw[] = "raw";
static const char _srgb_p3d65[] = "srgb_p3d65_scene";
static const char _srgb_rec709[] = "srgb_rec709_scene";

NCAPI const char* NcGetDescription(const NcColorSpace* cs) {
    if (!cs)
        return NULL;

    if (!strcmp(cs->desc.name, _lin_ap1))
        return "Academy Color Encoding System (ACEScg), a color space designed for computer graphics.";
    if (!strcmp(cs->desc.name, _g22_adobergb))
        return "Adobe RGB (1998), a color space developed by Adobe Systems.";
    if (!strcmp(cs->desc.name, _g18_rec709))
        return "Gamma 1.8, primaries from Rec. 709, white point from D65.";
    if (!strcmp(cs->desc.name, _g22_ap1))
        return "Gamma 2.2, primaries from ACEScg, white point from ACEScg.";
    if (!strcmp(cs->desc.name, _g22_rec709))
        return "Gamma 2.2, primaries from Rec. 709, white point from D65.";
    if (!strcmp(cs->desc.name, _identity))
        return "Identity color space, no conversion.";
    if (!strcmp(cs->desc.name, _lin_adobergb))
        return "Linear Adobe RGB (1998), a color space developed by Adobe Systems.";
    if (!strcmp(cs->desc.name, _lin_ap0))
        return "Linear transfer, ACES 2065-1.";
    if (!strcmp(cs->desc.name, _lin_ap1))
        return "Linear transfer, ACESCg.";
    if (!strcmp(cs->desc.name, _lin_p3d65))
        return "Linear Display P3, a color space using the Display P3 primaries.";
    if (!strcmp(cs->desc.name, _lin_rec709))
        return "Linear Rec. 709, a color space using the Rec. 709 primaries.";
    if (!strcmp(cs->desc.name, _lin_rec2020))
        return "Linear Rec. 2020, a color space using the Rec. 2020 primaries.";
    if (!strcmp(cs->desc.name, _lin_rec709))
        return "Linear sRGB, a color space using the sRGB primaries.";
    if (!strcmp(cs->desc.name, _raw))
        return "Raw color space, no conversion.";
    if (!strcmp(cs->desc.name, _srgb_p3d65))
        return "sRGB Display P3, a color space using the Display P3 primaries.";
    if (!strcmp(cs->desc.name, _srgb_rec709))
        return "sRGB, a display color space developed by HP and Microsoft.";
    return cs->desc.name;
}

// White point chromaticities.
#define _WpD65 { 0.3127, 0.3290 }
#define _WpACES { 0.32168, 0.33767 }

static NcColorSpace _colorSpaces[] = {
    {
        _g22_adobergb,
        { 0.64, 0.33 },
        { 0.21, 0.71 },
        { 0.15, 0.06 },
        _WpD65,
        563.0/256.0, // 2.19921875
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _g22_ap1,
        { 0.713, 0.293 },
        { 0.165, 0.830 },
        { 0.128, 0.044 },
        _WpACES,
        2.2,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _g18_rec709,
        { 0.640, 0.330 },
        { 0.300, 0.600 },
        { 0.150, 0.060 },
        _WpD65,
        1.8,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _g22_rec709,
        { 0.640, 0.330 },
        { 0.300, 0.600 },
        { 0.150, 0.060 },
        _WpD65,
        2.2,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _lin_adobergb,
        { 0.64, 0.33 },
        { 0.21, 0.71 },
        { 0.15, 0.06 },
        _WpD65,
        1.0,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _lin_ap0,
        { 0.7347, 0.2653  },
        { 0.0000, 1.0000  },
        { 0.0001, -0.0770 },
        _WpACES,
        1.0,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _lin_ap1,                      // same primaries and wp as acescg
        { 0.713, 0.293 },
        { 0.165, 0.830 },
        { 0.128, 0.044 },
        _WpACES,
        1.0,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _lin_p3d65,
        { 0.6800, 0.3200 },
        { 0.2650, 0.6900 },
        { 0.1500, 0.0600 },
        _WpD65,
        1.0,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _lin_rec709,
        { 0.640, 0.330 },
        { 0.300, 0.600 },
        { 0.150, 0.060 },
        _WpD65,
        1.0,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _lin_rec2020,
        { 0.708, 0.292 },
        { 0.170, 0.797 },
        { 0.131, 0.046 },
        _WpD65,
        1.0,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _srgb_p3d65,
        { 0.6800, 0.3200 },
        { 0.2650, 0.6900 },
        { 0.1500, 0.0600 },
        _WpD65,
        2.4,
        0.055,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _srgb_rec709,
        { 0.640, 0.330 },
        { 0.300, 0.600 },
        { 0.150, 0.060 },
        _WpD65,
        2.4,
        0.055,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _identity,
        { 1.0, 0.0 }, // these chromaticities generate identity
        { 0.0, 1.0 },
        { 0.0, 0.0 },
        { 1.0/3.0, 1.0/3.0 },
        1.0,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    },
    {
        _raw,
        { 1.0, 0.0 }, // these chromaticities generate identity
        { 0.0, 1.0 },
        { 0.0, 0.0 },
        { 1.0/3.0, 1.0/3.0 },
        1.0,
        0.0,
        0, 0,
        { 0,0,0, 0,0,0, 0,0,0 }
    }
};

bool NcColorSpaceEqual(const NcColorSpace* cs1, const NcColorSpace* cs2) {
    if (!cs1 || !cs2) {
        return false;
    }

    if (cs1->desc.name == NULL || cs2->desc.name == NULL) {
        return false;
    }

    if (strcmp(cs1->desc.name, cs1->desc.name) != 0) {
        return false;
    }

    // compare color transform op matrix and transfer op
    for (int i = 0; i < 9; i++) {
        if (fabsf(cs1->rgbToXYZ.m[i] - cs2->rgbToXYZ.m[i]) > 1e-5f) {
            return false;
        }
    }
    if (fabsf(cs1->desc.gamma - cs2->desc.gamma) > 1e-3f) {
        return false;
    }
    if (fabsf(cs1->desc.linearBias - cs2->desc.linearBias) > 1e-3f) {
        return false;
    }
    return true;
}

static NcM33f _M3ffInvert(NcM33f m) {
    NcM33f inv;
    const int M0 = 0, M1 = 3, M2 = 6, M3 = 1, M4 = 4, M5 = 7, M6 = 2, M7 = 5, M8 = 8;
    float det = m.m[M0] * (m.m[M4] * m.m[M8] - m.m[M5] * m.m[M7]) -
    m.m[M1] * (m.m[M3] * m.m[M8] - m.m[M5] * m.m[M6]) +
    m.m[M2] * (m.m[M3] * m.m[M7] - m.m[M4] * m.m[M6]);
    float invdet = 1.0 / det;
    inv.m[M0] = (m.m[M4] * m.m[M8] - m.m[M5] * m.m[M7]) * invdet;
    inv.m[M1] = (m.m[M2] * m.m[M7] - m.m[M1] * m.m[M8]) * invdet;
    inv.m[M2] = (m.m[M1] * m.m[M5] - m.m[M2] * m.m[M4]) * invdet;
    inv.m[M3] = (m.m[M5] * m.m[M6] - m.m[M3] * m.m[M8]) * invdet;
    inv.m[M4] = (m.m[M0] * m.m[M8] - m.m[M2] * m.m[M6]) * invdet;
    inv.m[M5] = (m.m[M2] * m.m[M3] - m.m[M0] * m.m[M5]) * invdet;
    inv.m[M6] = (m.m[M3] * m.m[M7] - m.m[M4] * m.m[M6]) * invdet;
    inv.m[M7] = (m.m[M1] * m.m[M6] - m.m[M0] * m.m[M7]) * invdet;
    inv.m[M8] = (m.m[M0] * m.m[M4] - m.m[M1] * m.m[M3]) * invdet;
    return inv;
}

static NcM33f _M33fMultiply(NcM33f lh, NcM33f rh) {
    NcM33f m;
    m.m[0] = lh.m[0] * rh.m[0] + lh.m[1] * rh.m[3] + lh.m[2] * rh.m[6];
    m.m[1] = lh.m[0] * rh.m[1] + lh.m[1] * rh.m[4] + lh.m[2] * rh.m[7];
    m.m[2] = lh.m[0] * rh.m[2] + lh.m[1] * rh.m[5] + lh.m[2] * rh.m[8];
    m.m[3] = lh.m[3] * rh.m[0] + lh.m[4] * rh.m[3] + lh.m[5] * rh.m[6];
    m.m[4] = lh.m[3] * rh.m[1] + lh.m[4] * rh.m[4] + lh.m[5] * rh.m[7];
    m.m[5] = lh.m[3] * rh.m[2] + lh.m[4] * rh.m[5] + lh.m[5] * rh.m[8];
    m.m[6] = lh.m[6] * rh.m[0] + lh.m[7] * rh.m[3] + lh.m[8] * rh.m[6];
    m.m[7] = lh.m[6] * rh.m[1] + lh.m[7] * rh.m[4] + lh.m[8] * rh.m[7];
    m.m[8] = lh.m[6] * rh.m[2] + lh.m[7] * rh.m[5] + lh.m[8] * rh.m[8];
    return m;
}

static void _InitColorSpace(NcColorSpace* cs) {
    if (!cs || cs->rgbToXYZ.m[8] != 0.0)
        return;
        
    const float a = cs->desc.linearBias;
    const float gamma = cs->desc.gamma;
    
    if (gamma == 1.f) {
        cs->K0 = 1.e9f;
        cs->phi = 1.f;
    }
    else {
        if (a <= 0.f) {
            cs->K0 = 0.f;
            cs->phi = 1.f;
        }
        else {
            cs->K0 = a / (gamma - 1.f);
            cs->phi = (a /
                       expf(logf(gamma * a /
                       (gamma + gamma * a - 1.f - a)) * gamma)) /
                       (gamma - 1.f);
        }
    }
    
    // if the primaries are zero, the cs was defined by the 3x3 matrix, don't overwrite it.
    if (cs->desc.whitePoint.x == 0.f)
        return;
    
    NcM33f m;
    // To be consistent, we use SMPTE RP 177-1993
    // compute xyz [little xyz]
    float red[3]   = { cs->desc.redPrimary.x,
                       cs->desc.redPrimary.y,
                       1.f - cs->desc.redPrimary.x - cs->desc.redPrimary.y };
    
    float green[3] = { cs->desc.greenPrimary.x,
                       cs->desc.greenPrimary.y,
                       1.f - cs->desc.greenPrimary.x - cs->desc.greenPrimary.y };
    
    float blue[3]  = { cs->desc.bluePrimary.x,
                       cs->desc.bluePrimary.y,
                       1.f - cs->desc.bluePrimary.x - cs->desc.bluePrimary.y };
    
    float white[3] = { cs->desc.whitePoint.x,
                       cs->desc.whitePoint.y,
                       1.f - cs->desc.whitePoint.x - cs->desc.whitePoint.y };
    
    // Build the P matrix by column binding red, green, and blue:
    m.m[0] = red[0];
    m.m[1] = green[0];
    m.m[2] = blue[0];
    m.m[3] = red[1];
    m.m[4] = green[1];
    m.m[5] = blue[1];
    m.m[6] = red[2];
    m.m[7] = green[2];
    m.m[8] = blue[2];
    
    // and W
    // white has luminance factor of 1.0, ie Y = 1
    float W[3] = { white[0] / white[1], white[1] / white[1], white[2] / white[1] };
    
    // compute the coefficients to scale primaries
    NcM33f mInv = _M3ffInvert(m);
    float C[3] = {
        mInv.m[0] * W[0] + mInv.m[1] * W[1] + mInv.m[2] * W[2],
        mInv.m[3] * W[0] + mInv.m[4] * W[1] + mInv.m[5] * W[2],
        mInv.m[6] * W[0] + mInv.m[7] * W[1] + mInv.m[8] * W[2]
    };
    
    // multiply the P matrix by the diagonal matrix of coefficients
    m.m[0] *= C[0];
    m.m[1] *= C[1];
    m.m[2] *= C[2];
    m.m[3] *= C[0];
    m.m[4] *= C[1];
    m.m[5] *= C[2];
    m.m[6] *= C[0];
    m.m[7] *= C[1];
    m.m[8] *= C[2];
    
    cs->rgbToXYZ = m;
}

void NcInitColorSpaceLibrary(void) {
    for (size_t i = 0; i < sizeof(_colorSpaces) / sizeof(_colorSpaces[0]); i++) {
        _InitColorSpace(&_colorSpaces[i]);
    }
}

const NcColorSpace* NcCreateColorSpace(const NcColorSpaceDescriptor* csd) {
    if (!csd)
        return NULL;
    
    NcColorSpace* cs = (NcColorSpace*) calloc(1, sizeof(*cs));
    cs->desc = *csd;
    cs->desc.name = strdup(csd->name);
    _InitColorSpace(cs);
    return cs;
}

const NcColorSpace* NcCreateColorSpaceM33(const NcColorSpaceM33Descriptor* csd, 
                                          bool* matrixIsNormalized) {
    if (!csd)
        return NULL;
    
    NcColorSpace* cs = (NcColorSpace*) calloc(1, sizeof(*cs));
    cs->desc.name = strdup(csd->name);
    cs->desc.gamma = csd->gamma;
    cs->desc.linearBias = csd->linearBias;
    cs->rgbToXYZ = csd->rgbToXYZ;
    _InitColorSpace(cs);

    // fill in the assumed chromaticities
    NcXYZ whiteXYZ = NcRGBToXYZ(cs, (NcRGB){ 1, 1, 1 });
    NcYxy whiteYxy = NcXYZToYxy(whiteXYZ);
    NcXYZ redXYZ = NcRGBToXYZ(cs, (NcRGB){ 1, 0, 0 });
    NcXYZ greenXYZ = NcRGBToXYZ(cs, (NcRGB){ 0, 1, 0 });
    NcXYZ blueXYZ = NcRGBToXYZ(cs, (NcRGB){ 0, 0, 1 });
    NcYxy redYxy = NcXYZToYxy(redXYZ);
    NcYxy greenYxy = NcXYZToYxy(greenXYZ);
    NcYxy blueYxy = NcXYZToYxy(blueXYZ);

    // if the Y components are not close to one, the matrix was not a normalized
    // primary matrix; report that if requested.
    if (matrixIsNormalized)
        *matrixIsNormalized = (fabsf(redYxy.Y - 1) < 1e-3f &&
                               fabsf(greenYxy.Y - 1) < 1e-3f &&
                               fabsf(blueYxy.Y - 1) < 1e-3f &&
                               fabsf(whiteYxy.Y - 1) < 1e-3f);

    cs->desc.redPrimary = (NcChromaticity) { redYxy.x, redYxy.y };
    cs->desc.greenPrimary = (NcChromaticity) { greenYxy.x, greenYxy.y };
    cs->desc.bluePrimary = (NcChromaticity) { blueYxy.x, blueYxy.y };
    cs->desc.whitePoint = (NcChromaticity) { whiteYxy.x, whiteYxy.y };
    return cs;
}

void NcFreeColorSpace(const NcColorSpace* cs) {
    if (!cs)
        return;

    // don't free the built in color spaces
    for (size_t i = 0; i < sizeof(_colorSpaces) / sizeof(_colorSpaces[0]); i++) {
        if (cs == &_colorSpaces[i]) {
            return;
        }
    }
    
    free((void*)cs->desc.name);
    free((void*)cs);
}

NcM33f NcGetRGBToXYZMatrix(const NcColorSpace* cs) {
    if (!cs)
        return (NcM33f) {1,0,0, 0,1,0, 0,0,1};

    return cs->rgbToXYZ;
}

NcM33f NcGetXYZToRGBMatrix(const NcColorSpace* cs) {
    if (!cs)
        return (NcM33f) {1,0,0, 0,1,0, 0,0,1};

    return _M3ffInvert(NcGetRGBToXYZMatrix(cs));
}

NcM33f NcGetRGBToRGBMatrix(const NcColorSpace* src, const NcColorSpace* dst) {
    if (!dst || !src) {
        return (NcM33f){1,0,0,0,1,0,0,0,1};
    }
    
    NcM33f toXYZ = NcGetRGBToXYZMatrix(src);
    NcM33f fromXYZ = NcGetXYZToRGBMatrix(dst);
    
    NcM33f tx = _M33fMultiply(fromXYZ, toXYZ);
    return tx;
}

NcRGB NcTransformColor(const NcColorSpace* dst, const NcColorSpace* src, NcRGB rgb) {
    if (!dst || !src) {
        return rgb;
    }
    
    NcM33f tx = _M33fMultiply(NcGetRGBToXYZMatrix(src),
                               NcGetXYZToRGBMatrix(dst));
    
    // if the source color space indicates a curve remove it.
    rgb.r = _ToLinear(src, rgb.r);
    rgb.g = _ToLinear(src, rgb.g);
    rgb.b = _ToLinear(src, rgb.b);

    NcRGB out;
    out.r = tx.m[0] * rgb.r + tx.m[1] * rgb.g + tx.m[2] * rgb.b;
    out.g = tx.m[3] * rgb.r + tx.m[4] * rgb.g + tx.m[5] * rgb.b;
    out.b = tx.m[6] * rgb.r + tx.m[7] * rgb.g + tx.m[8] * rgb.b;
    
    // if the destination color space indicates a curve apply it.
    out.r = _FromLinear(dst, out.r);
    out.g = _FromLinear(dst, out.g);
    out.b = _FromLinear(dst, out.b);
    return out;
}

void NcTransformColors(const NcColorSpace* dst, const NcColorSpace* src, NcRGB* rgb, size_t count)
{
    if (!dst || !src || !rgb)
        return;
    
    NcM33f tx = _M33fMultiply(NcGetRGBToXYZMatrix(src),
                               NcGetXYZToRGBMatrix(dst));
    
    // if the source color space indicates a curve remove it.
    for (size_t i = 0; i < count; i++) {
        NcRGB out = rgb[i];
        out.r = _ToLinear(src, out.r);
        out.g = _ToLinear(src, out.g);
        out.b = _ToLinear(src, out.b);
        rgb[i] = out;
    }
    
    int start = 0;
#if HAVE_SSE2
    __m128 m0 = _mm_set_ps(tx.m[0], tx.m[1], tx.m[2], 0);
    __m128 m1 = _mm_set_ps(tx.m[3], tx.m[4], tx.m[5], 0);
    __m128 m2 = _mm_set_ps(tx.m[6], tx.m[7], tx.m[8], 0);
    __m128 m3 = _mm_set_ps(0, 0, 0, 1);
    
    for (size_t i = 0; i < count - 1; i++) {
        __m128 rgba = _mm_loadu_ps(&rgb[i].r);   // load rgbr
        
        // Set alpha component to 1.0 before multiplication
        rgba = _mm_add_ps(rgba, m3);
        
        // Perform the matrix multiplication
        __m128 rout = _mm_mul_ps(m0, rgba);
        rout = _mm_add_ps(rout, _mm_mul_ps(m1, rgba));
        rout = _mm_add_ps(rout, _mm_mul_ps(m2, rgba));
        rout = _mm_add_ps(rout, _mm_mul_ps(m3, rgba));
        
        // Store the result
        _mm_storeu_ps(&rgb[i].r, rout);
    }
    
    // transform the last value separately, because _mm_storeu_ps
    // writes 4 floats, and we may not have 4 floats left
    start = count - 2;
    count = 1;
#elif HAVE_NEON
    float32x4_t m0 = { tx.m[0], tx.m[1], tx.m[2], 0 };
    float32x4_t m1 = { tx.m[3], tx.m[4], tx.m[5], 0 };
    float32x4_t m2 = { tx.m[6], tx.m[7], tx.m[8], 0 };
    float32x4_t m3 = { 0, 0, 0, 1 };
    
    for (size_t i = 0; i < count - 1; i++) {
        float32x4_t rgba = vld1q_f32(&rgb[i].r);   // load rgbr
        
        // Set alpha component to 1.0 before multiplication
        rgba = vsetq_lane_f32(1.0f, rgba, 3);
        
        // Perform the matrix multiplication
        float32x4_t rout = vmulq_f32(m0, rgba);
        rout = vmlaq_f32(rout, m1, rgba);
        rout = vmlaq_f32(rout, m2, rgba);
        rout = vmlaq_f32(rout, m3, rgba);
        
        // Store the result
        vst1q_f32(&rgb[i].r, rout);
    }
    // transform the last value separately, because _mm_storeu_ps
    // writes 4 floats, and we may not have 4 floats left
    start = count - 2;
    count = 1;
#else
    for (size_t i = start; i < count; i++) {
        NcRGB in = rgb[i];
        NcRGB out = {
            tx.m[0] * in.r + tx.m[1] * in.g + tx.m[2] * in.b,
            tx.m[3] * in.r + tx.m[4] * in.g + tx.m[5] * in.b,
            tx.m[6] * in.r + tx.m[7] * in.g + tx.m[8] * in.b
        };
        rgb[i] = out;
    }
#endif
    
    // if the destination color space indicates a curve apply it.
    for (size_t i = 0; i < count; i++) {
        NcRGB out = rgb[i];
        out.r = _FromLinear(dst, out.r);
        out.g = _FromLinear(dst, out.g);
        out.b = _FromLinear(dst, out.b);
        rgb[i] = out;
    }
}

// same as NcTransformColor, but preserve alpha in the transformation
void NcTransformColorsWithAlpha(const NcColorSpace* dst, const NcColorSpace* src,
                                float* rgba, size_t count)
{
    if (!dst || !src || !rgba)
        return;
    
    NcM33f tx = _M33fMultiply(NcGetRGBToXYZMatrix(src),
                               NcGetXYZToRGBMatrix(dst));
    
    // if the source color space indicates a curve remove it.
    for (size_t i = 0; i < count; i++) {
        NcRGB out = { rgba[i * 4 + 0], rgba[i * 4 + 1], rgba[i * 4 + 2] };
        out.r = _ToLinear(src, out.r);
        out.g = _ToLinear(src, out.g);
        out.b = _ToLinear(src, out.b);
        rgba[i * 4 + 0] = out.r;
        rgba[i * 4 + 1] = out.g;
        rgba[i * 4 + 2] = out.b;
    }
    
#if HAVE_SSE2
    __m128 m0 = _mm_set_ps(tx.m[0], tx.m[1], tx.m[2], 0);
    __m128 m1 = _mm_set_ps(tx.m[3], tx.m[4], tx.m[5], 0);
    __m128 m2 = _mm_set_ps(tx.m[6], tx.m[7], tx.m[8], 0);
    __m128 m3 = _mm_set_ps(0,0,0,1);
    
    for (size_t i = 0; i < count; i += 4) {
        __m128 rgbaVec = _mm_loadu_ps(&rgba[i * 4]);  // Load all components (r, g, b, a)
        
        __m128  rout = _mm_mul_ps(m0, rgbaVec);
        rout = _mm_add_ps(rout, _mm_mul_ps(m1, rgbaVec));
        rout = _mm_add_ps(rout, _mm_mul_ps(m2, rgbaVec));
        rout = _mm_add_ps(rout, _mm_mul_ps(m3, rgbaVec));
        
        _mm_storeu_ps(&rgba[i * 4], rout);  // Store the result
    }
#elif HAVE_NEON
    float32x4x4_t matrix = {
        {tx.m[0], tx.m[1], tx.m[2], 0},
        {tx.m[3], tx.m[4], tx.m[5], 0},
        {tx.m[6], tx.m[7], tx.m[8], 0},
        {0, 0, 0, 1}
    };
    
    for (size_t i = 0; i < count; i += 4) {
        float32x4x4_t rgba_values = vld4q_f32(&rgba[i * 4]);
        
        float32x4_t rout = vmulq_f32(matrix.val[0], rgba_values.val[0]);
        rout = vmlaq_f32(rout, matrix.val[1], rgba_values.val[1]);
        rout = vmlaq_f32(rout, matrix.val[2], rgba_values.val[2]);
        rout = vmlaq_f32(rout, matrix.val[3], rgba_values.val[3]);
        
        vst1q_f32(&rgba[i * 4], rout);
    }
#else
    for (size_t i = 0; i < count; i++) {
        NcRGB in = { rgba[i * 4 + 0], rgba[i * 4 + 1], rgba[i * 4 + 2] };
        NcRGB out = {
            tx.m[0] * in.r + tx.m[1] * in.g + tx.m[2] * in.b,
            tx.m[3] * in.r + tx.m[4] * in.g + tx.m[5] * in.b,
            tx.m[6] * in.r + tx.m[7] * in.g + tx.m[8] * in.b
        };
        rgba[i * 4 + 0] = out.r;
        rgba[i * 4 + 1] = out.g;
        rgba[i * 4 + 2] = out.b;
        // leave alpha alone
    }
#endif

    // if the destination color space indicates a curve apply it.
    for (size_t i = 0; i < count; i++) {
        NcRGB out = { rgba[i * 4 + 0], rgba[i * 4 + 1], rgba[i * 4 + 2] };
        out.r = _FromLinear(dst, out.r);
        out.g = _FromLinear(dst, out.g);
        out.b = _FromLinear(dst, out.b);
        rgba[i * 4 + 0] = out.r;
        rgba[i * 4 + 1] = out.g;
        rgba[i * 4 + 2] = out.b;
    }
}

NcXYZ NcRGBToXYZ(const NcColorSpace* ct, NcRGB rgb) {
    if (!ct)
        return (NcXYZ) {0,0,0};
    
    rgb.r = _ToLinear(ct, rgb.r);
    rgb.g = _ToLinear(ct, rgb.g);
    rgb.b = _ToLinear(ct, rgb.b);

    NcM33f m = NcGetRGBToXYZMatrix(ct);
    return (NcXYZ) {
        m.m[0] * rgb.r + m.m[1] * rgb.g + m.m[2] * rgb.b,
        m.m[3] * rgb.r + m.m[4] * rgb.g + m.m[5] * rgb.b,
        m.m[6] * rgb.r + m.m[7] * rgb.g + m.m[8] * rgb.b
    };
}

NcRGB NcXYZToRGB(const NcColorSpace* ct, NcXYZ xyz) {
    if (!ct)
        return (NcRGB) {0,0,0};
    
    NcM33f m = NcGetXYZToRGBMatrix(ct);
    
    NcRGB rgb = {
        m.m[0] * xyz.x + m.m[1] * xyz.y + m.m[2] * xyz.z,
        m.m[3] * xyz.x + m.m[4] * xyz.y + m.m[5] * xyz.z,
        m.m[6] * xyz.x + m.m[7] * xyz.y + m.m[8] * xyz.z
    };
    
    rgb.r = _FromLinear(ct, rgb.r);
    rgb.g = _FromLinear(ct, rgb.g);
    rgb.b = _FromLinear(ct, rgb.b);
    return rgb;
}

NcYxy NcXYZToYxy(NcXYZ xyz) {
    float sum = xyz.x + xyz.y + xyz.z;
    if (sum == 0.f)
        return (NcYxy) {0, 0, xyz.y};
    
    return (NcYxy) {xyz.y, xyz.x / sum, xyz.y / sum};
}

NCAPI NcXYZ NcYxyToXYZ(NcYxy Yxy) {
    return (NcXYZ) { 
        Yxy.Y * Yxy.x / Yxy.y,
        Yxy.Y,
        Yxy.Y * (1.f - Yxy.x - Yxy.y) / Yxy.y };
}

const NcColorSpace* NcGetNamedColorSpace(const char* name)
{
    if (name) {
        for (size_t i = 0; i < sizeof(_colorSpaces) / sizeof(_colorSpaces[0]); i++) {
            if (strcmp(name, _colorSpaces[i].desc.name) == 0) {
                _InitColorSpace((NcColorSpace*) &_colorSpaces[i]); // ensure initialization
                return &_colorSpaces[i];
            }
        }
    }
    
    // currently Nanocolor doesn't have a concept of registering new color spaces
    return NULL;
}

static bool _CompareChromaticity(const NcChromaticity* a, const NcChromaticity* b, float threshold) {
    return fabsf(a->x - b->x) < threshold &&
           fabsf(a->y - b->y) < threshold;
}

// The main reason this exists is that OpenEXR encodes colorspaces via primaries
// and white point, and it would be good to be able to match an EXR file to a
// known colorspace, rather than setting up unique transforms for each image.
// Therefore constructing the namespaced name here via macro, to avoid including
// utils itself.

const char*
NcMatchLinearColorSpace(NcChromaticity redPrimary, NcChromaticity greenPrimary, NcChromaticity bluePrimary,
                        NcChromaticity  whitePoint, float threshold) {
    for (size_t i = 0; i < sizeof(_colorSpaces) / sizeof(NcColorSpace); ++i) {
        if (_colorSpaces[i].desc.gamma != 1.0f)
            continue;
        if (_CompareChromaticity(&_colorSpaces[i].desc.redPrimary, &redPrimary, threshold) &&
            _CompareChromaticity(&_colorSpaces[i].desc.greenPrimary, &greenPrimary, threshold) &&
            _CompareChromaticity(&_colorSpaces[i].desc.bluePrimary, &bluePrimary, threshold) &&
            _CompareChromaticity(&_colorSpaces[i].desc.whitePoint, &whitePoint, threshold))
            return _colorSpaces[i].desc.name;
    }
    return NULL;
}

bool NcGetColorSpaceDescriptor(const NcColorSpace* cs, NcColorSpaceDescriptor* desc) {
    if (!cs || !desc)
        return false;
    if (cs->desc.whitePoint.x == 0.f)
        return false;
    memcpy(desc, &cs->desc, sizeof(NcColorSpaceDescriptor));
    return true;
}

bool NcGetColorSpaceM33Descriptor(const NcColorSpace* cs, NcColorSpaceM33Descriptor* desc) {
    if (!cs || !desc)
        return false;
    desc->name = cs->desc.name;
    desc->gamma = cs->desc.gamma;
    desc->linearBias = cs->desc.linearBias;
    desc->rgbToXYZ = cs->rgbToXYZ;
    return true;
}

void NcGetK0Phi(const NcColorSpace* cs, float* K0, float* phi) {
    if (cs && K0 && phi) {
        *K0 = cs->K0;
        *phi = cs->phi;
    }
}

/* This is actually u'v', u'v' is uv scaled by 1.5 along the v axis
*/

typedef struct {
    float Y;
    float u;
    float v;
} NcYuvPrime;

static NcYxy _NcYuv2Yxy(NcYuvPrime c) {
    float d = 6.f * c.u - 16.f * c.v + 12.f;
    return (NcYxy) {
        c.Y,
        9.f * c.u / d,
        4.f * c.v / d
    };
}

/* Equations from the paper "An Algorithm to Calculate Correlated Colour 
   Temperature" by M. Krystek in 1985, using a rational Chebyshev approximation.
*/
NcYxy NcKelvinToYxy(float T, float luminance) {
    if (T < 1000 || T > 15000)
        return (NcYxy) { 0, 0, 0 };

    float u = (0.860117757 + 1.54118254e-4 * T + 1.2864121e-7 * T * T) /
              (1.0 + 8.42420235e-4 * T + 7.08145163e-7 * T * T);
    float v = (0.317398726 + 4.22806245e-5 * T + 4.20481691e-8 * T * T) /
              (1.0 - 2.89741816e-5 * T + 1.61456053e-7 * T * T);

    return _NcYuv2Yxy((NcYuvPrime) {luminance, u, 3.f * v / 2.f });
}

static NcYxy _NormalizeYxy(NcYxy c) {
    return (NcYxy) {
        c.Y,
        c.Y * c.x / c.y,
        c.Y * (1.f - c.x - c.y) / c.y
    };
}

static inline float _SignOf(float x) {
    return x > 0 ? 1.f : (x < 0) ? -1.f : 0.f;
}

NcRGB NcYxyToRGB(const NcColorSpace* cs, NcYxy c) {
    NcYxy cYxy = _NormalizeYxy(c);
    NcRGB rgb = NcXYZToRGB(cs, (NcXYZ) { cYxy.x, cYxy.Y, cYxy.y });
    NcRGB magRgb = {
        fabsf(rgb.r),
        fabsf(rgb.g),
        fabsf(rgb.b) };

    float maxc = (magRgb.r > magRgb.g) ? magRgb.r : magRgb.g;
    maxc = maxc > magRgb.b ? maxc : magRgb.b;
    NcRGB ret = (NcRGB) {
        _SignOf(rgb.r) * rgb.r / maxc,
        _SignOf(rgb.g) * rgb.g / maxc,
        _SignOf(rgb.b) * rgb.b / maxc };
    return ret;
}

