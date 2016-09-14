//
// Copyright 2016 Pixar
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
#ifndef GF_RGB_H
#define GF_RGB_H

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3f.h"

#include <iosfwd>

class GfMatrix4d;

//!
// \file rgb.h
// \ingroup group_gf_Color
//

//! \class GfRGB rgb.h "pxr/base/gf/rgb.h"
//  \brief A color represented as 3 floats for red, green, and blue.
//
// The \c GfRGB class contains three floats that represent an RGB
// color, in the order red, green, blue.
// Conversions to and from some other color spaces are provided.

class GfRGB {

  public:
    //! The default constructor creates an invalid color.
    GfRGB() { Set(NAN, NAN, NAN); }

    //! This constructor initializes the color to grey.
    explicit GfRGB(int bw) { _rgb[0] = _rgb[1] = _rgb[2] = bw; }

    explicit GfRGB(GfVec3f const &v) : _rgb(v) {}
    
    //! This constructor initializes the color to grey.
    explicit GfRGB(float grey) : _rgb(grey, grey, grey) {}

    //! Constructor that takes an array of 3 floats.
    explicit GfRGB(const float rgb[3]) : _rgb(rgb) {}

    //! Constructor that takes individual red, green, and blue values.
    GfRGB(float red, float green, float blue) : _rgb(red, green, blue) {}

    //! Sets the color from an array of 3 floats.
    GfRGB &Set(const float rgb[3]) {
        _rgb.Set(rgb);
        return *this;
    }

    //! Sets the color to individual red, green, and blue values.
    GfRGB &Set(float red, float green, float blue) {
        _rgb.Set(red, green, blue);
        return *this;
    }
    
    //! Returns whether or not the color is valid. By convention, a color
    //  is valid if the first color component is not NAN.
    bool IsValid() const { return !std::isnan(_rgb[0]); }

    //! Returns the RGB color as a \c GfVec3f.
    const GfVec3f &GetVec() const { return _rgb; }
    
    //! Returns the RGB color as an array of 3 floats.
    const float *GetArray() const { return _rgb.GetArray(); }

    //! Accesses indexed component of color as a modifiable l-value.
    float &operator [](int i) { return _rgb[i]; }

    //! Accesses indexed component of color as a \c const l-value.
    const float &operator [](int i) const { return _rgb[i]; }

    //! Clamps each component of the color to be in the given range.
    void Clamp(float min = 0.0, float max = 1.0) {
        _rgb[0] = GfClamp(_rgb[0], min, max);
        _rgb[1] = GfClamp(_rgb[1], min, max);
        _rgb[2] = GfClamp(_rgb[2], min, max);
    }

    //! All components must match exactly for colors to be considered equal.
    bool operator ==(const GfRGB &c) const {
        return _rgb == c._rgb;
    }

    //! Any component may differ
    bool	operator !=(const GfRGB &c) const {
        return !(*this == c);
    }
    
    //! Check to see if all components are set to 0.
    bool IsBlack() const { 
        return _rgb == GfVec3f(0);
    }

    //! Check to see if all components are set to 1.
    bool IsWhite() const {
        return _rgb == GfVec3f(1,1,1);
    }

    //! Component-wise color addition.
    GfRGB &operator +=(const GfRGB &c) {
        _rgb += c._rgb;
        return *this;
    }

    //! Component-wise color subtraction.
    GfRGB &operator -=(const GfRGB &c) {
        _rgb -= c._rgb;
        return *this;
    }

    //! Component-wise color multiplication.
    GfRGB &operator *=(const GfRGB &c) {
        _rgb = GfCompMult(_rgb, c._rgb);
        return *this;
    }

    //! Component-wise color division.
    GfRGB &operator /=(const GfRGB &c) {
        _rgb = GfCompDiv(_rgb, c._rgb);
        return *this;
    }

    //! Component-wise scalar multiplication.
    GfRGB &operator *=(float d) {
        _rgb *= d;
        return *this;
    }

    //! Component-wise scalar division.
    GfRGB &operator /=(float d) {
        _rgb /= d;
        return *this;
    }

