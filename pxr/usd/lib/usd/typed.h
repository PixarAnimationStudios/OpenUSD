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
#ifndef USD_TYPED_H
#define USD_TYPED_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdTyped
///
/// The base class for all \em typed schemas (those that can impart a
/// typeName to a UsdPrim), and therefore the base class for all
/// instantiable and "IsA" schemas.
///    
/// UsdTyped implements a typeName-based query for its override of
/// UsdSchemaBase::_IsCompatible().  It provides no other behavior.
///
class UsdTyped : public UsdSchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType in usd/common.h
    static const UsdSchemaType schemaType = UsdSchemaType::AbstractBase;

    /// Construct a UsdTyped on UsdPrim \p prim .
    /// Equivalent to UsdTyped::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdTyped(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdTyped on the prim wrapped by \p schemaObj .
    /// Should be preferred over UsdTyped(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdTyped(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    USD_API
    virtual ~UsdTyped();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true) {
        /* This only exists for consistency */
        static TfTokenVector names;
        return names;
    }

    /// Return a UsdTyped holding the prim adhering to this schema at \p path
    /// on \p stage.  If no prim exists at \p path on \p stage, or if the prim
    /// at that path does not adhere to this schema, return an invalid schema
    /// object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdTyped(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USD_API
    static UsdTyped
    Get(const UsdStagePtr &stage, const SdfPath &path);

protected:
    USD_API
    virtual bool _IsCompatible() const override;

private:
    USD_API
    virtual const TfType &_GetTfType() const;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_TYPED_H
