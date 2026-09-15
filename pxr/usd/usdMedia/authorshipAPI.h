//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDMEDIA_GENERATED_AUTHORSHIPAPI_H
#define USDMEDIA_GENERATED_AUTHORSHIPAPI_H

/// \file usdMedia/authorshipAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdMedia/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdMedia/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// AUTHORSHIPAPI                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdMediaAuthorshipAPI
///
/// Records who or what authored a prim.
/// 
/// Each record describes one authoring step, such as a generative AI run,
/// an export from a DCC, or a manual cleanup session. A prim can carry
/// its full history of contributors, provided that each step is recorded
/// with a unique-to-the-prim schema instance name.
/// 
/// The schema may also be applied to any of a prim's ancestors.
/// Authorship aggregates down the namespace hierarchy. A prim's own
/// records add to, rather than override, its ancestors' records.
/// 
/// Provides a standardized, unverified record of authorship. This is not
/// a cryptographic signature, and `copyrightOwner` represents a claim
/// rather than a legal guarantee.
/// 
/// \sa UsdMediaAuthorshipAPI::GetAllOnStage()
/// \sa UsdMediaAuthorshipAPI::ComputeAccumulatedRecords()
/// \sa UsdMediaAuthorshipAPI::GetAllInPrimStacks()
/// 
///
class UsdMediaAuthorshipAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::MultipleApplyAPI;

    /// Construct a UsdMediaAuthorshipAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdMediaAuthorshipAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "authorship:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdMediaAuthorshipAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdMediaAuthorshipAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdMediaAuthorshipAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdMediaAuthorshipAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDMEDIA_API
    virtual ~UsdMediaAuthorshipAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDMEDIA_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USDMEDIA_API
    static TfTokenVector
    GetSchemaAttributeNames(bool includeInherited, const TfToken &instanceName);

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdMediaAuthorshipAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.authorship:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdMediaAuthorshipAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDMEDIA_API
    static UsdMediaAuthorshipAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdMediaAuthorshipAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdMediaAuthorshipAPI(prim, name);
    USDMEDIA_API
    static UsdMediaAuthorshipAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Return a vector of all named instances of UsdMediaAuthorshipAPI on the 
    /// given \p prim.
    USDMEDIA_API
    static std::vector<UsdMediaAuthorshipAPI>
    GetAll(const UsdPrim &prim);

    /// Checks if the given name \p baseName is the base name of a property
    /// of AuthorshipAPI.
    USDMEDIA_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// AuthorshipAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDMEDIA_API
    static bool
    IsAuthorshipAPIPath(const SdfPath &path, TfToken *name);

    /// Returns true if this <b>multiple-apply</b> API schema can be applied,
    /// with the given instance name, \p name, to the given \p prim. If this 
    /// schema can not be a applied the prim, this returns false and, if 
    /// provided, populates \p whyNot with the reason it can not be applied.
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
    USDMEDIA_API
    static bool 
    CanApply(const UsdPrim &prim, const TfToken &name, 
             std::string *whyNot=nullptr);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "AuthorshipAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'AuthorshipAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdMediaAuthorshipAPI object is returned upon success. 
    /// An invalid (or empty) UsdMediaAuthorshipAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDMEDIA_API
    static UsdMediaAuthorshipAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDMEDIA_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDMEDIA_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDMEDIA_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // SOFTWAREPACKAGE 
    // --------------------------------------------------------------------- //
    /// The tool or system that most directly wrote the USD data
    /// for this prim. Must be a tool (e.g., `org.blender`), not a
    /// person; human contributors belong in `creator`. If an AI model
    /// runs inside a host application, name the model itself. If someone
    /// authored the data directly with the OpenUSD API in an unnamed
    /// script, the softwarePackage would be `OpenUSD`.
    /// 
    /// Reverse domain notation is recommended (e.g.,
    /// `net.trellis3d.hunyuan3d`) to minimize name conflicts. Values are
    /// unvalidated and compared case-insensitively.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string softwarePackage` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetSoftwarePackageAttr() const;

    /// See GetSoftwarePackageAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateSoftwarePackageAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SOFTWAREVERSION 
    // --------------------------------------------------------------------- //
    /// The version of the softwarePackage that created this
    /// record, not of the asset. If a hosted service provides no stable
    /// version, use the provided build label or hash, and rely on
    /// `created` to pin down the iteration's ordering.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string softwareVersion` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetSoftwareVersionAttr() const;

    /// See GetSoftwareVersionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateSoftwareVersionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DIGITALSOURCETYPE 
    // --------------------------------------------------------------------- //
    /// A URI describing the nature of the creative process, such
    /// as whether the content is human-made or AI-generated.
    /// 
    /// The IPTC Digital Source Type vocabulary is recommended (e.g.,
    /// `trainedAlgorithmicMedia`). Tools evaluating AI content should
    /// check this field across the prim's accumulated ancestry and default
    /// to assuming non-AI generation if an AI source type is absent.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string digitalSourceType` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetDigitalSourceTypeAttr() const;

    /// See GetDigitalSourceTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateDigitalSourceTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREATOR 
    // --------------------------------------------------------------------- //
    /// Human-readable names of the artists, studios, or
    /// services that contributed to this authoring step. Intended for
    /// display. Corresponds to `dc:creator`.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string[] creator` |
    /// | C++ Type | VtArray<std::string> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->StringArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetCreatorAttr() const;

    /// See GetCreatorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateCreatorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DESCRIPTION 
    // --------------------------------------------------------------------- //
    /// Free-form description of how the prim was created. Better
    /// suited to technique notes, reference material, or history. For
    /// specific, itemized inputs, prefer `inputNames` and `inputValues`
    /// instead.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string description` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetDescriptionAttr() const;

    /// See GetDescriptionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateDescriptionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INPUTNAMES 
    // --------------------------------------------------------------------- //
    /// Names of the inputs that shaped this authoring step, e.g.,
    /// `prompt`, `seed`, `guidance`, or `referenceImage`. Not specific to
    /// generative AI; a human authoring step can record its reference
    /// art, schematics, or design specs the same way.
    /// 
    /// Must be index-matched with `inputValues` and authored in the same
    /// layer to ensure composition aligns them correctly. Length
    /// mismatches invalidate the record; readers should not attempt to
    /// guess the pairing.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string[] inputNames` |
    /// | C++ Type | VtArray<std::string> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->StringArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetInputNamesAttr() const;

    /// See GetInputNamesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateInputNamesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INPUTVALUES 
    // --------------------------------------------------------------------- //
    /// Values for the inputs named in `inputNames`,
    /// matched by index. Authored as plain strings rather than `asset`
    /// paths to prevent reference images or local files from being
    /// resolved or packaged with the asset.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string[] inputValues` |
    /// | C++ Type | VtArray<std::string> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->StringArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetInputValuesAttr() const;

    /// See GetInputValuesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateInputValuesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREATED 
    // --------------------------------------------------------------------- //
    /// An ISO 8601 timestamp indicating when this authoring step
    /// occurred (e.g., `2025-02-16T12:03:17+01:00`), which may differ from
    /// the original creation time of the asset.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string created` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetCreatedAttr() const;

    /// See GetCreatedAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateCreatedAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INSTANCEID 
    // --------------------------------------------------------------------- //
    /// A unique identifier for this run's output, such as a
    /// single generative AI result. A UUID4 is recommended. Do not encode
    /// personal author information.
    /// 
    /// Unrelated to USD native instancing and unrelated to this schema's
    /// applied instance name.
    /// 
    /// Identifies an output, not a reproducible recipe; identical inputs
    /// do not guarantee identical results. Uniqueness is not guaranteed
    /// across external pipelines. Named after `xmpMM:InstanceID` but
    /// identifies an authoring step's output (which may span several
    /// prims) rather than a single resource rendition. Not directly
    /// equivalent when round-tripping.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string instanceID` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetInstanceIDAttr() const;

    /// See GetInstanceIDAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateInstanceIDAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // USAGETERMS 
    // --------------------------------------------------------------------- //
    /// The license or usage terms for this asset.
    /// 
    /// SPDX identifiers are preferred (e.g., `CC-BY-4.0` or `MIT`) to
    /// allow programmatic evaluation. Otherwise, provide a URL to the
    /// license text, or the full text as a fallback. If unauthored, no
    /// specific license or rights should be assumed. Corresponds to
    /// `xmpRights:UsageTerms`.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string usageTerms` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetUsageTermsAttr() const;

    /// See GetUsageTermsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateUsageTermsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COPYRIGHTOWNER 
    // --------------------------------------------------------------------- //
    /// Entities holding copyright in this content, which may
    /// differ from `creator` (e.g., a studio owning an artist's work).
    /// Corresponds to `xmpRights:Owner`.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string[] copyrightOwner` |
    /// | C++ Type | VtArray<std::string> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->StringArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetCopyrightOwnerAttr() const;

    /// See GetCopyrightOwnerAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateCopyrightOwnerAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CONTACT 
    // --------------------------------------------------------------------- //
    /// Contact information for inquiries about this asset, such
    /// as an email address, support URL, or licensing page.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string[] contact` |
    /// | C++ Type | VtArray<std::string> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->StringArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetContactAttr() const;

    /// See GetContactAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateContactAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Returns every authorship record on \p stage, sorted by prim path then
    /// instance name. Records inside a native instance's prototype are
    /// reported once, against the prototype, not once per instance.
    /// Inactive and abstract prims are included, but an inactive prim's
    /// descendants are not (they aren't composed at all). Reflects the
    /// stage's current load state and variant selections.
    ///
    /// Reports applied schemas as seen by UsdPrim::GetAppliedSchemas() on
    /// composed prims. \sa GetAllInPrimStacks() for a diagnostic search
    /// that also finds shadowed or clobbered records.
    USDMEDIA_API
    static std::vector<UsdMediaAuthorshipAPI>
    GetAllOnStage(const UsdStagePtr &stage);

    /// Returns the records that apply to \p prim: its own plus those
    /// accumulated from its ancestors, sorted so ancestors come first.
    /// Compare a record's prim against \p prim to distinguish an inherited
    /// record from one the prim declared itself.
    ///
    /// A prim's children accumulate their ancestors' authorship without
    /// erasing it, so a hand-modelled prim under an AI-authored group
    /// carries both records. Records are never merged, even when they
    /// share an instance name across prims.
    ///
    /// Only consults the composed stage; does not accumulate records
    /// shadowed by explicit `apiSchemas` list-ops. \sa GetAllInPrimStacks().
    USDMEDIA_API
    static std::vector<UsdMediaAuthorshipAPI>
    ComputeAccumulatedRecords(const UsdPrim &prim);

    /// Returns the records on \p prim and everything beneath it, sorted by
    /// prim path then instance name. Includes \p prim itself. Prims
    /// beneath a native instance are skipped; their records live on the
    /// prototype (see GetAllOnStage()).
    ///
    /// Only consults the composed stage. \sa GetAllInPrimStacks().
    USDMEDIA_API
    static std::vector<UsdMediaAuthorshipAPI>
    GetAllUnder(const UsdPrim &prim);

    /// Performs a diagnostic search for records unreachable via composed
    /// views, such as an application shadowed by an explicit apiSchemas
    /// list in a stronger layer, or the same instance name authored in
    /// several layers (composition keeps only the strongest opinion).
    ///
    /// Scans every SdfPrimSpec on \p stage instead of the composed prim.
    /// Nothing is deduplicated: a record in three layers is returned
    /// three times, strongest-to-weakest, so callers can see what
    /// composition dropped.
    ///
    /// Only apiSchemas metadata is scanned, not authored properties, so a
    /// record whose schema was never applied anywhere is not found.
    /// Instance names and base names can both be namespaced, so a property
    /// name alone can't be split back into the two. Repeated entries read
    /// the same composed values; HasAPI() may report false for them.
    /// Unselected variants aren't in a prim's prim stack
    /// (UsdPrim::GetPrimStack()) and aren't found either; use
    /// GetAllInLayer() for those.
    ///
    /// Considerably slower than GetAllOnStage(), ComputeAccumulatedRecords(),
    /// and GetAllUnder(). Intended for debugging and validation, not
    /// general use.
    USDMEDIA_API
    static std::vector<UsdMediaAuthorshipAPI>
    GetAllInPrimStacks(const UsdStagePtr &stage);

    /// Returns the path of every authorship record authored in \p layer, sorted,
    /// in the <tt><primPath>.authorship:instanceName</tt> form that
    /// IsAuthorshipAPIPath() parses.
    ///
    /// Inspects a single layer's scene description without composition.
    /// Records in sublayers and reference targets are not included. Use
    /// this to identify what a specific layer contributes on its own.
    /// Reaches specs unreachable via composed stages, such as those inside
    /// unselected variants.
    ///
    /// Paths are returned rather than schema objects because a record found
    /// this way may have no corresponding prim on any stage: it may sit
    /// beneath a deactivated prim, behind a variant selection nothing
    /// resolves to, or the layer itself may only be a fragment of a larger
    /// scene assembled through references or sublayers that carry records
    /// of their own. There is no guarantee of correspondence between what
    /// this method reports for a layer and what GetAllOnStage() finds on a
    /// stage built from it.
    USDMEDIA_API
    static SdfPathVector
    GetAllInLayer(const SdfLayerHandle &layer);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
