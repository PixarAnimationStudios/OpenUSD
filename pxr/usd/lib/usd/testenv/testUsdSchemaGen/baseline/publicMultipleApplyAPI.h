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
#ifndef USDCONTRIVED_GENERATED_PUBLICMULTIPLEAPPLYAPI_H
#define USDCONTRIVED_GENERATED_PUBLICMULTIPLEAPPLYAPI_H

/// \file usdContrived/publicMultipleApplyAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdContrived/api.h"
#include "pxr/usd/usd/aPISchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdContrived/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PUBLICMULTIPLEAPPLYAPI                                                     //
// -------------------------------------------------------------------------- //

/// \class UsdContrivedPublicMultipleApplyAPI
///
///
class UsdContrivedPublicMultipleApplyAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::MultipleApplyAPI;

    /// Construct a UsdContrivedPublicMultipleApplyAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdContrivedPublicMultipleApplyAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "testo:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdContrivedPublicMultipleApplyAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdContrivedPublicMultipleApplyAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdContrivedPublicMultipleApplyAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdContrivedPublicMultipleApplyAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDCONTRIVED_API
    virtual ~UsdContrivedPublicMultipleApplyAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USDCONTRIVED_API
    static const TfTokenVector &
    GetSchemaAttributeNames(
        bool includeInherited=true, const TfToken instanceName=TfToken());

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdContrivedPublicMultipleApplyAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.testo:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdContrivedPublicMultipleApplyAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDCONTRIVED_API
    static UsdContrivedPublicMultipleApplyAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdContrivedPublicMultipleApplyAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdContrivedPublicMultipleApplyAPI(prim, name);
    USDCONTRIVED_API
    static UsdContrivedPublicMultipleApplyAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Checks if the given name \p baseName is the base name of a property
    /// of PublicMultipleApplyAPI.
    USDCONTRIVED_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// PublicMultipleApplyAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDCONTRIVED_API
    static bool
    IsPublicMultipleApplyAPIPath(const SdfPath &path, TfToken *name);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "PublicMultipleApplyAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'PublicMultipleApplyAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdContrivedPublicMultipleApplyAPI object is returned upon success. 
    /// An invalid (or empty) UsdContrivedPublicMultipleApplyAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_MultipleApplyAPISchema() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    USDCONTRIVED_API
    static UsdContrivedPublicMultipleApplyAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDCONTRIVED_API
    UsdSchemaType _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDCONTRIVED_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDCONTRIVED_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // TESTATTRONE 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: int
    /// \n  Usd Type: SdfValueTypeNames->Int
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetTestAttrOneAttr() const;

    /// See GetTestAttrOneAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateTestAttrOneAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TESTATTRTWO 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetTestAttrTwoAttr() const;

    /// See GetTestAttrTwoAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateTestAttrTwoAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
