//
// Copyright 2024 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_BASE_GF_NC_NANOCOLOR_H
#define PXR_BASE_GF_NC_NANOCOLOR_H

#include <stdbool.h>
#include <stddef.h>

// NCNAMESPACE is allows the introduction of a namespace to the symbols so that 
// multiple libraries can include the nanocolor library without symbol 
// conflicts. The default is nc_1_0_ to indicate the 1.0 version of Nanocolor.
//
// pxr: note that the PXR namespace macros are in pxr/pxr.h which
// is a C++ only header; so the generated namespace prefixes can't be
// used here.
#ifndef NCNAMESPACE
#define NCNAMESPACE pxr_nc_1_0_
#endif

// The NCCONCAT macro is used to apply a namespace to the symbols in the public
// interface.
#define NCCONCAT1(a, b) a ## b
#define NCCONCAT(a, b) NCCONCAT1(a, b)

// NCAPI may be overridden externally to control symbol visibility.
#ifndef NCAPI
#define NCAPI
#endif

#ifdef __cplusplus
#define NCEXTERNC extern "C" NCAPI
#else
#define NCEXTERNC extern NCAPI
#endif

#define NcChromaticity NCCONCAT(NCNAMESPACE, Chromaticity)
#define NcXYZ NCCONCAT(NCNAMESPACE, XYZ)
#define NcYxy NCCONCAT(NCNAMESPACE, Yxy)
#define NcRGB NCCONCAT(NCNAMESPACE, RGB)
#define NcM33f NCCONCAT(NCNAMESPACE, M33f)
#define NcColorSpaceDescriptor NCCONCAT(NCNAMESPACE, ColorSpaceDescriptor)
#define NcColorSpaceM33Descriptor NCCONCAT(NCNAMESPACE, ColorSpaceM33Descriptor)
#define NcColorSpace NCCONCAT(NCNAMESPACE, ColorSpace)

// NcChromaticity is a single coordinate in the CIE 1931 xy chromaticity diagram.
typedef struct {
    float x, y;
} NcChromaticity;

// NcXYZ is a coordinate in the CIE 1931 2-degree XYZ color space.
typedef struct {
    float x, y, z;
} NcXYZ;

// NcYxy is a chromaticity coordinate with luminance.
typedef struct {
    float Y, x, y;
} NcYxy;

// NcRGB is an rgb coordinate with no intrinsic color space.
typedef struct {
    float r, g, b;
} NcRGB;

// NcM33f is a 3x3 matrix of floats used for color space conversions.
// It's stored in column major order, such that multiplying an NcRGB
// by an NcM33f will yield another NcRGB transformed by that matrix.
typedef struct {
    float m[9];
} NcM33f;

// NcColorSpaceDescriptor describes a color space.
// The color space is defined by the red, green, and blue primaries,
// the white point, the gamma of the log section, and the linear bias.
typedef struct {
    const char*       name;
    NcChromaticity    redPrimary, greenPrimary, bluePrimary;
    NcChromaticity    whitePoint;
    float             gamma;      // gamma of log section
    float             linearBias; // where the linear section ends
} NcColorSpaceDescriptor;

// NcColorSpaceM33Descriptor describes a color space defined in terms of a 
// 3x3 matrix, the gamma of the log section, and the linear bias.
typedef struct {
    const char*       name;
    NcM33f            rgbToXYZ;
    float             gamma;      // gamma of log section
    float             linearBias; // where the linear section ends
} NcColorSpaceM33Descriptor;

// Opaque struct for the public interface
typedef struct NcColorSpace NcColorSpace;

