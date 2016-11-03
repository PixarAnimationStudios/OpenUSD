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
#ifndef USDSHADE_GENERATED_SUBGRAPH_H
#define USDSHADE_GENERATED_SUBGRAPH_H

/// \file usdShade/subgraph.h

#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <utility>
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usdShade/interfaceAttribute.h"
#include "pxr/usd/usdShade/parameter.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// SUBGRAPH                                                                   //
// -------------------------------------------------------------------------- //

/// \class UsdShadeSubgraph
///
/// A subgraph is a container for shading nodes, as well as other 
/// subgraphs. It has a public input interface and provides a list of public 
/// outputs, called terminals.
/// 
/// <b>Subgraph Interfaces</b>
/// 
/// One of the most important functions of a Subgraph is to host the "interface"
/// with which clients of already-built shading networks will interact.  Please
/// see \ref UsdShadeSubgraph_Interfaces "Interface Attributes" for a detailed
/// explanation of what the interface provides, and how to construct and
/// use it to effectively share/instance shader networks.
/// 
/// <b>Terminals</b>
/// 
/// Analogous to the public interface, these are relationships that each point 
/// to a single internal shader output.
/// 
///
class UsdShadeSubgraph : public UsdTyped
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Construct a UsdShadeSubgraph on UsdPrim \p prim .
    /// Equivalent to UsdShadeSubgraph::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeSubgraph(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdShadeSubgraph on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadeSubgraph(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadeSubgraph(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadeSubgraph();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdShadeSubgraph holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdShadeSubgraph(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadeSubgraph
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDSHADE_API
    static UsdShadeSubgraph
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSHADE_API
    virtual const TfType &_GetTfType() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class delcaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// \anchor UsdShadeSubgraph_Interfaces
    /// \name Interface Attributes
    ///
    /// In addition to serving as the "head" for all of the shading networks
    /// that describe each render target's particular Subgraph, the Subgraph
    /// prim provides a unified "interface" that allows Subgraphs to share 
    /// shading networks while retaining the ability for each to specify its own
    /// set of unique values for the parameters that users may need to modify.
    ///
    /// A "Subgraph Interface" is a combination of:
    /// \li a flat collection of attributes, of arbitrary names
    /// \li for each such attribute, a list of UsdShaderParameter targets
    /// whose attributes on Shader prims should be driven by the interface
    /// attribute
    ///
    /// A single interface attribute can drive multiple shader parameters - 
    /// within the same or multiple render targets.  Connections to the driven
    /// shader parameters are encoded in such a way that is easy to filter
    /// the Subgraph interface by render target; \em however, the set of 
    /// interface attributes itself is intentionally flat, to encourage
    /// sharing of interface between render targets.  Clients are always free
    /// to create interface attributes with namespacing to segregate "private"
    /// attributes exclusive to the render target, but we hope this will be
    /// an exception.
    ///
    /// To facilitate connecting, qualifying, and interrogating interface
    /// attributes, we provide an attribute schema UsdShadeInterfaceAttribute
    /// that performs services similar to UsdShadeParameter .
    ///
    /// <b>Scoped Interfaces</b>
    ///
    /// \todo describe scoped interfaces and fix bug/108940 to account for them
    /// @{

    /// Create an interface attribute.
    ///
    /// \p interfaceAttrName may be any legal property name, including
    /// arbitrary namespaces
    ///
    /// \todo clarify error behavior if typeName does not match existing,
    /// defined attribute - should match UsdPrim::CreateAttribute - bug/108970
    USDSHADE_API
    UsdShadeInterfaceAttribute CreateInterfaceAttribute(
            const TfToken& interfaceAttrName,
            const SdfValueTypeName& typeName);

    /// Return the Interface attribute named by \p name, which will
    /// be valid if an Interface attribute definition already exists.
    ///
    /// Name lookup will account for Interface namespacing, which means
    /// that this method will succeed in some cases where
    /// \code
    /// UsdShadeInterfaceAtribute(prim->GetAttribute(interfaceAttrName))
    /// \endcode
    /// will not, unless \p interfaceAttrName is properly namespace prefixed.
    USDSHADE_API
    UsdShadeInterfaceAttribute GetInterfaceAttribute(
            const TfToken& interfaceAttrName) const;

    /// Returns all interface attributes that drive parameters of a
    /// \p renderTarget shading network.
    USDSHADE_API
    std::vector<UsdShadeInterfaceAttribute> GetInterfaceAttributes(
            const TfToken& renderTarget) const;

    /// \todo GetInterfaceValueMap()

    /// @}

    /// Create and set a custom terminal of a subgraph
    /// 
    USDSHADE_API
    UsdRelationship CreateTerminal(
        const TfToken& terminalName,
        const SdfPath& targetPath) const;

    /// Get a terminal of a subgraph
    /// 
    USDSHADE_API
    UsdRelationship GetTerminal(
        const TfToken& terminalName) const;

    /// Get all terminals of a subgraph
    /// 
    USDSHADE_API
    UsdRelationshipVector GetTerminals() const;
};

#endif
