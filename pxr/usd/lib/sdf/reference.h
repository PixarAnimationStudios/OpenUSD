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
#ifndef SDF_REFERENCE_H
#define SDF_REFERENCE_H

/// \file sdf/reference.h

#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/tf/pathUtils.h"

#include <boost/operators.hpp>

#include <iosfwd>
#include <string>
#include <vector>

class SdfReference;

typedef std::vector<SdfReference> SdfReferenceVector;

/// \class SdfReference
///
/// Represents a reference and all its meta data.
///
/// A reference is expressed on a prim in a given layer and it identifies a
/// root prim in a layer.  All opinions in the namespace hierarchy
/// under the referenced root prim will be composed with the opinions in the
/// namespace hierarchy under the referencing prim.
///
/// The asset path specifies the layer stack being referenced.  If this
/// asset path is non-empty, this reference is considered an 'external' 
/// reference to the layer stack rooted at the specified layer.  If this
/// is empty, this reference is considered an 'internal' reference to the
/// layer stack containing (but not necessarily rooted at) the layer where
/// the reference is authored.
///
/// The prim path specifies the prim in the referenced layer stack from
/// which opinions will be composed.  If this prim path is empty, it will
/// be considered a reference to the default prim specified in the root layer
/// of the referenced layer stack -- see SdfLayer::GetDefaultPrim.
///
/// The meta data for a reference is its layer offset and custom data.  The
/// layer offset is an affine transformation applied to all anim splines in
/// the referenced prim's namespace hierarchy, see SdfLayerOffset for details.
/// Custom data is for use by plugins or other non-tools supplied extensions
/// that need to be able to store data associated with references.
///
class SdfReference : boost::totally_ordered<SdfReference> {
public:
    /// Creates a reference with all its meta data.  The default
    /// reference is an internal reference to the default prim.
    ///
	SDF_API SdfReference(
        const std::string &assetPath = std::string(),
        const SdfPath &primPath = SdfPath(),
        const SdfLayerOffset &layerOffset = SdfLayerOffset(),
        const VtDictionary &customData = VtDictionary());

    /// Returns the asset path to the root layer of the referenced layer
    /// stack.  This will be empty in the case of an internal reference.
    ///
    const std::string &GetAssetPath() const {
        return _assetPath;
    }

    /// Sets the asset path for the root layer of the referenced layer stack.  
    /// This may be set to an empty string to specify an internal reference.
    ///
    void SetAssetPath(const std::string &assetPath) {
        _assetPath = assetPath;
    }

    /// Returns the path of the referenced prim.
    /// This will be empty if the referenced prim is the default prim specified
    /// in the referenced layer stack.
    ///
    const SdfPath &GetPrimPath() const {
        return _primPath;
    }

    /// Sets the path of the referenced prim.
    /// This may be set to an empty path to specify a reference to the default
    /// prim in the referenced layer stack.
    ///
    void SetPrimPath(const SdfPath &primPath) {
        _primPath = primPath;
    }

    /// Returns the layer offset associated with the reference.
    ///
    const SdfLayerOffset &GetLayerOffset() const {
        return _layerOffset;
    }

    /// Sets a new layer offset.
    ///
    void SetLayerOffset(const SdfLayerOffset &layerOffset) {
        _layerOffset = layerOffset;
    }

    /// Returns the custom data associated with the reference.
    ///
    const VtDictionary &GetCustomData() const {
        return _customData;
    }

    /// Sets the custom data associated with the reference.
    ///
    void SetCustomData(const VtDictionary &customData) {
        _customData = customData;
    }

    /// Sets a custom data entry for the reference.
    ///
    /// If \a value is empty, then this removes the given custom data entry.
    ///
	SDF_API void SetCustomData(const std::string &name, const VtValue &value);

    /// Swaps the custom data dictionary for this reference.
    void SwapCustomData(VtDictionary &customData) {
        _customData.swap(customData);
    }

    friend inline size_t hash_value(const SdfReference &r) {
        size_t h = 0;
        boost::hash_combine(h, r._assetPath);
        boost::hash_combine(h, r._primPath);
        boost::hash_combine(h, r._layerOffset);
        boost::hash_combine(h, r._customData);
        return h;
    }

    /// Returns whether this reference equals \a rhs.
	SDF_API bool operator==(const SdfReference &rhs) const;

    /// Returns whether this reference is less than \a rhs.  The meaning
    /// of less than is somewhat arbitrary.
	SDF_API bool operator<(const SdfReference &rhs) const;

    /// Struct that defines equality of SdfReferences based on their
    /// identity (the asset path and prim path).
    ///
    struct IdentityEqual {
        bool operator()(const SdfReference &lhs, const SdfReference &rhs) const {
            return lhs._assetPath == rhs._assetPath and
                   lhs._primPath  == rhs._primPath;
        }
    };

    /// Struct that defines a strict weak ordering of SdfReferences based on
    /// their identity (the asset path and prim path).
    ///
    struct IdentityLessThan {
        bool operator()(const SdfReference &lhs, const SdfReference &rhs) const {
            return lhs._assetPath <  rhs._assetPath or
                  (lhs._assetPath == rhs._assetPath and
                   lhs._primPath  <  rhs._primPath);
        }
    };

private:
    // The asset path to the external layer.
    std::string _assetPath;

    // The root prim path to the referenced prim in the external layer.
    SdfPath _primPath;

    // The layer offset to transform time.
    SdfLayerOffset _layerOffset;

    // The custom data associated with the reference.
    VtDictionary _customData;
};

/// Convenience function to find the index of the reference in \a references
/// that has the same identity as the given reference \a referenceId.
///
/// A reference's identity is given by its asset path and prim path alone
/// (i.e. the layer offset and custom data is ignored).
///
/// If no reference with the same identity exists in \a reference, -1 is
/// returned.  If more than one reference with the same identity exist in
/// \a references the index of the first one is returned.
///
SDF_API int SdfFindReferenceByIdentity(
    const SdfReferenceVector &references,
    const SdfReference &referenceId);

/// Writes the string representation of \a SdfReference to \a out.
SDF_API std::ostream & operator<<( std::ostream &out,
                           const SdfReference &reference );

#endif // SDF_REFERENCE_H
