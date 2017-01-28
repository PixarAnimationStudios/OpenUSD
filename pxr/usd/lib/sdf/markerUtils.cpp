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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/markerUtils.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// Sdf_MarkerUtilsPolicy<Spec>
// Helper policy class for specifying behaviors that are dependent on the
// owning spec type.
//

template <class Spec>
class Sdf_MarkerUtilsPolicy
{
public:
    static TfToken GetChildFieldKey();
    static const char* GetMarkerDescription();
    static SdfAllowed IsValidConnectionPath(const SdfPath& path);
};

//
// Sdf_MarkerUtils
//

template <class Spec>
SdfPathVector 
Sdf_MarkerUtils<Spec>::GetMarkerPaths(const Spec& owner)
{
    SdfPathVector paths;

    const SdfLayerHandle layer = owner.GetLayer();
    const SdfPathVector children = owner.template GetFieldAs<SdfPathVector>(
        _MarkerPolicy::GetChildFieldKey());

    TF_FOR_ALL(path, children) {
        const SdfPath targetSpecPath = owner.GetPath().AppendTarget(*path);
        if (layer->HasField(targetSpecPath, SdfFieldKeys->Marker)) {
            paths.push_back(*path);
        }
    }

    return paths;
}

template <class Spec>
std::string 
Sdf_MarkerUtils<Spec>::GetMarker(
    const Spec& owner, const SdfPath& connectionPath)
{
    if (connectionPath.IsEmpty()) {
        return std::string();
    }

    const SdfLayerHandle layer = owner.GetLayer();
    const SdfPath specPath = owner.GetPath().AppendTarget(connectionPath);
    return layer->GetFieldAs<std::string>(specPath, SdfFieldKeys->Marker);
}

template <class Spec>
void 
Sdf_MarkerUtils<Spec>::SetMarker(
    Spec* owner, const SdfPath& connectionPath, 
    const std::string& marker)
{
    if (marker.empty()) {
        ClearMarker(owner, connectionPath);
        return;
    }

    SdfAllowed allowed;
    if (!owner->PermissionToEdit()) {
        allowed = SdfAllowed("Permission denied.");
    }
    else {
        allowed = _MarkerPolicy::IsValidConnectionPath(connectionPath);
    }

    if (!allowed) {
        TF_CODING_ERROR("Set %s: %s", 
                        _MarkerPolicy::GetMarkerDescription(),
                        allowed.GetWhyNot().c_str());
        return;
    }

    SdfSpecHandle connectionSpec = 
        owner->_FindOrCreateChildSpecForMarker(connectionPath);
    if (!connectionSpec) {
        TF_CODING_ERROR("Set %s: Could not find or create child for marker.",
            _MarkerPolicy::GetMarkerDescription());
        return;
    }

    connectionSpec->SetField(SdfFieldKeys->Marker, marker);
}

template <class Spec>
void 
Sdf_MarkerUtils<Spec>::SetMarkers(
    Spec* owner, const MarkerMap& markers)
{
    // Explicitly check permission here to ensure that any editing operation
    // (even no-ops) trigger an error.
    if (!owner->PermissionToEdit()) {
        TF_CODING_ERROR("Set %s: Permission denied", 
                        _MarkerPolicy::GetMarkerDescription());
        return;
    }

    // Validate all connection paths; we don't want to author any changes if
    // an invalid path exists in the map.
    TF_FOR_ALL(newMarker, markers) {
        const SdfAllowed isValidPath = 
            _MarkerPolicy::IsValidConnectionPath(newMarker->first);
        if (!isValidPath) {
            TF_CODING_ERROR("Set %s: %s",
                            _MarkerPolicy::GetMarkerDescription(),
                            isValidPath.GetWhyNot().c_str());
            return;
        }
    }

    // Replace all markers; clear out all current markers and add in new 
    // markers from the given map.
    SdfChangeBlock block;

    const SdfPathVector markerPaths = GetMarkerPaths(*owner);
    TF_FOR_ALL(oldPath, markerPaths) {
        if (markers.find(*oldPath) == markers.end()) {
            ClearMarker(owner, *oldPath);
        }
    }

    TF_FOR_ALL(newMarker, markers) {
        SetMarker(owner, newMarker->first, newMarker->second);
    }
}

template <class Spec>
void 
Sdf_MarkerUtils<Spec>::ClearMarker(
    Spec* owner, const SdfPath& connectionPath)
{
    if (!owner->PermissionToEdit()) {
        TF_CODING_ERROR("Clear %s: Permission denied.", 
                        _MarkerPolicy::GetMarkerDescription());
        return;
    }

    if (connectionPath.IsEmpty()) {
        return;
    }

    owner->GetLayer()->EraseField(
        owner->GetPath().AppendTarget(connectionPath), SdfFieldKeys->Marker);
}

//
// Explicit instantiation of Sdf_MarkerUtils for attribute connections.
//

#include "pxr/usd/sdf/attributeSpec.h"

template <>
class Sdf_MarkerUtilsPolicy<SdfAttributeSpec>
{
public:
    static TfToken GetChildFieldKey()
    { 
        return SdfChildrenKeys->ConnectionChildren;
    }

    static const char* GetMarkerDescription()
    {
        return "connection marker";
    }

    static SdfAllowed IsValidConnectionPath(const SdfPath& path)
    {
        return SdfSchema::IsValidAttributeConnectionPath(path);
    }
};

template class Sdf_MarkerUtils<SdfAttributeSpec>;

//
// Explicit instantiation of Sdf_MarkerUtils for relationship targets
//

#include "pxr/usd/sdf/relationshipSpec.h"

template <>
class Sdf_MarkerUtilsPolicy<SdfRelationshipSpec>
{
public:
    static TfToken GetChildFieldKey()
    { 
        return SdfChildrenKeys->RelationshipTargetChildren;
    }

    static const char* GetMarkerDescription()
    {
        return "target marker";
    }

    static SdfAllowed IsValidConnectionPath(const SdfPath& path)
    {
        return SdfSchema::IsValidRelationshipTargetPath(path);
    }
};

template class Sdf_MarkerUtils<SdfRelationshipSpec>;

PXR_NAMESPACE_CLOSE_SCOPE