#ifdef __cplusplus
extern "C" {
#endif

/*
 The named color spaces provided by Nanocolor are as follows.
 Note that the names are shared with libraries such as MaterialX.

 - acescg:           The Academy Color Encoding System, a color space designed
                     for cinematic content creation and exchange, using AP1 primaries
 - adobergb:         A color space developed by Adobe Systems. It has a wider gamut
                     than sRGB and is suitable for photography and printing
 - g18_ap1:          A color space with a 1.8 gamma and an AP1 primaries color gamut
 - g18_rec709:       A color space with a 1.8 gamma, and primaries per the Rec. 709
                     standard, commonly used in HDTV
 - g22_ap1:          A color space with a 2.2 gamma and an AP1 primaries color gamut
 - g22_rec709:       A color space with a 2.2 gamma, and primaries per the Rec. 709
                     standard, commonly used in HDTV
 - identity:         Indicates that no transform is applied.
 - lin_adobergb:     The AdobeRGB gamut, and linear gamma
 - lin_ap0:          AP0 primaries, and linear gamma
 - lin_ap1:          AP1 primaries, and linear gamma; these are the ACESCg primaries
 - lin_displayp3:    DisplayP3 gamut, and linear gamma
 - lin_rec709:       A linearized version of the Rec. 709 color space.
 - lin_rec2020:      Rec2020 gamut, and linear gamma
 - lin_srgb:         sRGB gamut, linear gamma
 - raw:              Indicates that no transform is applied.
 - srgb_displayp3:   sRGB color space adapted to the Display P3 primaries
 - sRGB:             The sRGB color space.
 - srgb_texture:     The sRGB color space.
*/

NCEXTERNC const char* Nc_acescg;
NCEXTERNC const char* Nc_adobergb;
NCEXTERNC const char* Nc_g18_ap1;
NCEXTERNC const char* Nc_g18_rec709;
NCEXTERNC const char* Nc_g22_ap1;
NCEXTERNC const char* Nc_g22_rec709;
NCEXTERNC const char* Nc_identity;
NCEXTERNC const char* Nc_lin_adobergb;
NCEXTERNC const char* Nc_lin_ap0;
NCEXTERNC const char* Nc_lin_ap1;
NCEXTERNC const char* Nc_lin_displayp3;
NCEXTERNC const char* Nc_lin_rec709;
NCEXTERNC const char* Nc_lin_rec2020;
NCEXTERNC const char* Nc_lin_srgb;
NCEXTERNC const char* Nc_raw;
NCEXTERNC const char* Nc_srgb_displayp3;
NCEXTERNC const char* Nc_sRGB;
NCEXTERNC const char* Nc_srgb_texture;

// Declare the public interface using the namespacing macro.
#define NcColorSpaceEqual            NCCONCAT(NCNAMESPACE, ColorSpaceEqual)
#define NcCreateColorSpace           NCCONCAT(NCNAMESPACE, CreateColorSpace)
#define NcCreateColorSpaceM33        NCCONCAT(NCNAMESPACE, CreateColorSpaceM33)
#define NcFreeColorSpace             NCCONCAT(NCNAMESPACE, FreeColorSpace)
#define NcGetColorSpaceDescriptor    NCCONCAT(NCNAMESPACE, GetColorSpaceDescriptor)
#define NcGetColorSpaceM33Descriptor NCCONCAT(NCNAMESPACE, GetColorSpaceM33Descriptor)
#define NcGetDescription             NCCONCAT(NCNAMESPACE, GetDescription)
#define NcGetK0Phi                   NCCONCAT(NCNAMESPACE, GetK0Phi)
#define NcGetNamedColorSpace         NCCONCAT(NCNAMESPACE, GetNamedColorSpace)
#define NcGetRGBToRGBMatrix          NCCONCAT(NCNAMESPACE, GetRGBToRGBMatrix)
#define NcGetRGBToXYZMatrix          NCCONCAT(NCNAMESPACE, GetRGBtoXYZMatrix)
#define NcGetXYZToRGBMatrix          NCCONCAT(NCNAMESPACE, GetXYZtoRGBMatrix)
#define NcInitColorSpaceLibrary      NCCONCAT(NCNAMESPACE, InitColorSpaceLibrary)
#define NcKelvinToYxy                NCCONCAT(NCNAMESPACE, KelvinToYxy)
#define NcMatchLinearColorSpace      NCCONCAT(NCNAMESPACE, MatchLinearColorSpace)
#define NcRegisteredColorSpaceNames  NCCONCAT(NCNAMESPACE, RegisteredColorSpaceNames)
#define NcRGBToXYZ                   NCCONCAT(NCNAMESPACE, RGBToXYZ)
#define NcTransformColor             NCCONCAT(NCNAMESPACE, TransformColor)
#define NcTransformColors            NCCONCAT(NCNAMESPACE, TransformColors)
#define NcTransformColorsWithAlpha   NCCONCAT(NCNAMESPACE, TransformColorsWithAlpha)
#define NcXYZToRGB                   NCCONCAT(NCNAMESPACE, XYZToRGB)
#define NcXYZToYxy                   NCCONCAT(NCNAMESPACE, XYZToYxy)
#define NcYxyToRGB                   NCCONCAT(NCNAMESPACE, YxyToRGB)
#define NcYxyToXYZ                   NCCONCAT(NCNAMESPACE, YxyToXYZ)

/**
 * @brief Initializes the color space library.
 * 
 * Initializes the color spaces provided in the built-in color space library. 
 * This function is not thread-safe and must be called before NcGetNamedColorSpace
 * is called.
 * 
 * @return void
 */
NCAPI void NcInitColorSpaceLibrary(void);

/**
 * @brief Retrieves the names of the registered color spaces.
 * 
 * Retrieves the names of the color spaces that have been registered.
 * This function must not be called before NcInitColorSpaceLibrary is called.
 * 
 * @return Pointer to an array of strings containing the names of the registered color spaces.
 */
NCAPI const char** NcRegisteredColorSpaceNames(void);

/**
 * @brief Retrieves a named color space.
 * 
 * Retrieves a color space object based on the provided name.
 * This function must not be called before NcInitColorSpaceLibrary is called.
 * 
 * @param name The name of the color space to retrieve.
 * @return Pointer to the color space object, or NULL if not found.
 */
NCAPI const NcColorSpace* NcGetNamedColorSpace(const char* name);

/**
 * Creates a color space object based on the provided color space descriptor.
 * 
 * @param cs Pointer to the color space descriptor.
 * @return Pointer to the created color space object, or NULL if creation fails.
 */
NCAPI const NcColorSpace* NcCreateColorSpace(const NcColorSpaceDescriptor* cs);

/**
 * Creates a color space object based on the provided 3x3 matrix color space descriptor.
 * 
 * @param cs Pointer to the 3x3 matrix color space descriptor.
 * @return Pointer to the created color space object, or NULL if creation fails.
 */
NCAPI const NcColorSpace* NcCreateColorSpaceM33(const NcColorSpaceM33Descriptor* cs,
                                                bool* matrixIsNormalized);

/**
 * @brief Frees the memory associated with a color space object.
 * 
 * Frees the memory associated with a color space object. 
 * If this function is called on one of the built in library color spaces, it will
 * return without freeing the memory.
 * 
 * @param cs Pointer to the color space object to be freed.
 * @return void
 */
NCAPI void NcFreeColorSpace(const NcColorSpace* cs);

/**
 * Retrieves the RGB to XYZ transformation matrix for a given color space.
 * 
 * @param cs Pointer to the color space object.
 * @return The 3x3 transformation matrix.
 */
NCAPI NcM33f NcGetRGBToXYZMatrix(const NcColorSpace* cs);

/**
 * Retrieves the XYZ to RGB transformation matrix for a given color space.
 * 
 * @param cs Pointer to the color space object.
 * @return The 3x3 transformation matrix.
 */
NCAPI NcM33f NcGetXYZToRGBMatrix(const NcColorSpace* cs);

/**
 * Retrieves the RGB to RGB transformation matrix from source to destination color space.
 * 
 * @param src Pointer to the source color space object.
 * @param dst Pointer to the destination color space object.
 * @return The 3x3 transformation matrix.
 */
NCAPI NcM33f NcGetRGBToRGBMatrix(const NcColorSpace* src, const NcColorSpace* dst);

/**
 * Transforms a color from one color space to another.
 * 
 * @param dst Pointer to the destination color space object.
 * @param src Pointer to the source color space object.
 * @param rgb The RGB color to transform.
 * @return The transformed RGB color in the destination color space.
 */
NCAPI NcRGB NcTransformColor(const NcColorSpace* dst, const NcColorSpace* src, NcRGB rgb);

/**
 * Transforms an array of colors from one color space to another.
 * 
 * @param dst Pointer to the destination color space object.
 * @param src Pointer to the source color space object.
 * @param rgb Pointer to the array of RGB colors to transform.
 * @param count Number of colors in the array.
 * @return void
 */
NCAPI void NcTransformColors(const NcColorSpace* dst, const NcColorSpace* src, 
                             NcRGB* rgb, size_t count);

/**
 * Transforms an array of colors with alpha channel from one color space to another.
 * 
 * @param dst Pointer to the destination color space object.
 * @param src Pointer to the source color space object.
 * @param rgba Pointer to the array of RGBA colors to transform.
 * @param count Number of colors in the array.
 * @return void
 */
NCAPI void NcTransformColorsWithAlpha(const NcColorSpace* dst, const NcColorSpace* src,
                                      float* rgba, size_t count);

/**
 * Converts an RGB color to XYZ color space using the provided color space.
 * 
 * @param cs Pointer to the color space object.
 * @param rgb The RGB color to convert.
 * @return The XYZ color.
 */
NCAPI NcXYZ  NcRGBToXYZ(const NcColorSpace* cs, NcRGB rgb);

/**
 * Converts a XYZ color to RGB color space using the provided color space.
 * 
 * @param cs Pointer to the color space object.
 * @param xyz The XYZ color to convert.
 * @return The RGB color.
 */
NCAPI NcRGB NcXYZToRGB(const NcColorSpace* cs, NcXYZ xyz);

/**
 * Converts a XYZ color to Yxy color space.
 * 
 * @param xyz The XYZ color to convert.
 * @return The Yxy color.
*/
NCAPI NcYxy NcXYZToYxy(NcXYZ xyz);

/**
 * Converts an Yxy color coordinate to XYZ.
 * 
 * @param Yxy The Yxy color coordinate.
 * @return The XYZ color coordinate.
 */
NCAPI NcXYZ NcYxyToXYZ(NcYxy Yxy);

/**
 * Converts an Yxy color coordinate to RGB using the specified color space.
 * 
 * @param cs The color space.
 * @param c The Yxy color coordinate.
 * @return The RGB color coordinate.
 */
NCAPI NcRGB NcYxyToRGB(const NcColorSpace* cs, NcYxy c);

/**
 * Checks if two color space objects are equal by comparing their properties.
 * 
 * @param cs1 Pointer to the first color space object.
 * @param cs2 Pointer to the second color space object.
 * @return True if the color space objects are equal, false otherwise.
 */
NCAPI bool NcColorSpaceEqual(const NcColorSpace* cs1, const NcColorSpace* cs2);

/**
 * @brief Retrieves the color space descriptor.
 * 
 * Returns true if the color space descriptor was filled in. Color spaces initialized 
 * using a 3x3 matrix will not fill in the values. Note that 'name' within the populated 
 * descriptor is a pointer to a string owned by the color space, and is valid only as 
 * long as 'cs' is valid.
 * 
 * @param cs Pointer to the color space object.
 * @param desc Pointer to the color space descriptor to be filled in.
 * @return True if the descriptor was filled in, false otherwise.
 */
NCAPI bool NcGetColorSpaceDescriptor(const NcColorSpace* cs, NcColorSpaceDescriptor*);

/**
 * @brief Retrieves the 3x3 matrix color space descriptor.
 * 
 * Returns true if the color space descriptor was filled in. All properly initialized 
 * color spaces will be able to fill in the values. Note that 'name' within the populated 
 * descriptor is a pointer to a string owned by the color space, and is valid only as 
 * long as 'cs' is valid.
 * 
 * @param cs Pointer to the color space object.
 * @param desc Pointer to the 3x3 matrix color space descriptor to be filled in.
 * @return True if the descriptor was filled in, false otherwise.
 */
NCAPI bool NcGetColorSpaceM33Descriptor(const NcColorSpace* cs, NcColorSpaceM33Descriptor*);

/**
 * Returns a string describing the color space.
 * 
 * @param cs Pointer to the color space object.
 * @return A string describing the color space.
 */
NCAPI const char* NcGetDescription(const NcColorSpace* cs);

/**
 * @brief Retrieves the K0 and phi values of the color space.
 * 
 * Retrieves the K0 and Phi values of the color space, which are used in curve
 * transformations. K0 represents the transition point in the curve function,
 * and Phi represents the slope of the linear segment before the transition.
 * 
 * @param cs Pointer to the color space object.
 * @param K0 Pointer to store the K0 value.
 * @param phi Pointer to store the phi value.
 * @return void
 */
NCAPI void NcGetK0Phi(const NcColorSpace* cs, float* K0, float* phi);

/**
 * @brief Matches a linear color space based on specified primaries and white point.
 * 
 * Returns a string describing the color space that best matches the specified primaries
 * and white point. A reasonable epsilon for the comparison is 1e-4 because most color 
 * spaces are defined to that precision.
 * 
 * @param redPrimary Red primary chromaticity.
 * @param greenPrimary Green primary chromaticity.
 * @param bluePrimary Blue primary chromaticity.
 * @param whitePoint White point chromaticity.
 * @param epsilon Epsilon value for comparison.
 * @return A string describing the matched color space.
 */
NCAPI const char* NcMatchLinearColorSpace(NcChromaticity redPrimary,
                                          NcChromaticity greenPrimary,
                                          NcChromaticity bluePrimary,
                                          NcChromaticity whitePoint,
                                          float epsilon);

/**
 * @brief Returns an Yxy coordinate on the blackbody emission spectrum
 *
 * Returns an Yxy coordinate on the blackbody emission spectrum for values 
 * between 1000 and 15000K. Note that temperatures below 1900 are out of gamut
 * for some common colorspaces, such as Rec709.
 * 
 *  @param temperature The blackbody temperature in Kelvin.
 *  @param luminosity The luminosity.
 *  @return An Yxy coordinate.
 */
NCAPI NcYxy NcKelvinToYxy(float temperature, float luminosity);

#ifdef __cplusplus
}
#endif
#endif /* PXR_BASE_GF_NC_NANOCOLOR_H */
