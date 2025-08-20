//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_NC_NANOCOLOR_H
#define PXR_BASE_GF_NC_NANOCOLOR_H

#include "pxr/base/arch/export.h"
#include <stdbool.h>
#include <stddef.h>

// NCNAMESPACE is allows the introduction of a namespace to the symbols so that 
// multiple libraries can include the nanocolor library without symbol 
// conflicts. The default is nc_1_0_ to indicate the 1.0 version of Nanocolor.
//
#ifndef NCNAMESPACE
#define NCNAMESPACE pxr_nc_1_0_
#endif

// The NCCONCAT macro is used to apply a namespace to the symbols in the public
// interface.
#define NCCONCAT1(a, b) a ## b
#define NCCONCAT(a, b) NCCONCAT1(a, b)

// NCAPI may be overridden externally to control symbol visibility.
#ifndef NCAPI
#define NCAPI ARCH_HIDDEN
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
#define NcRGBA NCCONCAT(NCNAMESPACE, RGBA)
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

// NcRGBA is pairing of NcRGB with an alpha channel.
// It has no intrinsic color space, nor does it define whether the alpha
// value is straight (unassociated) or premultiplied (associated).
typedef struct {
    NcRGB rgb;
    float alpha;
} NcRGBA;

// NcM33f is a 3x3 matrix of floats used for color space conversions.
// It's stored in row major order, such that posting multiplying an NcRGB
// as a column vector by an NcM33f will yield another NcRGB column
// transformed by that matrix.
typedef struct {
    float m[9];
} NcM33f;

// NcColorSpaceDescriptor describes a color space.
// The color space is defined by the red, green, and blue primaries,
// the white point, the gamma of the log section, and the linear bias.
typedef struct {
    const char*       descriptiveName;
    const char*       shortName;
    NcChromaticity    redPrimary, greenPrimary, bluePrimary;
    NcChromaticity    whitePoint;
    float             gamma;      // gamma of log section
    float             linearBias; // where the linear section ends
} NcColorSpaceDescriptor;

// NcColorSpaceM33Descriptor describes a color space defined in terms of a 
// 3x3 matrix, the gamma of the log section, and the linear bias.
typedef struct {
    const char*       descriptiveName;
    const char*       shortName;
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
 The named color spaces provided by Nanocolor are those published in the
 Color Interop Forum Texture Asset Color Spaces document.

The names correspond to the names found in the standard set of CIF color space
configurations distributed as part of OpenColorIO. The short names take the form
of three components joined by underscores. The first component is the encoding
of the RGB tuple; either linear for no encoding, srgb for `IEC 61966-2-1:1999`
encoding, or gNN where NN indicates a gamma value. The second component is the
name of the color space. Finally, the third component names the image state, as
named in `ISO 22028-1`, drawing a distinction between scene-referred and display-
referred color spaces. Scene referenced color spaces are used to describe the
color of objects in the real world, while display-referred color spaces are used
to describe the color of images displayed on a screen.

`CIE XYZ-D65 - Scene-referred` bears some additional explanation. The D65
component in the name indicates that values transformed to this color space are
subject to an adaptation to the D65 white point. This is necessary to match
results from the ACES reference transformation implementations. Of the color
spaces provided, only the ACES color spaces by definition have an adaptation
to the D65 whitepoint, as required to match results provided by DCCs and OCIO.

ACEScg: lin_ap1_scene
ACES2065-1: lin_ap0_scene
Linear Rec.709 (sRGB): lin_rec709_scene
Linear P3-D65: lin_p3d65_scene
Linear Rec.2020: lin_rec2020_scene
Linear AdobeRGB: lin_adobergb_scene
CIE XYZ-D65 - Scene-referred: lin_ciexyzd65_scene
sRGB Encoded Rec.709 (sRGB): srgb_rec709_scene
Gamma 2.2 Encoded Rec.709: g22_rec709_scene
Gamma 1.8 Encoded Rec.709: g18_rec709_scene
sRGB Encoded AP1: srgb_ap1_scene
Gamma 2.2 Encoded AP1: g22_ap1_scene
sRGB Encoded P3-D65: srgb_p3d65_scene
Gamma 2.2 Encoded AdobeRGB: g22_adobergb_scene
Data: data
Unknown: unknown

Additionally, raw and identity color spaces are provided as they are commonly
found in documents naming color spaces.
*/

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
#define NcRGBToXYZ                   NCCONCAT(NCNAMESPACE, RGBToXYZ)
#define NcTransformColor             NCCONCAT(NCNAMESPACE, TransformColor)
#define NcTransformColors            NCCONCAT(NCNAMESPACE, TransformColors)
#define NcTransformColorsWithAlpha   NCCONCAT(NCNAMESPACE, TransformColorsWithAlpha)
#define NcXYZToRGB                   NCCONCAT(NCNAMESPACE, XYZToRGB)
#define NcXYZToYxy                   NCCONCAT(NCNAMESPACE, XYZToYxy)
#define NcYxyToRGB                   NCCONCAT(NCNAMESPACE, YxyToRGB)
#define NcYxyToXYZ                   NCCONCAT(NCNAMESPACE, YxyToXYZ)

// Reference implementations for testing purposes.
#define NcTransformColorsRef            NCCONCAT(NCNAMESPACE, TransformColorsRef)
#define NcTransformColorsWithAlphaRef   NCCONCAT(NCNAMESPACE, TransformColorsWithAlphaRef)

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
 * @param count Number of colors in the array. (1 RGB triplet = 1 color).
 * @return void
 */
NCAPI void NcTransformColors(const NcColorSpace* dst, const NcColorSpace* src, 
                             NcRGB* rgb, size_t count);

/**
 * Transforms an array of colors from one color space to another.
 * Reference implementation for testing purposes.
 *
 * @param dst Pointer to the destination color space object.
 * @param src Pointer to the source color space object.
 * @param rgb Pointer to the array of RGB colors to transform.
 * @param count Number of colors in the array. (1 RGB triplet = 1 color).
 * @return void
 */
NCAPI void NcTransformColorsRef(const NcColorSpace* dst, const NcColorSpace* src, 
                                NcRGB* rgb, size_t count);

/**
 * Transforms an array of colors with alpha channel from one color space to another.
 *
 * @param dst Pointer to the destination color space object.
 * @param src Pointer to the source color space object.
 * @param rgba Pointer to the array of RGBA colors to transform.
 * @param count Number of colors in the array. (1 RGBA quadruplet = 1 color).
 * @return void
 */
NCAPI void NcTransformColorsWithAlpha(const NcColorSpace* dst, const NcColorSpace* src,
                                      NcRGBA* rgba, size_t count);

/**
 * Transforms an array of colors with alpha channel from one color space to another.
 * Reference implementation for testing purposes.
 * 
 * @param dst Pointer to the destination color space object.
 * @param src Pointer to the source color space object.
 * @param rgba Pointer to the array of RGBA colors to transform.
 * @param count Number of colors in the array. (1 RGBA quadruplet = 1 color).
 * @return void
 */
NCAPI void NcTransformColorsWithAlphaRef(const NcColorSpace* dst, const NcColorSpace* src,
                                         NcRGBA* rgba, size_t count);

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
