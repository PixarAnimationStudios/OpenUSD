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
#ifndef USD_GENERATED_APISCHEMABASE_H
#define USD_GENERATED_APISCHEMABASE_H

/// \file usd/apiSchemaBase.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
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

// -------------------------------------------------------------------------- //
// APISCHEMABASE                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdAPISchemaBase
///
/// The base class for all \em API schemas.
/// 
/// An API schema provides an interface to a prim's qualities, but does not
/// specify a typeName for the underlying prim. The prim's qualities include 
/// its inheritance structure, attributes, relationships etc. Since it cannot
/// provide a typeName, an API schema is considered to be non-concrete. 
/// 
/// To auto-generate an API schema using usdGenSchema, simply leave the 
/// typeName empty and make it inherit from "/APISchemaBase" or from another 
/// API schema. See UsdModelAPI, UsdClipsAPI and UsdCollectionAPI for examples.
/// 
/// API schemas are classified into applied and non-applied API schemas. 
/// The author of an API schema has to decide on the type of API schema 
/// at the time of its creation by setting customData['apiSchemaType'] in the 
/// schema definition (i.e. in  the associated primSpec inside the schema.usda 
/// file).  UsdAPISchemaBase implements methods that are used to record the 
/// application of an API schema on a USD prim.
/// 
/// If an API schema only provides an interface to set certain core bits of 
/// metadata (like UsdModelAPI, which sets model kind and UsdClipsAPI, which 
/// sets clips-related metadata) OR if the API schema can apply to any type of 
/// prim or only to a known fixed set of prim types OR if there is no use of 
/// recording the application of the API schema, in such cases, it would be 
/// better to make it a non-applied API schema. Examples of non-applied API 
/// schemas include UsdModelAPI, UsdClipsAPI, UsdShadeConnectableAPI and
/// UsdGeomPrimvarsAPI.
/// 
/// If there is a need to discover (or record) whether a prim contains or 
/// subscribes to a given API schema, it would be advantageous to make the API 
/// schema be "applied". In general, API schemas that add one or more properties 
/// to a prim should be tagged as applied API schemas. A public Apply() method 
/// is generated for applied API schemas by usdGenSchema. An applied API schema 
/// must be applied to a prim via a call to the generated Apply() method, for 
/// the schema object to evaluate to true when converted to a bool using the 
/// explicit bool conversion operator. Examples of applied API schemas include
/// UsdCollectionAPI, UsdGeomModelAPI and UsdGeomMotionAPI
/// 
/// \anchor UsdAPISchemaBase_SingleVsMultipleApply
/// \name Single vs. Multiple Apply API Schemas
/// 
/// Applied API schemas can further be classified into single-apply and 
/// multiple-apply API schemas. As the name suggests, a single-apply API schema 
/// can only be applied once to a prim. A multiple-apply API schema can be 
/// applied multiple times with different 'instanceName' values. An example of 
/// a multiple-apply API schema is UsdCollectionAPI, where the API schema is 
/// applied to a prim once for every collection owned by the prim. 
/// 
/// \note An applied API schema can only inherit from another applied API 
/// schema or directly from APISchemaBase. Similarly, a non-applied API schema 
/// can only inherit from a non-applied API Schema or directly from 
/// APISchemaBase. 'usdGenSchema' attempts to issue a warning if it detects 
/// an incompatibility.
/// 
/// \note A multiple-apply API schema may not inherit from a single-apply API 
/// schema and vice versa. 
/// 
/// \note When the bool-conversion operator is invoked on an applied API 
/// schema, it evaluates to true only if the application of the API schema has
/// been recorded on the prim via a call to the auto-generated Apply() method.
/// 
/// 
///
class UsdAPISchemaBase : public UsdSchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::AbstractBase;

    /// Construct a UsdAPISchemaBase on UsdPrim \p prim .
    /// Equivalent to UsdAPISchemaBase::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdAPISchemaBase(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdAPISchemaBase on the prim held by \p schemaObj .
    /// Should be preferred over UsdAPISchemaBase(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdAPISchemaBase(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USD_API
    virtual ~UsdAPISchemaBase() = 0;

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);


protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USD_API
    const TfType &_GetTfType() const override;

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

protected:
    /// Construct a multiple-apply UsdAPISchemaBase on UsdPrim \p prim with 
    /// the specified \p instanceName.
    explicit UsdAPISchemaBase(const UsdPrim& prim,
                              const TfToken &instanceName)
        : UsdSchemaBase(prim)
        , _instanceName(instanceName)
    {
    }

    /// Construct a multiple-apply UsdAPISchemaBase on the prim held by 
    /// \p schemaObj with the given \p instanceName.
    explicit UsdAPISchemaBase(const UsdSchemaBase& schemaObj,
                              const TfToken &instanceName)
        : UsdSchemaBase(schemaObj)
        , _instanceName(instanceName)
    {
    }

    /// Returns the instance name of the API schema object belonging to a 
    /// multiple-apply API schema.
    /// 
    /// The returned instance name will be empty for non-applied and 
    /// single-apply API schemas.
    const TfToken &_GetInstanceName() const { 
        return _instanceName;
    }

protected:        
    /// Check whether this APISchema object is valid for the currently held  
    /// prim. 
    /// 
    /// If this is an applied API schema, this returns true if the held prim 
    /// is valid and already has the API schema applied to it, along with the 
    /// instanceName (in the case of multiple-apply). The instanceName should 
    /// not be empty in the case of a multiple-apply API schema.
    /// 
    /// This check is performed when clients invoke the explicit bool conversion
    /// operator, implemented in UsdSchemaBase.
    /// 
    /// \sa UsdPrim::HasAPI()
    USD_API
    bool _IsCompatible() const override;

private:
    // The instance name associated with this schema object, if it is a 
    // multiple-apply API schema. For example, in the case of UsdCollectionAPI, 
    // this will hold the name of the collection.
    TfToken _instanceName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
