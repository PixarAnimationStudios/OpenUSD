//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLIGHTFIELD_GENERATED_KERNELCONSTANTTRIANGLEAPI_H
#define USDLIGHTFIELD_GENERATED_KERNELCONSTANTTRIANGLEAPI_H

/// \file usdLightField/kernelConstantTriangleAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLightField/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

            #include "pxr/usd/usdLightField/kernelBaseAPI.h"
        

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LIGHTFIELDKERNELCONSTANTTRIANGLEAPI                                        //
// -------------------------------------------------------------------------- //

/// \class UsdLightFieldKernelConstantTriangleAPI
///
/// Defines the gaussian triangle kernel for a given ParticleField.
/// 
/// The kernal shape is an equilateral triangle centered at the origin, with
/// the base of the triangle, parallel to the x-axis, and the apex of the
/// triangle on the y-axis.
/// 
/// The falloff function for this kernel is constant and the value is directly
/// defined by the opacity data source.
///
class UsdLightFieldKernelConstantTriangleAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLightFieldKernelConstantTriangleAPI on UsdPrim \p prim .
    /// Equivalent to UsdLightFieldKernelConstantTriangleAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLightFieldKernelConstantTriangleAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLightFieldKernelConstantTriangleAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLightFieldKernelConstantTriangleAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLightFieldKernelConstantTriangleAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLIGHTFIELD_API
    virtual ~UsdLightFieldKernelConstantTriangleAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLIGHTFIELD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLightFieldKernelConstantTriangleAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLightFieldKernelConstantTriangleAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLIGHTFIELD_API
    static UsdLightFieldKernelConstantTriangleAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Returns true if this <b>single-apply</b> API schema can be applied to 
    /// the given \p prim. If this schema can not be a applied to the prim, 
    /// this returns false and, if provided, populates \p whyNot with the 
    /// reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLIGHTFIELD_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "LightFieldKernelConstantTriangleAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLightFieldKernelConstantTriangleAPI object is returned upon success. 
    /// An invalid (or empty) UsdLightFieldKernelConstantTriangleAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLIGHTFIELD_API
    static UsdLightFieldKernelConstantTriangleAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLIGHTFIELD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLIGHTFIELD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLIGHTFIELD_API
    const TfType &_GetTfType() const override;

public:
    /// \name LightFieldKernelBaseAPI
    /// 
    /// Convenience accessors for the built-in UsdLightFieldKernelBaseAPI
    /// 
    /// @{

    /// Constructs and returns a UsdLightFieldKernelBaseAPI object.
    /// Use this object to access UsdLightFieldKernelBaseAPI custom methods.
    USDLIGHTFIELD_API
    UsdLightFieldKernelBaseAPI LightFieldKernelBaseAPI() const;

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
