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
///
/// LayerOffset.cpp

#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/base/gf/math.h"

#include "pxr/base/tf/type.h"

#include <boost/functional/hash/hash.hpp>

#include <climits>
#include <limits>
#include <ostream>
#include <vector>

#define EPSILON (1e-6)

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<SdfLayerOffset>();
    TfType::Define<std::vector<SdfLayerOffset>>();
}

bool 
SdfLayerOffset::IsIdentity() const
{
    // Construct a static instance to avoid a default construction on every call
    // to SdfLayerOffset::IsIdentity().
    static SdfLayerOffset identityOffset;

    // Use operator==() for fuzzy compare (i.e. GfIsClose).
    return *this == identityOffset;
}

SdfLayerOffset::SdfLayerOffset(double offset, double scale) :
    _offset(offset),
    _scale(scale)
{
}

bool
SdfLayerOffset::IsValid() const
{
    return std::isfinite(_offset) && std::isfinite(_scale);
}

SdfLayerOffset SdfLayerOffset::GetInverse() const
{
    if (IsIdentity()) {
        return *this;
    }

    double newScale;
    if (_scale != 0.0) {
        newScale = 1.0 / _scale;
    } else {
        newScale = std::numeric_limits<double>::infinity();
    }
    return SdfLayerOffset( -_offset * newScale, newScale );
}

SdfLayerOffset
SdfLayerOffset::operator*(const SdfLayerOffset &rhs) const
{
    return SdfLayerOffset( _scale * rhs._offset + _offset,
                           _scale * rhs._scale );
}

double
SdfLayerOffset::operator*(double rhs) const
{
    return ( rhs * _scale + _offset );
}

bool
SdfLayerOffset::operator==(const SdfLayerOffset &rhs) const
{
    // Use EPSILON so that 0 == -0, for example.
    return (!IsValid() && !rhs.IsValid()) ||
           (GfIsClose(_offset, rhs._offset, EPSILON) &&
            GfIsClose(_scale, rhs._scale, EPSILON));
}

bool
SdfLayerOffset::operator<(const SdfLayerOffset &rhs) const
{
    if (!IsValid()) {
        return false;
    }
    if (!rhs.IsValid()) {
        return true;
    }
    if (GfIsClose(_scale, rhs._scale, EPSILON)) {
        if (GfIsClose(_offset, rhs._offset, EPSILON)) {
            return false;
        }
        else {
            return _offset < rhs._offset;
        }
    }
    else {
        return _scale < rhs._scale;
    }
}

size_t
SdfLayerOffset::GetHash() const
{
    size_t hash = 0;
    boost::hash_combine(hash, _offset);
    boost::hash_combine(hash, _scale);
    return hash;
}

std::ostream & operator<<( std::ostream &out,
                           const SdfLayerOffset &layerOffset )
{
    return out << "SdfLayerOffset("
        << layerOffset.GetOffset() << ", "
        << layerOffset.GetScale() << ")";
}
