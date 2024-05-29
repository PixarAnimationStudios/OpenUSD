//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// LayerOffset.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/timeCode.h"
#include "pxr/base/gf/math.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/type.h"

#include <climits>
#include <limits>
#include <ostream>
#include <vector>

#define EPSILON (1e-6)

PXR_NAMESPACE_OPEN_SCOPE

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

SdfTimeCode
SdfLayerOffset::operator*(const SdfTimeCode &rhs) const
{
    return SdfTimeCode( (*this) * double(rhs) );
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
    return TfHash::Combine(
        _offset,
        _scale
    );
}

std::ostream & operator<<( std::ostream &out,
                           const SdfLayerOffset &layerOffset )
{
    return out << "SdfLayerOffset("
        << layerOffset.GetOffset() << ", "
        << layerOffset.GetScale() << ")";
}

PXR_NAMESPACE_CLOSE_SCOPE
