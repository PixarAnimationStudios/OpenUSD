//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PAYLOAD_H
#define PXR_USD_SDF_PAYLOAD_H

/// \file sdf/payload.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hash.h"

#include <iosfwd>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPayload;

typedef std::vector<SdfPayload> SdfPayloadVector;

/// \class SdfPayload
///
/// Represents a payload and all its meta data.
///
/// A payload represents a prim reference to an external layer.  A payload
/// is similar to a prim reference (see SdfReference) with the major
/// difference that payloads are explicitly loaded by the user.
///
/// Unloaded payloads represent a boundary that lazy composition and
/// system behaviors will not traverse across, providing a user-visible
/// way to manage the working set of the scene.
///
class SdfPayload {
public:
    /// Create a payload. See SdfAssetPath for what characters are valid in \p
    /// assetPath.  If \p assetPath contains invalid characters, issue an error
    /// and set this payload's asset path to the empty asset path.
    ///
    SDF_API
    SdfPayload(
        const std::string &assetPath = std::string(),
        const SdfPath &primPath = SdfPath(),
        const SdfLayerOffset &layerOffset = SdfLayerOffset());

    /// Returns the asset path of the layer that the payload uses.
    const std::string &GetAssetPath() const {
        return _assetPath;
    }

    /// Sets a new asset path for the layer the payload uses.  See SdfAssetPath
    /// for what characters are valid in \p assetPath.  If \p assetPath contains
    /// invalid characters, issue an error and set this payload's asset path to
    /// the empty asset path.
    void SetAssetPath(const std::string &assetPath) {
        // Go through SdfAssetPath() to raise an error if \p assetPath contains
        // illegal characters (i.e. control characters).
        _assetPath = SdfAssetPath(assetPath).GetAssetPath();
    }

    /// Returns the scene path of the prim for the payload.
    const SdfPath &GetPrimPath() const {
        return _primPath;
    }

    /// Sets a new prim path for the prim that the payload uses.
    void SetPrimPath(const SdfPath &primPath) {
        _primPath = primPath;
    }

    /// Returns the layer offset associated with the payload.
    const SdfLayerOffset &GetLayerOffset() const {
        return _layerOffset;
    }

    /// Sets a new layer offset.
    void SetLayerOffset(const SdfLayerOffset &layerOffset) {
        _layerOffset = layerOffset;
    }

    /// Returns whether this payload equals \a rhs.
    SDF_API bool operator==(const SdfPayload &rhs) const;

    /// \sa SdfPayload::operator==
    bool operator!=(const SdfPayload& rhs) const {
        return !(*this == rhs);
    }

    /// Returns whether this payload is less than \a rhs.
    /// The meaning of less than is arbitrary but stable.
    SDF_API bool operator<(const SdfPayload &rhs) const;

    /// \sa SdfPayload::operator<
    bool operator>(const SdfPayload& rhs) const {
        return rhs < *this;
    }

    /// \sa SdfPayload::operator<
    bool operator<=(const SdfPayload& rhs) const {
        return !(rhs < *this);
    }

    /// \sa SdfPayload::operator<
    bool operator>=(const SdfPayload& rhs) const {
        return !(*this < rhs);
    }

private:
    friend inline size_t hash_value(const SdfPayload &p) {
        return TfHash::Combine(
            p._assetPath,
            p._primPath,
            p._layerOffset
        );
    }

    // The asset path to the external layer.
    std::string _assetPath;

    // The root prim path to the referenced prim in the external layer.
    SdfPath _primPath;

    // The layer offset to transform time.
    SdfLayerOffset _layerOffset;
};

/// Writes the string representation of \a SdfPayload to \a out.
SDF_API
std::ostream & operator<<(std::ostream &out, const SdfPayload &payload);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