    //! Returns component-wise multiplication of colors \p c1 and \p c2.
    // \warning this is \em not a dot product operator, as it is with vectors.
    friend GfRGB	operator *(const GfRGB &c1, const GfRGB &c2) {
	return GfRGB(GfCompMult(c1._rgb, c2._rgb));
    }

    //! quotient operator
    friend GfRGB	operator /(const GfRGB &c1, const GfRGB &c2) {
	return GfRGB(GfCompDiv(c1._rgb, c2._rgb));
    }

    //!  addition operator.
    friend GfRGB	operator +(const GfRGB &c1, const GfRGB &c2) {
	return GfRGB(c1._rgb + c2._rgb);
    }

    //!  subtraction operator.
    friend GfRGB	operator -(const GfRGB &c1, const GfRGB &c2) {
	return GfRGB(c1._rgb - c2._rgb);
    }

    //!  multiplication operator.
    friend GfRGB	operator *(const GfRGB &c, float s) {
	return GfRGB(c._rgb * s);
    }

    //! Component-wise binary color/scalar multiplication operator.
    friend GfRGB  operator *(float s, const GfRGB &c) {
        return c * s;
    }

    //! Component-wise binary color/scalar division operator.
    friend GfRGB  operator /(const GfRGB &c, float s) {
        return c * (1./s);
    }

    //! Tests for equality within a given tolerance, returning true if the
    // difference between each component is less than \p tolerance.
    friend bool GfIsClose(const GfRGB &v1, const GfRGB &v2, double tolerance);

    //! Returns \code (1-alpha) * a + alpha * b \endcode
    // similar to GfLerp for other vector types.
    friend GfRGB GfLerp(float alpha, const GfRGB &a, const GfRGB &b) {
        return (1.0-alpha) * a + alpha * b;
    }

    //! \name Color-space conversions.
    // The methods in this group convert between RGB and other color spaces.
    //@{

    //! Transform the color into an arbitrary space.
    GfRGB Transform(const GfMatrix4d &m) const;

    //! Transform the color into an arbitrary space.
    friend GfRGB operator *(const GfRGB &c, const GfMatrix4d &m);

    //! Return the complement of a color.
    // (Note that this assumes normalized RGB channels in [0,1]
    // and doesn't work with HDR color values.)
    GfRGB GetComplement() const {
        return GfRGB(1) - *this;
    }

    //! Return the luminance of a color given a set of RGB
    // weighting values.  Defaults are Rec.709 weights for
    // linear RGB components.
    float       GetLuminance(float wr = 0.2126390,
                             float wg = 0.7151687,
                             float wb = 0.07219232) const {
                    return _rgb[0]*wr + _rgb[1]*wg + _rgb[2]*wb;
                }

    //! Return the luminance of a color given a set of RGB
    // weighting values passed as a GfRGB color object.
    float       GetLuminance(const GfRGB &coeffs) const {
                    return _rgb[0]*coeffs._rgb[0] +
                           _rgb[1]*coeffs._rgb[1] +
                           _rgb[2]*coeffs._rgb[2];
                }

    //! Returns the equivalent of this color in HSV space 
    void GetHSV(float *hue, float *sat, float *value) const;

    //! Sets this RGB to the RGB equivalent of the given HSV color.
    void SetHSV(float hue, float sat, float value);

    //! Given an RGB base and HSV offset, get an RGB color.
    static GfRGB GetColorFromOffset(const GfRGB &offsetBase, 
                                    const GfRGB &offsetHSV);

    //! Given an HSV offset color and an RGB base, get the HSV offset
    static GfRGB GetOffsetFromColor(const GfRGB &offsetBase,
                                    const GfRGB &offsetColor);

    //@}

  private:
    //! Color storage.
    GfVec3f _rgb;
};

// Friend functions must be declared.
bool GfIsClose(const GfRGB &v1, const GfRGB &v2, double tolerance);
GfRGB operator *(const GfRGB &c, const GfMatrix4d &m);

/// Output a GfRGB color using the format (r, g, b).
/// \ingroup group_gf_DebuggingOutput
std::ostream& operator<<(std::ostream& out, const GfRGB& c);

#endif // GF_RGB_H
