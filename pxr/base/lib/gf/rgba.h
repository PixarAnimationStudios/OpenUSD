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
#ifndef GF_RGBA_H
#define GF_RGBA_H

/// \file gf/rgba.h
/// \ingroup group_gf_Color

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/rgb.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/api.h"

#include <iosfwd>

class GfMatrix4d;

/// \class GfRGBA
///
/// A color represented as 4 floats for red, green, blue, and alpha.
///
/// The \c GfRGBA class contains four floats that represent an RGBA
/// color, in the order red, green, blue, alpha (opacity). Several
/// color operations are provided.
///
class GfRGBA {

  public:
    /// The default constructor creates an invalid color.
    GfRGBA() {
    	Set(NAN, NAN, NAN, NAN);
    }

    /// Constructor that takes a \c GfVec4f.
    explicit GfRGBA(const GfVec4f &v) : _rgba(v) {}

    /// This constructor initializes the each component to \a grey.
    explicit GfRGBA(float grey) : _rgba(grey, grey, grey, grey) {}

    /// Constructor that takes an array of 4 floats.
    explicit GfRGBA(const float rgba[4]) : _rgba(rgba) {}

    /// Constructor that takes individual red, green, and blue values.
    GfRGBA(float red, float green, float blue) :
        _rgba(red, green, blue, 1.0)
    {
    }

    /// Constructor that takes individual red, green, blue, and alpha values.
    GfRGBA(float red, float green, float blue, float alpha) :
        _rgba(red, green, blue, alpha)
    {
    }

    /// Constructor and implicit conversion from GfRGB that takes an optional
    /// alpha value.
    GfRGBA(const GfRGB &rgb, float alpha=1.0) :
        _rgba(rgb[0], rgb[1], rgb[2], alpha)
    {
    }

    /// Sets the color from an array of 4 floats.
    GfRGBA &Set(const float rgba[4]) {
        _rgba.Set(rgba);
        return *this;
    }

    /// Sets the color to individual red, green, blue, and alpha values.
    GfRGBA &Set(float red, float green, float blue, float alpha) {
        _rgba.Set(red, green, blue, alpha);
        return *this;
    }

    /// Sets the color from a \c GfRGB and an alpha (opacity) value.
    GfRGBA &Set(const GfRGB &rgb, float alpha) {
        _rgba.Set(rgb[0], rgb[1], rgb[2], alpha);
        return *this;
    }

    /// Returns the RGBA color as a \c GfVec4f.
    const GfVec4f &GetVec() const { return _rgba; }

    /// Returns the RGBA color as an array of 4 floats.
    const float *GetArray() const { return _rgba.GetArray(); }

    /// Returns whether or not the color is valid. By convention, a color is
    /// valid if the first color component is not NAN.
    bool IsValid() const { return !isnan(_rgba[0]); }

    /// Accesses indexed component of color as a modifiable l-value.
    float &operator [](int i) { return _rgba[i]; }

    /// Accesses indexed component of color as a \c const l-value.
    const float &operator [](int i) const { return _rgba[i]; }

    /// Clamps each component of the color to be in the given range.
    void Clamp(float min = 0.0, float max = 1.0) {
        _rgba[0] = GfClamp(_rgba[0], min, max);
        _rgba[1] = GfClamp(_rgba[1], min, max);
        _rgba[2] = GfClamp(_rgba[2], min, max);
        _rgba[3] = GfClamp(_rgba[3], min, max);
    }

    /// Component-wise color equality test. All components must match exactly
    /// for colors to be considered equal.
    bool operator ==(const GfRGBA &c) const {
        return _rgba == c._rgba;
    }

    /// Component-wise color inequality test. All components must match
    /// exactly for colors to be considered equal.
    bool		operator !=(const GfRGBA &c) const {
	return ! (*this == c);
    }

    /// Check to see if all color components are set to 0, ignoring alpha.
    bool IsBlack() const {
        return GfVec3f(_rgba[0], _rgba[1], _rgba[2]) == GfVec3f(0);
    }

    /// Check to see if all color components are set to 1, ignoring alpha.
    bool IsWhite() const {
        return GfVec3f(_rgba[0], _rgba[1], _rgba[2]) == GfVec3f(1,1,1);
    }

