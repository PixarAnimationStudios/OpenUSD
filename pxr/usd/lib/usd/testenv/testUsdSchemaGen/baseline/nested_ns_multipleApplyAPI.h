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
#ifndef USDCONTRIVED_GENERATED_MULTIPLEAPPLYAPI_H
#define USDCONTRIVED_GENERATED_MULTIPLEAPPLYAPI_H

/// \file usdContrived/multipleApplyAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdContrived/api.h"
#include "pxr/usd/usd/aPISchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

namespace foo { namespace bar { namespace baz {

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// MULTIPLEAPPLYAPI                                                           //
// -------------------------------------------------------------------------- //

/// \class UsdContrivedMultipleApplyAPI
///
///
class UsdContrivedMultipleApplyAPI : public UsdAPISchemaBase
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

    /// Compile-time constant indicating whether or not this class represents an 
    /// applied API schema, i.e. an API schema that has to be applied to a prim
    /// with a call to auto-generated Apply() method before any schema 
    /// properties are authored.
    static const bool IsApplied = true;
    
    /// Compile-time constant indicating whether or not this class represents a 
    /// multiple-apply API schema. Mutiple-apply API schemas can be applied 
    /// to the same prim multiple times with different instance names. 
    static const bool IsMultipleApply = true;

    /// Construct a UsdContrivedMultipleApplyAPI on UsdPrim \p prim .
    /// Equivalent to UsdContrivedMultipleApplyAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdContrivedMultipleApplyAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdContrivedMultipleApplyAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdContrivedMultipleApplyAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdContrivedMultipleApplyAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDCONTRIVED_API
    virtual ~UsdContrivedMultipleApplyAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDCONTRIVED_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdContrivedMultipleApplyAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdContrivedMultipleApplyAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDCONTRIVED_API
    static UsdContrivedMultipleApplyAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

private:

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "MultipleApplyAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'MultipleApplyAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdContrivedMultipleApplyAPI object is returned upon success. 
    /// An invalid (or empty) UsdContrivedMultipleApplyAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_MultipleApplyAPISchema() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    static UsdContrivedMultipleApplyAPI 
    _Apply(const UsdPrim &prim, const TfToken &name);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDCONTRIVED_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDCONTRIVED_API
    virtual const TfType &_GetTfType() const;

    // This override returns true since UsdContrivedMultipleApplyAPI is an 
    // applied API schema.
    USDCONTRIVED_API
    virtual bool _IsAppliedAPISchema() const override;

    // This override returns true since UsdContrivedMultipleApplyAPI is a multiple-
    // apply API schema.
    USDCONTRIVED_API
    virtual bool _IsMultipleApplyAPISchema() const override;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with }}}
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

}}}

#endif
