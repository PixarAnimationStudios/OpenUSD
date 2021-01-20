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
#ifndef PXR_USD_SDF_LAYER_OFFSET_H
#define PXR_USD_SDF_LAYER_OFFSET_H

/// \file sdf/layerOffset.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include <boost/operators.hpp>
#include <iosfwd>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfTimeCode;

/// \class SdfLayerOffset 
///
/// Represents a time offset and scale between layers.
///
/// The SdfLayerOffset class is an affine transform, providing both a scale and
/// a translate.  It supports vector algebra semantics for composing
/// SdfLayerOffsets together via multiplication.  The SdfLayerOffset class is 
/// unitless: it does not refer to seconds or frames.
///
/// For example, suppose layer A uses layer B, with an offset of X:  
/// when bringing animation from B into A, you first apply the scale of X, and 
/// then the offset.  Suppose you have a scale of 2 and an offset of 24: 
/// first multiply B's frame numbers by 2, and then add 24.  The animation from 
/// B as seen in A will take twice as long and start 24 frames later.
///
/// Offsets are typically used in either sublayers or prim references. For more 
/// information, see the SetSubLayerOffset() method of the SdfLayer class (the 
/// subLayerOffsets property in Python), as well as the SetReference() and 
/// GetReferenceLayerOffset() methods (the latter is the referenceLayerOffset 
/// property in Python) of the SdfPrimSpec class.
///
class SdfLayerOffset : public boost::totally_ordered<SdfLayerOffset>
{
public:
    /// \name Constructors
    /// @{

    /// Constructs a new SdfLayerOffset instance.
    SDF_API
    explicit SdfLayerOffset(double offset = 0.0, double scale = 1.0);

    /// @}

    /// \name Accessors
    /// @{

    /// Returns the time offset.
    double GetOffset() const { return _offset; }
    
    /// Returns the time scale factor.
    double GetScale() const { return _scale; }

    /// Sets the time offset.
    void SetOffset(double newOffset) { _offset = newOffset; }
    
    /// Sets the time scale factor.
    void SetScale(double newScale) { _scale = newScale; }

    /// Returns \c true if this is an identity transformation, with
    /// an offset of 0.0 and a scale of 1.0.
    SDF_API
    bool IsIdentity() const;

    /// Returns \c true if this offset is valid, i.e. both the offset and
    /// scale are finite (not infinite or NaN).  Note that a valid layer
    /// offset's inverse may be invalid.
    SDF_API
    bool IsValid() const;

    /// Gets the inverse offset, which performs the opposite transformation.
    SDF_API
    SdfLayerOffset GetInverse() const;

    /// \name Hashing
    /// @{

    /// Returns hash for this offset.
    SDF_API
    size_t GetHash() const;

    /// Hash functor for hash maps and sets.
    struct Hash {
        size_t operator()(const SdfLayerOffset &offset) const {
            return offset.GetHash();
        }
    };

    friend inline size_t hash_value(const SdfLayerOffset &offset) {
        return offset.GetHash();
    }

    /// @}

    /// \name Operators
    /// @{

    /// Returns whether the offsets are equal.
    SDF_API
    bool operator==(const SdfLayerOffset &rhs) const;

    /// Returns whether this offset is less than another.  The meaning
    /// of less than is somewhat arbitrary.
    SDF_API
    bool operator<(const SdfLayerOffset &rhs) const;

    /// Composes this with the offset \e rhs, such that the resulting
    /// offset is equivalent to first applying \e rhs and then \e *this.
    SDF_API
    SdfLayerOffset operator*(const SdfLayerOffset &rhs) const;

    /// Applies the offset to the given value.
    SDF_API
    double operator*(double rhs) const;

    /// Applies the offset to the given value.
    SDF_API
    SdfTimeCode operator*(const SdfTimeCode &rhs) const;

    /// @}

private:
    double _offset;
    double _scale;
};

typedef std::vector<SdfLayerOffset> SdfLayerOffsetVector;

///
/// Writes the string representation of \a SdfLayerOffset to \a out.
SDF_API
std::ostream & operator<<( std::ostream &out,
                           const SdfLayerOffset &layerOffset );

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_LAYER_OFFSET_H