    /// Return true if \a alpha is 0.
    bool IsTransparent() const {
        return _rgba[3] == 0;
    }

    /// Return true if \a alpha is 1.
    bool IsOpaque() const {
        return _rgba[3] == 1;
    }
    
    /// Component-wise unary color addition.
    GfRGBA &operator +=(const GfRGBA &c) {
        _rgba += c._rgba;
        return *this;
    }

    /// Component-wise unary color subtraction.
    GfRGBA &operator -=(const GfRGBA &c) {
        _rgba -= c._rgba;
        return *this;
    }

    /// Component-wise color multiplication.
    GfRGBA &operator *=(const GfRGBA &c) {
        _rgba = GfCompMult(_rgba, c._rgba);
        return *this;
    }

    /// Component-wise color division.
    GfRGBA &operator /=(const GfRGBA &c) {
        _rgba = GfCompDiv(_rgba, c._rgba);
        return *this;
    }

    /// Component-wise scalar multiplication.
    GfRGBA &operator *=(double d) {
        _rgba *= d;
        return *this;
    }

    /// Component-wise unary scalar division.
    GfRGBA &operator /=(double d) {
        _rgba /= d;
        return *this;
    }

    /// Returns component-wise multiplication of colors \p c1 and \p c2. Note
    /// that this is \em not a dot product operator, as it is with vectors.
    friend GfRGBA	operator *(const GfRGBA &c1, const GfRGBA &c2) {
        return GfRGBA(GfCompMult(c1._rgba, c2._rgba));
    }

    /// quotient operator.
    friend GfRGBA	operator /(const GfRGBA &c1, const GfRGBA &c2) {
        return GfRGBA(GfCompDiv(c1._rgba, c2._rgba));
    }

    /// Component-wise binary color addition operator.
    friend GfRGBA	operator +(const GfRGBA &c1, const GfRGBA &c2) {
        return GfRGBA(c1._rgba + c2._rgba);
    }

    /// Component-wise binary color subtraction operator.
    friend GfRGBA	operator -(const GfRGBA &c1, const GfRGBA &c2) {
        return GfRGBA(c1._rgba - c2._rgba);
    }

    /// Component-wise binary color/scalar multiplication operator.
    friend GfRGBA	operator *(const GfRGBA &c, double s) {
        return GfRGBA(c._rgba * s);
    }

    /// Component-wise binary color/scalar multiplication operator.
    friend GfRGBA	operator *(double s, const GfRGBA &c) {
        return c * s;
    }

    /// Component-wise binary color/scalar division operator.
    friend GfRGBA	operator /(const GfRGBA &c, double s) {
        return c * (1./s);
    }

    /// Tests for equality within a given tolerance, returning true if the
    /// difference between each component is less than \p tolerance.
    GF_API
    friend bool GfIsClose(const GfRGBA &v1, const GfRGBA &v2, double tolerance);

    /// \name Color-space conversions.
    ///
    /// The methods in this group convert between RGBA and other color spaces.
    ///
    ///@{

    /// Transform the color into an arbitrary space.
    GF_API
    GfRGBA Transform(const GfMatrix4d &m) const;

    /// Transform the color into an arbitrary space.
    GF_API
    friend GfRGBA operator *(const GfRGBA &c, const GfMatrix4d &m);

    /// Return the complement of a color.
    GfRGBA GetComplement() const {
        return GfRGBA(1) - *this;
    }

    /// Returns the equivalent of this color in HSV space 
    GF_API
    void GetHSV(float *hue, float *sat, float *value) const;

    /// Sets this RGB to the RGB equivalent of the given HSV color.
    GF_API
    void SetHSV(float hue, float sat, float value);

    ///@}
    
  private:
    /// Color storage.
    GfVec4f             _rgba;
};

// Friend functions must be declared.
GF_API
bool GfIsClose(const GfRGBA &v1, const GfRGBA &v2, double tolerance);
GF_API
GfRGBA operator *(const GfRGBA &c, const GfMatrix4d &m);

/// Output a GfRGBA color using the format (r, g, b, a).
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream& operator<<(std::ostream& out, const GfRGBA& c);

#endif // TF_RGBA_H
