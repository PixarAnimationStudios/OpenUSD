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
#ifndef USDLUX_GENERATED_LINKINGAPI_H
#define USDLUX_GENERATED_LINKINGAPI_H

/// \file usdLux/linkingAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;
class UsdGeomFaceSetAPI;

// -------------------------------------------------------------------------- //
// LINKINGAPI                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdLuxLinkingAPI
///
/// API schema for linking a light or light filter to subsets
/// of geometry for purposes of contributing illumination.
///
/// You probably don't want to construct these directly.  Instead,
/// the typical pattern is to request a linking API for a particular
/// purpose from a UsdLux object; ex: UsdLuxLight::GetLightLinkingAPI().
///
class UsdLuxLinkingAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdLuxLinkingAPI on UsdPrim \p prim .
    /// Equivalent to UsdLuxLinkingAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxLinkingAPI(const UsdPrim& prim=UsdPrim(),
                               const TfToken &name=TfToken())
        : UsdSchemaBase(prim)
        , _name(name)
    {
    }

    /// Construct a UsdLuxLinkingAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxLinkingAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxLinkingAPI(const UsdSchemaBase& schemaObj,
                              const TfToken &name=TfToken())
        : UsdSchemaBase(schemaObj)
        , _name(name)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxLinkingAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxLinkingAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxLinkingAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxLinkingAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLUX_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLUX_API
    virtual const TfType &_GetTfType() const;

public:
    /// An unordered map describing linkage of paths.
    /// This is a standalone value representing the linkage.
    /// Any path not present in this table is assumed to inherit its
    /// setting from the longest prefix path that is present. If there
    /// is no containing path, the path is assumed to be linked.
    typedef std::map<SdfPath, bool, SdfPath::FastLessThan> LinkMap;

    /// Return true if the given path (or ancestor) is linked by the linkMap.
    /// It is a coding error to pass a non-absolute path.
    USDLUX_API
    static bool DoesLinkPath(const LinkMap &linkMap, const SdfPath &path);

    /// Return true if the given UsdGeomFaceSetAPI (or ancestor) is linked
    /// by the linkMap.  Linking to faceSets is expressed as a target
    /// path to the faceSet's faceIndices property.
    USDLUX_API
    static bool DoesLinkFaceSet(const LinkMap &linkMap,
                                const UsdGeomFaceSetAPI &faceSet );

    /// Compute and return the link map, which can answer queries about
    /// linkage to particular paths.  Computing the link map once
    /// up front allows for more efficient repeated queries.
    /// See LinkMap for semantics.
    USDLUX_API
    LinkMap ComputeLinkMap() const;

    /// Set the underlying attributes to establish the given linkmap.
    USDLUX_API
    void SetLinkMap(const LinkMap &l) const;

private:
    UsdRelationship _GetIncludesRel(bool create=false) const;
    UsdRelationship _GetExcludesRel(bool create=false) const;
    UsdAttribute _GetIncludeByDefaultAttr(bool create = false) const;
    TfToken _GetCollectionPropertyName(const TfToken &baseName=TfToken()) const;

    // Name of the linkage.
    TfToken _name;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
