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
#ifndef USDRENDER_GENERATED_PASS_H
#define USDRENDER_GENERATED_PASS_H

/// \file usdRender/pass.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRender/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRender/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// RENDERPASS                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdRenderPass
///
/// A RenderPass prim encapsulates the necessary information
/// to generate multipass renders.  It houses properties for generating
/// dependencies and the necessary commands to run to generate renders, as 
/// well as visibility controls for the scene.  While RenderSettings
/// describes the information needed to generate images from a single 
/// invocation of a renderer, RenderPass describes the additional information
/// needed to generate a time varying set of images.
/// 
/// There are two consumers of RenderPass prims - a runtime executable that
/// generates images from usdRender prims, and pipeline specific code that
/// translates between usdRender prims and the pipeline's resource scheduling
/// software.  We'll refer to the latter as 'job submission code'.
/// 
/// The name of the prim is used as the pass's name.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdRenderTokens.
/// So to set an attribute to the value "rightHanded", use UsdRenderTokens->rightHanded
/// as the value.
///
class UsdRenderPass : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdRenderPass on UsdPrim \p prim .
    /// Equivalent to UsdRenderPass::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRenderPass(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdRenderPass on the prim held by \p schemaObj .
    /// Should be preferred over UsdRenderPass(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRenderPass(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDRENDER_API
    virtual ~UsdRenderPass();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRENDER_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRenderPass holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRenderPass(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRENDER_API
    static UsdRenderPass
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
    USDRENDER_API
    static UsdRenderPass
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDRENDER_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDRENDER_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDRENDER_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // PASSTYPE 
    // --------------------------------------------------------------------- //
    /// A string used to categorize differently structured 
    /// or executed types of passes within a customized pipeline.
    /// 
    /// For example, when multiple DCC's (e.g. Houdini, Katana, Nuke) 
    /// each compute and contribute different Products to a final result, 
    /// it may be clearest and most flexible to create a separate 
    /// RenderPass for each.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token passType` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetPassTypeAttr() const;

    /// See GetPassTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreatePassTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COMMAND 
    // --------------------------------------------------------------------- //
    /// The command to run in order to generate
    /// renders for this pass.  The job submission code can use
    /// this to properly send tasks to the job scheduling software
    /// that will generate products.
    /// 
    /// The command can contain variables that will be substituted
    /// appropriately during submission, as seen in the example below 
    /// with {fileName}.
    /// 
    /// For example:
    /// command[0] = "prman"
    /// command[1] = "-progress"
    /// command[2] = "-pixelvariance"
    /// command[3] = "-0.15"
    /// command[4] = "{fileName}" # the fileName property will be substituted
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string[] command` |
    /// | C++ Type | VtArray<std::string> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->StringArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetCommandAttr() const;

    /// See GetCommandAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateCommandAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FILENAME 
    // --------------------------------------------------------------------- //
    /// The asset that contains the rendering prims or other 
    /// information needed to render this pass.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform asset fileName` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetFileNameAttr() const;

    /// See GetFileNameAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateFileNameAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DENOISEENABLE 
    // --------------------------------------------------------------------- //
    /// When True, this Pass pass should be denoised.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool denoise:enable = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetDenoiseEnableAttr() const;

    /// See GetDenoiseEnableAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateDenoiseEnableAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // RENDERSOURCE 
    // --------------------------------------------------------------------- //
    /// The source prim to render from.  If _fileName_ is not present,
    /// the source is assumed to be a RenderSettings prim present in the current 
    /// Usd stage. If fileName is present, the source should be found in the
    /// file there. This relationship might target a string attribute on this 
    /// or another prim that identifies the appropriate object in the external 
    /// container.
    /// 
    /// For example, for a Usd-backed pass, this would point to a RenderSettings
    /// prim.  Houdini passes would point to a Rop.  Nuke passes would point to 
    /// a write node.
    /// 
    ///
    USDRENDER_API
    UsdRelationship GetRenderSourceRel() const;

    /// See GetRenderSourceRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRENDER_API
    UsdRelationship CreateRenderSourceRel() const;

public:
    // --------------------------------------------------------------------- //
    // INPUTPASSES 
    // --------------------------------------------------------------------- //
    /// The set of other Passes that this Pass depends on
    /// in order to be constructed properly.  For example, a Pass A
    /// may generate a texture, which is then used as an input to
    /// Pass B.
    /// 
    /// By default, usdRender makes some assumptions about the
    /// relationship between this prim and the prims listed in inputPasses.
    /// Namely, when per-frame tasks are generated from these pass prims,
    /// usdRender will assume a one-to-one relationship between tasks
    /// that share their frame number.  Consider a pass named 'composite'
    /// whose _inputPasses_ targets a Pass prim named 'beauty`.  
    /// By default, each frame for 'composite' will depend on the 
    /// same frame from 'beauty':
    /// beauty.1 -> composite.1
    /// beauty.2 -> composite.2
    /// etc
    /// 
    /// The consumer of this RenderPass graph of inputs will need to resolve
    /// the transitive dependencies.
    /// 
    ///
    USDRENDER_API
    UsdRelationship GetInputPassesRel() const;

    /// See GetInputPassesRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRENDER_API
    UsdRelationship CreateInputPassesRel() const;

public:
    // --------------------------------------------------------------------- //
    // DENOISEPASS 
    // --------------------------------------------------------------------- //
    /// The The UsdRenderDenoisePass prim from which to 
    /// source denoise settings.
    /// 
    ///
    USDRENDER_API
    UsdRelationship GetDenoisePassRel() const;

    /// See GetDenoisePassRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRENDER_API
    UsdRelationship CreateDenoisePassRel() const;

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
