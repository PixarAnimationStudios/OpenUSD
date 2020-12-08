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
#ifndef USDUI_GENERATED_NODEGRAPHNODEAPI_H
#define USDUI_GENERATED_NODEGRAPHNODEAPI_H

/// \file usdUI/nodeGraphNodeAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUI/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// NODEGRAPHNODEAPI                                                           //
// -------------------------------------------------------------------------- //

/// \class UsdUINodeGraphNodeAPI
///
/// 
/// This api helps storing information about nodes in node graphs.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdUITokens.
/// So to set an attribute to the value "rightHanded", use UsdUITokens->rightHanded
/// as the value.
///
class UsdUINodeGraphNodeAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// \deprecated
    /// Same as schemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    static const UsdSchemaKind schemaType = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdUINodeGraphNodeAPI on UsdPrim \p prim .
    /// Equivalent to UsdUINodeGraphNodeAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdUINodeGraphNodeAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdUINodeGraphNodeAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdUINodeGraphNodeAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdUINodeGraphNodeAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDUI_API
    virtual ~UsdUINodeGraphNodeAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDUI_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdUINodeGraphNodeAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdUINodeGraphNodeAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDUI_API
    static UsdUINodeGraphNodeAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "NodeGraphNodeAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdUINodeGraphNodeAPI object is returned upon success. 
    /// An invalid (or empty) UsdUINodeGraphNodeAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDUI_API
    static UsdUINodeGraphNodeAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDUI_API
    UsdSchemaKind _GetSchemaKind() const override;

    /// \deprecated
    /// Same as _GetSchemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    USDUI_API
    UsdSchemaKind _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDUI_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDUI_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // POS 
    // --------------------------------------------------------------------- //
    /// 
    /// Declared relative position to the parent in a node graph.
    /// X is the horizontal position.
    /// Y is the vertical position. Higher numbers correspond to lower positions 
    /// (coordinates are Qt style, not cartesian).
    /// 
    /// These positions are not explicitly meant in pixel space, but rather
    /// assume that the size of a node is approximately 1.0x1.0. Where size-x is
    /// the node width and size-y height of the node. Depending on 
    /// graph UI implementation, the size of a node may vary in each direction.
    /// 
    /// Example: If a node's width is 300 and it is position is at 1000, we
    /// store for x-position: 1000 * (1.0/300)
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float2 ui:nodegraph:node:pos` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDUI_API
    UsdAttribute GetPosAttr() const;

    /// See GetPosAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreatePosAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STACKINGORDER 
    // --------------------------------------------------------------------- //
    /// 
    /// This optional value is a useful hint when an application cares about 
    /// the visibility of a node and whether each node overlaps another.
    /// 
    /// Nodes with lower stacking order values are meant to be drawn below 
    /// higher ones. Negative values are meant as background. Positive values
    /// are meant as foreground.
    /// Undefined values should be treated as 0. 
    /// 
    /// There are no set limits in these values.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform int ui:nodegraph:node:stackingOrder` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDUI_API
    UsdAttribute GetStackingOrderAttr() const;

    /// See GetStackingOrderAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreateStackingOrderAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DISPLAYCOLOR 
    // --------------------------------------------------------------------- //
    /// 
    /// This hint defines what tint the node should have in the node graph.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform color3f ui:nodegraph:node:displayColor` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Color3f |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDUI_API
    UsdAttribute GetDisplayColorAttr() const;

    /// See GetDisplayColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreateDisplayColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ICON 
    // --------------------------------------------------------------------- //
    /// 
    /// This points to an image that should be displayed on the node.  It is 
    /// intended to be useful for summary visual classification of nodes, rather
    /// than a thumbnail preview of the computed result of the node in some
    /// computational system.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform asset ui:nodegraph:node:icon` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDUI_API
    UsdAttribute GetIconAttr() const;

    /// See GetIconAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreateIconAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXPANSIONSTATE 
    // --------------------------------------------------------------------- //
    /// 
    /// The current expansionState of the node in the ui. 
    /// 'open' = fully expanded
    /// 'closed' = fully collapsed
    /// 'minimized' = should take the least space possible
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token ui:nodegraph:node:expansionState` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdUITokens "Allowed Values" | open, closed, minimized |
    USDUI_API
    UsdAttribute GetExpansionStateAttr() const;

    /// See GetExpansionStateAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreateExpansionStateAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SIZE 
    // --------------------------------------------------------------------- //
    /// 
    /// Optional size hint for a node in a node graph.
    /// X is the width.
    /// Y is the height.
    /// 
    /// This value is optional, because node size is often determined 
    /// based on the number of in- and outputs of a node.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float2 ui:nodegraph:node:size` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDUI_API
    UsdAttribute GetSizeAttr() const;

    /// See GetSizeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreateSizeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
