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
/// Each record describes a single authoring step, like a generative AI run,
/// an export from a DCC, or a manual cleanup session. Because multiple
/// records can be applied, a prim can carry its full history of contributors
/// without them clobbering each other.
/// 
/// This schema provides a way to record authorship but does not verify or
/// sign it. It relies on the honor system to help with regulatory
/// compliance and artist credit. copyrightOwner records an ownership claim;
/// it does not establish one.
/// 
/// \sa UsdMediaAuthorshipAPI::GetAllOnStage()
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
    /// The tool or system that most directly wrote the USD data for
    /// this prim. This is always a tool, like 'org.blender', not a person.
    /// People go in 'creator'. If an AI model runs inside another tool,
    /// this should name the model.
    /// 
    /// We recommend using reverse domain notation (e.g., 'net.trellis3d.hunyuan3d')
    /// to avoid name conflicts. No network lookup or validation is performed;
    /// it is just a unique identifier. Compare values case-insensitively.
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
    /// The version of the softwarePackage that created this record.
    /// It versions the tool itself, not the asset. For hosted services
    /// without a stable version, use whatever label they provide, like a
    /// build hash. Rely on the 'created' timestamp to pin down which
    /// iteration was used.
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
    /// A URI describing the nature of the creative process, for
    /// example whether it was human-made or AI-generated.
    /// 
    /// We recommend using the IPTC Digital Source Type vocabulary (e.g., 
    /// 'trainedAlgorithmicMedia'). This is the main field tools should 
    /// check to identify AI content. Tools should 'fail safe': if no 
    /// record in the hierarchy specifies an AI source, the content should 
    /// not be assumed to be AI-generated.
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
    /// A list of human-friendly names for the artists, studios, or 
    /// services that contributed to this authoring step. This is intended 
    /// for display and corresponds to 'dc:creator'.
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
    /// A free-form description of how the prim was created. 
    /// For AI content, use 'prompt:inputNames' and 'prompt:inputValues' 
    /// for the specific generation inputs instead. This field is best for 
    /// technique notes, reference material, or historical context.
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
    // PROMPTINPUTNAMES 
    // --------------------------------------------------------------------- //
    /// Names of the specific inputs that shaped an AI or algorithmic 
    /// generation, like 'prompt', 'seed', or 'guidance'. 
    /// 
    /// These are index-matched with 'prompt:inputValues'. Both arrays
    /// must be the same length. Author them together in the same layer,
    /// since composition could otherwise pull them out of alignment.
    /// Treat a length mismatch as a malformed record.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string[] prompt:inputNames` |
    /// | C++ Type | VtArray<std::string> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->StringArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetPromptInputNamesAttr() const;

    /// See GetPromptInputNamesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreatePromptInputNamesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PROMPTINPUTVALUES 
    // --------------------------------------------------------------------- //
    /// Values for the inputs named in 'prompt:inputNames'. 
    /// These are simple strings and not 'asset' paths, so that reference 
    /// images or other inputs aren't accidentally resolved or packaged 
    /// along with the asset.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string[] prompt:inputValues` |
    /// | C++ Type | VtArray<std::string> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->StringArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetPromptInputValuesAttr() const;

    /// See GetPromptInputValuesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreatePromptInputValuesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREATED 
    // --------------------------------------------------------------------- //
    /// The date and time this specific authoring step occurred, 
    /// formatted as an ISO 8601 timestamp (e.g., '2025-02-16T12:03:17+01:00'). 
    /// It tracks this iteration, not necessarily when the asset was 
    /// first ever created.
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
    /// A unique ID for this specific run's output, like a single
    /// generative AI result. We recommend using a UUID4.
    /// 
    /// This is unrelated to USD native instancing. It is also unrelated to
    /// this schema's own applied-API instance name. It should not contain
    /// personal information about the author.
    /// 
    /// This identifies a run's output, not a reproducible recipe: the same
    /// inputs are not guaranteed to produce an identical result again. It is
    /// also not globally unique. No claim is made that it won't coincide
    /// with an asset identifier, a database key, an XMP DocumentID, or any
    /// other identifier in a pipeline.
    /// 
    /// Named after xmpMM:InstanceID, but broader in scope: XMP's identifies
    /// one rendition of one resource, while this one identifies an
    /// authoring step's output, which may span several prims. Don't treat
    /// the two as equivalent when round-tripping.
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
    /// We recommend using an SPDX identifier (e.g., 'CC-BY-4.0' or 'MIT')
    /// whenever possible. If the license isn't in the SPDX list, a URL to
    /// the license text is next best. Full text inline is the last resort.
    /// If no terms are provided, no specific license should be assumed.
    /// Corresponds to 'xmpRights:UsageTerms'.
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
    /// The entities that hold copyright for this content, which may 
    /// be different from the creators (e.g., a studio owning an artist's 
    /// work). Corresponds to 'xmpRights:Owner'.
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
    /// Contact information for inquiries about this asset, such as
    /// an email address, a support URL, or a licensing page.
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
    /// instance name. Prims beneath native instances are reached through the
    /// stage's prototypes, so a record inside a prototype is reported against
    /// the prototype rather than once per instance. Inactive and abstract prims
    /// are included. Note that descendants of an inactive prim are not
    /// composed onto the stage at all, so their records cannot be reported.
    ///
    /// If \p searchPrimStack is \c false (the default), records are those the
    /// composed prim reports through UsdPrim::GetAppliedSchemas().
    ///
    /// If \p searchPrimStack is \c true, records are gathered from every
    /// SdfPrimSpec contributing to each prim instead. Nothing is deduplicated.
    /// A record authored in three layers is returned three times, in
    /// strongest-to-weakest order, so that callers can see what composition
    /// collapsed. Resolving those opinions is composition's job, not this
    /// method's. This search covers cases the composed prim cannot report:
    ///
    /// - A stronger layer authoring an \em explicit \em apiSchemas list can
    ///   shadow an application coming from a weaker layer such as a reference.
    ///   The record's properties still compose onto the prim and remain
    ///   readable, but the schema no longer counts as applied.
    /// - Authorship properties authored without the schema ever having been
    ///   applied, which some tools emit.
    /// - The same instance name authored in several layers, where composition
    ///   keeps only the strongest opinion for each field.
    ///
    /// A returned schema object always reads composed property values, so
    /// repeated entries read alike. They record that several specs contribute,
    /// not what each one said. UsdPrim::HasAPI() may report \c false for them.
    /// Unselected variants are not part of a prim's spec stack, so records
    /// inside them are not found either way. This search visits every layer
    /// contributing to every prim and is considerably slower.
    USDMEDIA_API
    static std::vector<UsdMediaAuthorshipAPI>
    GetAllOnStage(const UsdStagePtr &stage, bool searchPrimStack = false);

    /// Returns the records that apply to \p prim, meaning its own plus those it
    /// accumulates from its ancestors, sorted by prim path so ancestors come
    /// first. Compare a record's prim against \p prim to tell an inherited
    /// record from one the prim declared itself.
    ///
    /// This proposal's convention is that a prim's children accumulate the
    /// authorship of their ancestors: a child may add its own records but does
    /// not erase the ones above it. So a hand-modelled prim under an
    /// AI-authored group carries both records. A tool should surface the mix
    /// rather than flattening it into a single label.
    ///
    /// Nothing is merged or resolved. Records from different prims stay
    /// distinct even when they share an instance name, since they describe
    /// different authoring steps.
    ///
    /// \p searchPrimStack behaves as it does for GetAllOnStage().
    USDMEDIA_API
    static std::vector<UsdMediaAuthorshipAPI>
    ComputeAccumulatedRecords(const UsdPrim &prim,
                              bool searchPrimStack = false);

    /// Returns the records on \p prim and everything beneath it, sorted by prim
    /// path then instance name.
    ///
    /// \p prim itself is included, matching UsdPrimRange. Prims beneath a
    /// native instance are not visited, since their records live on the
    /// prototype; use GetAllOnStage() to reach those, or query the prototype
    /// directly.
    ///
    /// \p searchPrimStack behaves as it does for GetAllOnStage().
    USDMEDIA_API
    static std::vector<UsdMediaAuthorshipAPI>
    GetAllUnder(const UsdPrim &prim, bool searchPrimStack = false);

    /// Returns the path of every authorship record authored in \p layer, sorted,
    /// in the <tt><primPath>.authorship:instanceName</tt> form that
    /// Get(const UsdStagePtr&, const SdfPath&) accepts and
    /// IsAuthorshipAPIPath() parses.
    ///
    /// This inspects one layer's scene description directly and composes
    /// nothing, so records in its sublayers and reference targets are not
    /// included. Use it to ask what a particular layer contributes, for example
    /// to check a DCC's export. Because it does not compose, it reaches specs
    /// that no stage would report, including those inside variants that are not
    /// selected.
    ///
    /// Paths are returned rather than schema objects because a record found
    /// this way may have no composed prim to attach one to. Values can be read
    /// from the layer's specs, or the paths can be passed to
    /// Get(const UsdStagePtr&, const SdfPath&) once a stage is at hand.
    USDMEDIA_API
    static SdfPathVector
    GetAllInLayer(const SdfLayerHandle &layer);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
