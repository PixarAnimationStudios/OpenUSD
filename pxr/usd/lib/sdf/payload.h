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
#ifndef SDF_PAYLOAD_H
#define SDF_PAYLOAD_H

/// \file sdf/payload.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/dictionary.h"

#include <boost/operators.hpp>

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
class SdfPayload : boost::totally_ordered<SdfPayload> {
public:
    /// Creates a payload.
    ///
    SdfPayload(
        const std::string &assetPath = std::string(),
        const SdfPath &primPath = SdfPath());

    /// Returns the asset path of the layer that the payload uses.
    const std::string &GetAssetPath() const {
        return _assetPath;
    }

    /// Sets a new asset path for the layer the payload uses.
    void SetAssetPath(const std::string &assetPath) {
        _assetPath = assetPath;
    }

    /// Returns the scene path of the prim for the payload.
    const SdfPath &GetPrimPath() const {
        return _primPath;
    }

    /// Sets a new prim path for the prim that the payload uses.
    void SetPrimPath(const SdfPath &primPath) {
        _primPath = primPath;
    }

    /// Bool conversion; true if the payload is not empty.
    operator bool() const;

    /// Returns whether this payload equals \a rhs.
    bool operator==(const SdfPayload &rhs) const;

    /// Returns whether this payload is less than \a rhs.
    /// The meaning of less than is arbitrary but stable.
    bool operator<(const SdfPayload &rhs) const;

private:
    friend inline size_t hash_value(const SdfPayload &p) {
        size_t h = 0;
        boost::hash_combine(h, p._assetPath);
        boost::hash_combine(h, p._primPath);
        return h;
    }

    // The asset path to the external layer.
    std::string _assetPath;

    // The root prim path to the referenced prim in the external layer.
    SdfPath _primPath;
};

/// Writes the string representation of \a SdfPayload to \a out.
std::ostream & operator<<(std::ostream &out, const SdfPayload &payload);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
