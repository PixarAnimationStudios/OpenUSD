//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDCONTRIVED_GENERATED_TESTREFLECTEDAPIBASE_H
#define USDCONTRIVED_GENERATED_TESTREFLECTEDAPIBASE_H

/// \file usdContrived/testReflectedAPIBase.h

#include "pxr/pxr.h"
#include "pxr/usd/usdContrived/api.h"
#include "pxr/usd/usd/typed.h"
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
// TESTREFLECTEDAPIBASE                                                       //
// -------------------------------------------------------------------------- //

/// \class UsdContrivedTestReflectedAPIBase
///
///
class UsdContrivedTestReflectedAPIBase : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::AbstractTyped;

    /// Construct a UsdContrivedTestReflectedAPIBase on UsdPrim \p prim .
    /// Equivalent to UsdContrivedTestReflectedAPIBase::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdContrivedTestReflectedAPIBase(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdContrivedTestReflectedAPIBase on the prim held by \p schemaObj .
    /// Should be preferred over UsdContrivedTestReflectedAPIBase(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdContrivedTestReflectedAPIBase(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDCONTRIVED_API
    virtual ~UsdContrivedTestReflectedAPIBase();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDCONTRIVED_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdContrivedTestReflectedAPIBase holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdContrivedTestReflectedAPIBase(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDCONTRIVED_API
    static UsdContrivedTestReflectedAPIBase
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDCONTRIVED_API
    UsdSchemaKind _GetSchemaKind() const override;

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
    /// \name TestReflectedInternalAPI
    /// 
    /// Convenience accessors for the built-in UsdContrivedTestReflectedInternalAPI
    /// 
    /// @{

    /// Constructs and returns a UsdContrivedTestReflectedInternalAPI object.
    /// Use this object to access UsdContrivedTestReflectedInternalAPI custom methods.
    USDCONTRIVED_API
    UsdContrivedTestReflectedInternalAPI TestReflectedInternalAPI() const;

    /// See UsdContrivedTestReflectedInternalAPI::GetTestAttrInternalAttr().
    USDCONTRIVED_API
    UsdAttribute GetTestAttrInternalAttr() const;

    /// See UsdContrivedTestReflectedInternalAPI::CreateTestAttrInternalAttr().
    USDCONTRIVED_API
    UsdAttribute CreateTestAttrInternalAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdContrivedTestReflectedInternalAPI::GetTestAttrDuplicateAttr().
    USDCONTRIVED_API
    UsdAttribute GetTestAttrDuplicateAttr() const;

    /// See UsdContrivedTestReflectedInternalAPI::CreateTestAttrDuplicateAttr().
    USDCONTRIVED_API
    UsdAttribute CreateTestAttrDuplicateAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdContrivedTestReflectedInternalAPI::GetTestRelInternalRel().
    USDCONTRIVED_API
    UsdRelationship GetTestRelInternalRel() const;

    /// See UsdContrivedTestReflectedInternalAPI::CreateTestRelInternalRel().
    USDCONTRIVED_API
    UsdRelationship CreateTestRelInternalRel() const;

    /// See UsdContrivedTestReflectedInternalAPI::GetTestRelDuplicateRel().
    USDCONTRIVED_API
    UsdRelationship GetTestRelDuplicateRel() const;

    /// See UsdContrivedTestReflectedInternalAPI::CreateTestRelDuplicateRel().
    USDCONTRIVED_API
    UsdRelationship CreateTestRelDuplicateRel() const;

    /// @}
public:
    /// \name TestReflectedExternalAPI
    /// 
    /// Convenience accessors for the built-in UsdTestReflectedExternalAPI
    /// 
    /// @{

    /// Constructs and returns a UsdTestReflectedExternalAPI object.
    /// Use this object to access UsdTestReflectedExternalAPI custom methods.
    USDCONTRIVED_API
    UsdTestReflectedExternalAPI TestReflectedExternalAPI() const;

    /// See UsdTestReflectedExternalAPI::GetTestAttrExternalAttr().
    USDCONTRIVED_API
    UsdAttribute GetTestAttrExternalAttr() const;

    /// See UsdTestReflectedExternalAPI::CreateTestAttrExternalAttr().
    USDCONTRIVED_API
    UsdAttribute CreateTestAttrExternalAttr(
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// See UsdTestReflectedExternalAPI::GetTestRelExternalRel().
    USDCONTRIVED_API
    UsdRelationship GetTestRelExternalRel() const;

    /// See UsdTestReflectedExternalAPI::CreateTestRelExternalRel().
    USDCONTRIVED_API
    UsdRelationship CreateTestRelExternalRel() const;

    /// @}
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
