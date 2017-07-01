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
#ifndef USDLUX_GENERATED_LISTAPI_H
#define USDLUX_GENERATED_LISTAPI_H

/// \file usdLux/listAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LISTAPI                                                                    //
// -------------------------------------------------------------------------- //

/// \class UsdLuxListAPI
///
/// API schema to support discovery and publishing of lights in a scene.
/// 
/// \section UsdLuxListAPI_Discovery Discovering Lights via Traversal
/// 
/// To motivate this, first consider what is required to discover all
/// lights in a scene.  We must load all payloads and traverse all prims:
/// 
/// \code
/// 01  SdfPathVector result;
/// 02  
/// 03  // Load everything on the stage so we can find all lights,
/// 04  // including those inside payloads
/// 05  stage->Load();
/// 06  
/// 07  // Traverse all prims, checking if they are of type UsdLuxLight
/// 08  // (Note: ignoring instancing and a few other things for simplicity)
/// 09  UsdPrimRange range = stage->Traverse();
/// 10  for (auto i = range.begin(); i != range.end(); ++i) {
/// 11     if (i->IsA<UsdLuxLight>()) {
/// 12         result.push_back(i->GetPath());
/// 13     }
/// 14  }
/// \endcode
/// 
/// This traversal -- expanded to handle instancing, prim
/// activation, and similar concerns -- is the first and
/// simplest thing that UsdLuxListAPI provides. By invoking
/// UsdLuxListAPI::ComputeLightList() on a schema bound to a prim, with
/// StoredListIgnore as the argument, all lights within that prim and
/// its active, non-abstract descendents will be returned.
/// 
/// \section UsdLuxListAPI_LightList Publishing a Cached Light List
/// 
/// Next consider a USD consumer that wants to defer loading payloads
/// where possible, but needs a full list of lights, and is willing to do
/// up-front computation and caching to achieve that.
/// As one approach, we might pre-discover lights within payloads and
/// publish that list on the outside of the payload arc, where it is
/// discoverable without loading the payload.  In scene description:
/// 
/// \code
/// 01  def Model "BigSetWithLights" (
/// 02      payload = @BigSetWithLights.usd@   // Lots of stuff inside here
/// 03  ) {
/// 04      // Pre-computed list of lights inside payload
/// 05      rel lightList = [
/// 06          <./Lights/light_1>,
/// 07          <./Lights/light_2>,
/// 08          ...
/// 09      ]
/// 10      bool lightList:isValid = true
/// 11  }
/// \endcode
/// 
/// This list can be established with UsdLuxListAPI::StoreLightList().
/// With this cache in place, UsdLuxListAPI::ComputeLightList() --
/// given the StoredListConsult argument this time -- can terminate its
/// recursion early whenever it discovers the lightList relationship,
/// using the stored list of targets instead.
/// 
/// To make use of this to find and load all lights in a large scene
/// where the lightLists have been published, loading only the
/// payloads required:
/// 
/// \code
/// 01  // Find and load all light paths, including lights published
/// 02  // in a lightList.
/// 03  for (const auto &child: stage->GetPseudoRoot().GetChildren()) {
/// 04      stage.LoadAndUnload(
/// 05          /* load */ UsdLuxListAPI(child).ComputeLightList(true),
/// 06          /* unload */ SdfPathSet());
/// 07  }
/// \endcode
/// 
/// \section UsdLuxListAPI_Invalidation Invalidating a Light List
/// 
/// While ComputeLightList() allows code to choose to disregard
/// any stored lightlist, it can be useful to do the same in
/// scene description, invalidating the stored list for a prim.
/// That can be achieved by calling InvalidateLightList(), which
/// sets the \c lightList:isValid attribute to false.  A separate
/// attribute is used because it is valid for a relationship to
/// have an empty list of targets.
/// 
/// \section UsdLuxListAPI_Instancing Instancing
/// 
/// If instances are present, UsdLuxListAPI::ComputeLightList() will
/// return the instance-unqiue paths to any lights discovered within
/// those instances.  Lights within a UsdGeomPointInstancer will
/// not be returned, however, since they cannot be referred to
/// solely via paths.
/// 
///
class UsdLuxListAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdLuxListAPI on UsdPrim \p prim .
    /// Equivalent to UsdLuxListAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxListAPI(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdLuxListAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxListAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxListAPI(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxListAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxListAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxListAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxListAPI
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
    // --------------------------------------------------------------------- //
    // LIGHTLISTISVALID 
    // --------------------------------------------------------------------- //
    /// Bool indicating if the lightList targets should be
    /// considered valid.
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDLUX_API
    UsdAttribute GetLightListIsValidAttr() const;

    /// See GetLightListIsValidAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateLightListIsValidAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LIGHTLIST 
    // --------------------------------------------------------------------- //
    /// Relationship to lights in the scene.
    ///
    USDLUX_API
    UsdRelationship GetLightListRel() const;

    /// See GetLightListRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDLUX_API
    UsdRelationship CreateLightListRel() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    enum StoredListBehavior {
        StoredListConsult,
        StoredListIgnore,
    };

    /// Computes and returns the list of lights in the stage, considering
    /// this prim and any active, non-abstract descendents.
    ///
    /// If listBehavior is StoredListConsult, this will first check the
    /// light list relationship on each prim, as well as the associated
    /// isValid attribute.  If there is a valid opinion, the stored
    /// list will be used in place of further recursion below that prim.
    /// 
    /// If instances are present, ComputeLightList() will return the
    /// instance-unqiue paths to any lights discovered within those
    /// instances. Lights within a UsdGeomPointInstancer will not be
    /// returned, however, since they cannot be referred to solely via
    /// paths.
    USDLUX_API
    SdfPathSet ComputeLightList(StoredListBehavior listBehavior) const;

    /// Store the given paths as the lightlist for this prim.
    /// Paths that do not have this prim's path as a prefix
    /// will be silently ignored.
    USDLUX_API
    void StoreLightList(const SdfPathSet &) const;

    /// Mark any stored lightlist as invalid, by setting the
    /// lightList:isValid attribute to false.
    USDLUX_API
    void InvalidateLightList() const;

    /// Return true if the prim has a valid lightList.
    USDLUX_API
    bool IsLightListValid() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
