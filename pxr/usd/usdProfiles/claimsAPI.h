//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPROFILES_GENERATED_CLAIMSAPI_H
#define USDPROFILES_GENERATED_CLAIMSAPI_H

/// \file UsdProfiles/claimsAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdProfiles/profileRegistry.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
    

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// CLAIMSAPI                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdProfilesClaimsAPI
///
/// UsdClaimsAPI records a prim's profile compatibility claims 
/// and per-capability usage declarations.
/// 
/// It provides two tiers of data, stored as sub-dictionaries within the
/// prim's \c profilesInfo metadata dictionary (analogous to \c assetInfo):
/// 
/// \section profiles_capability_usage Capability Usage (DCC-authored)
/// 
/// The \em capability usage tier records which USD capabilities this prim
/// actually uses, together with a degradation class for each:
/// 
/// - \b hard  — the capability is load-bearing; a consumer that does not
/// support it will produce incorrect results.
/// - \b soft  — it is acceptable that the prim degrades gracefully if the
/// capability is absent (e.g. although the capability may be absent, an
/// application has a fallback representation).
/// - \b enhancement — the capability improves quality but its absence is
/// allowable.
/// 
/// This tier is intended to be written automatically by the DCC tool that
/// saved the prim, or during a tool-based post-process that traverses a stage
/// aggregating capabilities.
/// 
/// \section profiles_compatibility Profile Compatibility (pipeline-authored)
/// 
/// The \em profile compatibility tier records which named profiles (nodes in
/// the UsdProfileRegistry DAG) this prim is compatible with, together with an
/// optional per-profile exception set listing capabilities that are
/// present but known to be out of compliance. As an example, a low-end mobile
/// profile may be able to display a particular material, but the performance
/// is known to be poor. So, while the prim has a `hard` capability declared,
/// the specific embodiment of that prim fails the profile's constraints.
/// 
/// A compatibility claim without exceptions is an unqualified assertion:
/// "this prim satisfies every capability in the profile's transitive closure."
/// A claim with exceptions is an audited, signed degradation declaration:
/// "this prim satisfies the profile except for the listed capabilities, which
/// are present but failing audit."
/// 
/// This tier is appropriate for publishable deliverables.  It is authored by
/// a pipeline conformance step or when performing a platform-compatibility
/// audit.
/// 
/// \section profiles_query Querying Compatibility
/// 
/// Use \c IsCompatibleWith() for the common single-profile check.  It calls
/// \c UsdProfileRegistry::CoversCapabilities() under the hood, passing the
/// stored exception set, and returns the aggregate \c QueryStatus.
/// 
/// For bulk access, \c GetProfilesInfo() / \c SetProfilesInfo() expose the
/// raw \c VtDictionary so callers can read or write the full record at once.
/// 
/// \section profiles_data_layout Data Layout
/// 
/// All data lives in the prim-level \c profilesInfo metadata dictionary:
/// 
/// \code
/// profilesInfo = {
/// dictionary capabilityUsages = {
/// string "usd.geom.mesh"    = "hard"
/// string "usd.geom.hairAndFur" = "soft"
/// string "usd.shading.mtlx" = "hard"
/// }
/// dictionary profileCompatibility = {
/// # profile token and list of present capabilities that 
/// # don't pass audit. (may be empty)
/// string[] "profile.studio.vfx.25.08" = []
/// string[] "vnd.apple.visionos_v1"    = ["usd.geom.hairAndFur"]
/// }
/// }
/// \endcode
/// 
///
class UsdProfilesClaimsAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdProfilesClaimsAPI on UsdPrim \p prim .
    /// Equivalent to UsdProfilesClaimsAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdProfilesClaimsAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdProfilesClaimsAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdProfilesClaimsAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdProfilesClaimsAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDPROFILES_API
    virtual ~UsdProfilesClaimsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDPROFILES_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdProfilesClaimsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdProfilesClaimsAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDPROFILES_API
    static UsdProfilesClaimsAPI
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
    USDPROFILES_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ClaimsAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdProfilesClaimsAPI object is returned upon success. 
    /// An invalid (or empty) UsdProfilesClaimsAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDPROFILES_API
    static UsdProfilesClaimsAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDPROFILES_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDPROFILES_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDPROFILES_API
    const TfType &_GetTfType() const override;

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

    /// \anchor UsdProfilesClaimsAPI
    /// \name Claims API
    ///
    /// All profile data is stored in the prim-level \c customData
    /// dictionary under the key \c profilesInfo, using two sub-dictionaries:
    ///
    /// - \c capabilityUsages — maps capability token → degradation class string
    ///   (\c "hard", \c "soft", or \c "enhancement"). Written by the DCC.
    /// - \c profileCompatibility — maps profile token → array of excepted
    ///   capability tokens (empty array = fully compatible). Written by the
    ///   pipeline audit step.
    ///
    /// @{

    // ---------------------------------------------------------------------- //
    // Raw dictionary access
    // ---------------------------------------------------------------------- //

    /// Returns the composed \c profilesInfo dictionary, or an empty
    /// dictionary if none has been authored.
    USDPROFILES_API
    VtDictionary GetProfilesInfo() const;

    /// Replaces the entire \c profilesInfo dictionary on the current edit
    /// target.
    USDPROFILES_API
    void SetProfilesInfo(const VtDictionary &info) const;

    // ---------------------------------------------------------------------- //
    // Capability Usage (DCC-authored tier)
    // ---------------------------------------------------------------------- //

    /// Returns the capability usage map as a \c VtDictionary whose keys are
    /// capability token strings and whose values are degradation class strings
    /// (\c "hard", \c "soft", or \c "enhancement").
    ///
    /// Returns an empty dictionary if no capability usages have been authored.
    USDPROFILES_API
    VtDictionary GetCapabilityUsages() const;

    /// Replaces the entire capability usage map.
    ///
    /// \p usages must be a dictionary mapping capability token strings to one
    /// of \c "hard", \c "soft", or \c "enhancement".
    USDPROFILES_API
    void SetCapabilityUsages(const VtDictionary &usages) const;

    /// Sets the degradation class for a single capability.
    ///
    /// \p degradationClass must be \c "hard", \c "soft", or \c "enhancement".
    /// Replaces any previously authored value for \p capability.
    USDPROFILES_API
    void SetCapabilityUsage(const TfToken &capability,
                            const TfToken &degradationClass) const;

    /// Returns the degradation class string for \p capability, or an empty
    /// \c TfToken if \p capability has not been recorded.
    USDPROFILES_API
    TfToken GetCapabilityUsage(const TfToken &capability) const;

    // ---------------------------------------------------------------------- //
    // Profile Compatibility (pipeline-authored tier)
    // ---------------------------------------------------------------------- //

    /// Returns all profiles that this prim has been declared compatible with,
    /// as an unordered array of profile tokens.
    ///
    /// This includes both fully-compatible profiles (no exceptions) and those
    /// with a non-empty exception set.
    USDPROFILES_API
    VtArray<TfToken> GetCompatibleProfiles() const;

    /// Declares this prim fully compatible with \p profile (no exceptions).
    ///
    /// Replaces any previously authored compatibility entry for \p profile.
    USDPROFILES_API
    void SetProfileCompatible(const TfToken &profile) const;

    /// Declares this prim compatible with \p profile subject to the given
    /// \p exceptions — capabilities that are intentionally unsupported.
    ///
    /// Replaces any previously authored compatibility entry for \p profile.
    USDPROFILES_API
    void SetProfileCompatibleWithExceptions(
        const TfToken &profile,
        const VtArray<TfToken> &exceptions) const;

    /// Returns the exception set for \p profile — the capabilities that are
    /// intentionally unsupported in the context of that profile.
    ///
    /// Returns an empty array if \p profile is not a declared compatible
    /// profile, or if it is compatible with no exceptions.
    USDPROFILES_API
    VtArray<TfToken> GetProfileExceptions(const TfToken &profile) const;

    /// Removes the compatibility entry for \p profile entirely.
    ///
    /// After this call, \p profile will not appear in \c GetCompatibleProfiles.
    USDPROFILES_API
    void ClearProfileCompatibility(const TfToken &profile) const;

    // ---------------------------------------------------------------------- //
    // Compatibility query
    // ---------------------------------------------------------------------- //

    /// Returns the \c QueryStatus representing how well this prim satisfies
    /// \p profile, taking into account any exception set stored for that
    /// profile via \c SetProfileCompatibleWithExceptions.
    ///
    /// This is a convenience that calls
    /// \c UsdProfileRegistry::CoversCapabilities() with:
    /// - \p profile as the perspective
    /// - the capability tokens from \c GetCapabilityUsages() as \p required
    /// - the stored exception set (from \c GetProfileExceptions()) as
    ///   \p excepted
    ///
    /// If \p results is non-null it is populated with per-capability detail,
    /// following the same contract as \c CoversCapabilities.
    ///
    /// Returns \c QueryStatus::NoPath if \p profile is not a declared
    /// compatible profile on this prim, or if no capability usages have been
    /// recorded.
    USDPROFILES_API
    UsdProfileRegistry::QueryStatus IsCompatibleWith(
        const TfToken &profile,
        std::vector<UsdProfileRegistry::CapabilityResult> *results = nullptr)
        const;

    /// @}

private:
    template <typename T>
    bool _GetProfilesInfoByKey(const TfToken &keyPath, T *val) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
