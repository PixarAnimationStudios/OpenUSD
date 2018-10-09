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
#ifndef USDCONTRIVED_GENERATED_DERIVEDMULTIPLEAPPLYAPI_H
#define USDCONTRIVED_GENERATED_DERIVEDMULTIPLEAPPLYAPI_H

/// \file usdContrived/derivedMultipleApplyAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdContrived/api.h"
#include "pxr/usd/usdContrived/multipleApplyAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdContrived/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

namespace foo { namespace bar { namespace baz {

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// DERIVEDMULTIPLEAPPLYAPI                                                    //
// -------------------------------------------------------------------------- //

/// \class UsdContrivedDerivedMultipleApplyAPI
///
///
class UsdContrivedDerivedMultipleApplyAPI : public UsdContrivedMultipleApplyAPI
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::MultipleApplyAPI;

    /// Construct a UsdContrivedDerivedMultipleApplyAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdContrivedDerivedMultipleApplyAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "derived:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdContrivedDerivedMultipleApplyAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdContrivedMultipleApplyAPI(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdContrivedDerivedMultipleApplyAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdContrivedDerivedMultipleApplyAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdContrivedDerivedMultipleApplyAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdContrivedMultipleApplyAPI(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDCONTRIVED_API
    virtual ~UsdContrivedDerivedMultipleApplyAPI();

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

    /// Return a UsdContrivedDerivedMultipleApplyAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.derived:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdContrivedDerivedMultipleApplyAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDCONTRIVED_API
    static UsdContrivedDerivedMultipleApplyAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdContrivedDerivedMultipleApplyAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdContrivedDerivedMultipleApplyAPI(prim, name);
    USDCONTRIVED_API
    static UsdContrivedDerivedMultipleApplyAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Checks if the given name \p baseName is the base name of a property
    /// of DerivedMultipleApplyAPI.
    USDCONTRIVED_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// DerivedMultipleApplyAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDCONTRIVED_API
    static bool
    IsDerivedMultipleApplyAPIPath(const SdfPath &path, TfToken *name);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "DerivedMultipleApplyAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'DerivedMultipleApplyAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdContrivedDerivedMultipleApplyAPI object is returned upon success. 
    /// An invalid (or empty) UsdContrivedDerivedMultipleApplyAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_MultipleApplyAPISchema() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    USDCONTRIVED_API
    static UsdContrivedDerivedMultipleApplyAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDCONTRIVED_API
    virtual UsdSchemaType _GetSchemaType() const;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDCONTRIVED_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDCONTRIVED_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // TESTATTRTHREE 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: int
    /// \n  Usd Type: SdfValueTypeNames->Int
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDCONTRIVED_API
    UsdAttribute GetTestAttrThreeAttr() const;

    /// See GetTestAttrThreeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCONTRIVED_API
    UsdAttribute CreateTestAttrThreeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
