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
#ifndef USDSCHEMAEXAMPLES_GENERATED_SIMPLE_H
#define USDSCHEMAEXAMPLES_GENERATED_SIMPLE_H

/// \file usdSchemaExamples/simple.h

#include "pxr/pxr.h"
#include "./api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "./tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// SIMPLEPRIM                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdSchemaExamplesSimple
///
/// An example of an untyped schema prim. Note that it does not 
/// specify a typeName
///
class UsdSchemaExamplesSimple : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::AbstractTyped;

    /// Construct a UsdSchemaExamplesSimple on UsdPrim \p prim .
    /// Equivalent to UsdSchemaExamplesSimple::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdSchemaExamplesSimple(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdSchemaExamplesSimple on the prim held by \p schemaObj .
    /// Should be preferred over UsdSchemaExamplesSimple(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdSchemaExamplesSimple(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDSCHEMAEXAMPLES_API
    virtual ~UsdSchemaExamplesSimple();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSCHEMAEXAMPLES_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdSchemaExamplesSimple holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdSchemaExamplesSimple(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSCHEMAEXAMPLES_API
    static UsdSchemaExamplesSimple
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDSCHEMAEXAMPLES_API
    UsdSchemaType _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSCHEMAEXAMPLES_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSCHEMAEXAMPLES_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // INTATTR 
    // --------------------------------------------------------------------- //
    /// An integer attribute with fallback value of 0.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int intAttr = 0` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDSCHEMAEXAMPLES_API
    UsdAttribute GetIntAttrAttr() const;

    /// See GetIntAttrAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSCHEMAEXAMPLES_API
    UsdAttribute CreateIntAttrAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TARGET 
    // --------------------------------------------------------------------- //
    /// A relationship called target that could point to another prim
    /// or a property
    ///
    USDSCHEMAEXAMPLES_API
    UsdRelationship GetTargetRel() const;

    /// See GetTargetRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDSCHEMAEXAMPLES_API
    UsdRelationship CreateTargetRel() const;

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
