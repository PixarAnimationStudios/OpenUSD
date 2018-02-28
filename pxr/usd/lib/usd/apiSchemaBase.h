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
/// typeName empty and don't add an inherits for it. See UsdModelAPI and 
/// UsdCollectionAPI for examples.
/// 
/// UsdAPISchemaBase implements methods that are used to record the application
/// of an API schema on a USD prim.
/// 
///
class UsdAPISchemaBase : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = false;

    /// Compile-time constant indicating whether or not this class represents a 
    /// multiple-apply API schema. Mutiple-apply API schemas can be applied 
    /// to the same prim multiple times with different instance names. 
    static const bool IsMultipleApply = false;

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
    virtual ~UsdAPISchemaBase();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdAPISchemaBase holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdAPISchemaBase(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USD_API
    static UsdAPISchemaBase
    Get(const UsdStagePtr &stage, const SdfPath &path);

private:

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "APISchemaBase" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdAPISchemaBase object is returned upon success. 
    /// An invalid (or empty) UsdAPISchemaBase object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_ApplyAPISchema() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    static UsdAPISchemaBase 
    _Apply(const UsdPrim &prim);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USD_API
    virtual const TfType &_GetTfType() const;

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
    /// 
    /// Helper method to apply a <b>single-apply</b> API schema with the given 
    /// schema name \p apiSchemaName' and C++ type 'APISchemaType'. The schema 
    /// is applied on the given \p prim in the current edit target. 
    /// 
    /// This information is stored by adding \p apiSchemaName value to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// A valid schema object of type \em APISchemaType is returned upon 
    /// success. 
    /// 
    /// A coding error is issued and an invalid schema object is returned if 
    /// <li>if \p prim is invalid or is an instance proxy prim or is contained 
    /// within an instance master OR</li>
    /// <li>\p apiSchemaName cannot be added to the apiSchemas listOp 
    /// metadata.</li></ul>
    /// 
    /// A run-time error is issued and an invalid schema object is returned 
    /// if the given prim is valid, but cannot be reached or overridden in the 
    /// current edit target. 
    /// 
    template <typename APISchemaType>
    static APISchemaType _ApplyAPISchema(
        const UsdPrim &prim, 
        const TfToken &apiSchemaName) 
    {
        return APISchemaType(_ApplyAPISchemaImpl(prim, apiSchemaName));
    }

    /// Helper method to apply a </b>multiple-apply</b> API schema with the 
    /// given schema name \p apiSchemaName', C++ type 'APISchemaType' and 
    /// instance name \p instanceName. The schema is applied on the given 
    /// \p prim in the current edit target. 
    /// 
    /// This information is stored in the token-valued, listOp metadata
    /// \em apiSchemas on the prim. For example, if \p apiSchemaName is
    /// 'CollectionAPI' and \p instanceName is 'plasticStuff', the name 
    /// 'CollectionAPI:plasticStuff' is added to 'apiSchemas' listOp metadata. 
    /// 
    /// A valid schema object of type \em APISchemaType is returned upon 
    /// success. 
    /// 
    /// A coding error is issued and an invalid schema object is returned if 
    /// <li>the \p prim is invalid or is an instance proxy prim or is contained 
    /// within an instance master OR</li>
    /// <li>\p instanceName is empty OR</li>
    /// <li><i>apiSchemaName:instanceName</i> cannot be added to the apiSchemas 
    /// listOp metadata.</li></ul>
    /// 
    /// A run-time error is issued and an invalid schema object is returned 
    /// if the given prim is valid, but cannot be reached or overridden in the 
    /// current edit target. 
    /// 
    template <typename APISchemaType>
    static APISchemaType _MultipleApplyAPISchema(
        const UsdPrim &prim, 
        const TfToken &apiSchemaName,
        const TfToken &instanceName) 
    {
        if (instanceName.IsEmpty()) {
            TF_CODING_ERROR("Instance name is empty!");
            return APISchemaType();
        }

        TfToken apiName(SdfPath::JoinIdentifier(apiSchemaName, instanceName));
        return APISchemaType(_ApplyAPISchemaImpl(prim, apiName), instanceName);
    }

private:
    // Helper method for adding 'apiName' to the apiSchemas metadata on the 
    // prim at 'path' on the given 'stage'.
    USD_API
    static UsdPrim _ApplyAPISchemaImpl(
        const UsdPrim &prim,
        const TfToken &apiName);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
