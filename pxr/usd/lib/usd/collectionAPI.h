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
#ifndef USD_GENERATED_COLLECTIONAPI_H
#define USD_GENERATED_COLLECTIONAPI_H

/// \file usd/collectionAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/usd/primFlags.h"
#include "pxr/usd/usd/tokens.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// COLLECTIONAPI                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdCollectionAPI
///
/// This is a general purpose API schema, used to describe a 
/// collection of heterogeneous objects within the scene. "Objects" here may be 
/// prims or properties belonging to prims or other collections. It's an add-on 
/// schema that can be applied many times to a prim with different collection 
/// names. 
/// 
/// A collection allows an enumeration of a set of paths to include and a 
/// set of paths to exclude.  Whether the descendants of an included
/// path are members of a collection are decided by its expansion rule
/// (see below).  If the collection excludes paths that are not descendents
/// of included paths, the collection implicitly includes the root path
/// &lt;/&gt;.  If such a collection also includes paths that are not
/// descendants of the excluded paths, it is considered invalid, since
/// the intention is ambiguous.
/// 
/// All the properties authored by the schema are namespaced under
/// "collection:". The given name of the collection provides additional 
/// namespacing for the various per-collection properties, which include the 
/// following:
/// 
/// <ul><li><b>uniform token collection:<i>collectionName</i>:expansionRule</b> - 
/// specified how the paths that are included in the collection must be expanded 
/// to determine its members. Possible values include:
/// <ul>
/// <li><b>explicitOnly</b> - only paths in the includes rel targets and not 
/// in the excludes rel targets belong to the collection.
/// </li>
/// <li><b>expandPrims</b> - all the prims at or below the includes rel-
/// targets (and not under the excludes rel-targets) belong to the 
/// collection.  Any property paths included in the collection would, of 
/// course, also be honored. This is the default behavior as it satisfies 
/// most use cases.
/// </li>
/// <li><b>expandPrimsAndProperties</b> - like expandPrims, but also 
/// includes all properties on all matched prims.  We're still not quite 
/// sure what the use cases are for this, but you can use it to capture a 
/// whole lot of UsdObjects very concisely.
/// </li>
/// </ul>
/// </li>
/// <li><b>rel collection:<i>collectionName</i>:includeRoot</b> - boolean
/// attribute indicating whether the pseudo-root path &lt;/&gt; should
/// be counted as one of the included target paths.  The fallback is false.
/// This separate attribute is required because relationships cannot
/// directly target the root.
/// <li><b>rel collection:<i>collectionName</i>:includes</b> - specifies a list 
/// of targets that are included in the collection. This can target prims or 
/// properties directly. A collection can insert the rules of another
/// collection by making its <i>includes</i> relationship target the
/// <b>collection:{collectionName}</b> property on the owning prim of the
/// collection to be included.
/// Such a property may not (and typically does not) exist on the UsdStage, but 
/// it is the path that is used to refer to the collection.
/// It is important to note that including another collection does not
/// guarantee the contents of that collection will be in the final collection;
/// instead, the rules are merged.  This means, for example, an exclude
/// entry may exclude a portion of the included collection.
/// When a collection includes one or more collections, the order in which 
/// targets are added to the includes relationship may become significant, if 
/// there are conflicting opinions about the same path. Targets that are added 
/// later are considered to be stronger than earlier targets for the same path.
/// </li>
/// <li><b>rel collection:<i>collectionName</i>:excludes</b> - specifies a list 
/// of targets that are excluded below the <b>included</b> paths in this 
/// collection. This can target prims or properties directly, but <b>cannot
/// target another collection</b>. This is to keep the membership determining 
/// logic simple, efficient and easier to reason about. Finally, it is invalid 
/// for a collection to exclude paths that are not included in it. The presence
/// of such "orphaned" excluded paths will not affect the set of paths included 
/// in the collection, but may affect the performance of querying membership of 
/// a path in the collection (see UsdCollectionAPI::MembershipQuery::IsPathIncluded) 
/// or of enumerating the objects belonging to the collection (see 
/// UsdCollectionAPI::GetIncludedObjects).
/// </li></ul>
/// 
/// <b>Implicit inclusion</b>
/// 
/// In some scenarios it is useful to express a collection that includes
/// everything except certain paths.  To support this, a collection
/// that has an exclude that is not a descendent of any include
/// will include the root path &lt;/&gt;.
/// 
/// <b>Creating collections in C++</b>
/// 
/// \snippet examples.cpp ApplyCollections
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdTokens.
/// So to set an attribute to the value "rightHanded", use UsdTokens->rightHanded
/// as the value.
///
class UsdCollectionAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::MultipleApplyAPI;

    /// Construct a UsdCollectionAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdCollectionAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "collection:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdCollectionAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdCollectionAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdCollectionAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdCollectionAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USD_API
    virtual ~UsdCollectionAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(
        bool includeInherited=true, const TfToken instanceName=TfToken());

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdCollectionAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.collection:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdCollectionAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USD_API
    static UsdCollectionAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdCollectionAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdCollectionAPI(prim, name);
    USD_API
    static UsdCollectionAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Checks if the given name \p baseName is the base name of a property
    /// of CollectionAPI.
    USD_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// CollectionAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USD_API
    static bool
    IsCollectionAPIPath(const SdfPath &path, TfToken *name);
private:

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "CollectionAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'CollectionAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdCollectionAPI object is returned upon success. 
    /// An invalid (or empty) UsdCollectionAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_MultipleApplyAPISchema() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    static UsdCollectionAPI 
    _Apply(const UsdPrim &prim, const TfToken &name);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USD_API
    virtual UsdSchemaType _GetSchemaType() const;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USD_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // EXPANSIONRULE 
    // --------------------------------------------------------------------- //
    /// Specifies how the paths that are included in
    /// the collection must be expanded to determine its members.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USD_API
    UsdAttribute GetExpansionRuleAttr() const;

    /// See GetExpansionRuleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateExpansionRuleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INCLUDEROOT 
    // --------------------------------------------------------------------- //
    /// Boolean attribute indicating whether the pseudo-root
    /// path &lt;/&gt; should be counted as one of the included target
    /// paths.  The fallback is false.  This separate attribute is
    /// required because relationships cannot directly target the root.
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USD_API
    UsdAttribute GetIncludeRootAttr() const;

    /// See GetIncludeRootAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateIncludeRootAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INCLUDES 
    // --------------------------------------------------------------------- //
    /// Specifies a list of targets that are included in the collection.
    /// This can target prims or properties directly. A collection can insert
    /// the rules of another collection by making its <i>includes</i>
    /// relationship target the <b>collection:{collectionName}</b> property on
    /// the owning prim of the collection to be included
    ///
    USD_API
    UsdRelationship GetIncludesRel() const;

    /// See GetIncludesRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USD_API
    UsdRelationship CreateIncludesRel() const;

public:
    // --------------------------------------------------------------------- //
    // EXCLUDES 
    // --------------------------------------------------------------------- //
    /// Specifies a list of targets that are excluded below
    /// the included paths in this collection. This can target prims or
    /// properties directly, but cannot target another collection. This is to
    /// keep the membership determining logic simple, efficient and easier to
    /// reason about. Finally, it is invalid for a collection to exclude
    /// paths that are not included in it. The presence of such "orphaned"
    /// excluded paths will not affect the set of paths included in the
    /// collection, but may affect the performance of querying membership of 
    /// a path in the collection (see
    /// UsdCollectionAPI::MembershipQuery::IsPathIncluded) 
    /// or of enumerating the objects belonging to the collection (see 
    /// UsdCollectionAPI::GetIncludedObjects).
    ///
    USD_API
    UsdRelationship GetExcludesRel() const;

    /// See GetExcludesRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USD_API
    UsdRelationship CreateExcludesRel() const;

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

public:
    /// Adds a new collection named \p name on the given prim, \p prim with the 
    /// specified expansion-rule, \p expansionRule.
    /// 
    /// If a collection named \p name already exists, its expansion-rule is 
    /// updated with the provided value and it is returned.
    /// 
    /// The name of a collection, \p name may itself be namespaced, to facilitate 
    /// organization of collections into groups. However, the base-name of a 
    /// collection (i.e. the last component of the collection name) should not 
    /// be the same as one of the core collection schema properties,
    /// i.e. should not be 'expansionRule' or 'includes' or 'excludes'.
    USD_API
    static UsdCollectionAPI ApplyCollection(
        const UsdPrim& prim, 
        const TfToken &name, 
        const TfToken &expansionRule=UsdTokens->expandPrims);

    /// Returns the collection represented by the given collection path, 
    /// \p collectionPath on the given USD stage.
    USD_API
    static UsdCollectionAPI GetCollection(const UsdStagePtr &stage, 
                                          const SdfPath &collectionPath);

    /// Returns the schema object representing a collection named \p name on 
    /// the given \p prim.
    USD_API
    static UsdCollectionAPI GetCollection(const UsdPrim &prim, 
                                          const TfToken &name);

    /// Returns all the named collections on the given USD prim. 
    USD_API
    static std::vector<UsdCollectionAPI> GetAllCollections(const UsdPrim &prim);

    /// Represents a flattened view of a collection. An object of this class 
    /// is computed by calling UsdCollectionAPI::ComputeMembershipQuery() on a 
    /// collection. It can be used to answer queries about membership of paths 
    /// in the collection efficiently.
    class MembershipQuery {
    public:
        /// Holds an unordered map describing membership of paths in
        /// this collection and the associated expansionRule for how the
        /// paths are to be expanded. If a collection includes another
        /// collection, the included collection's PathExpansionRuleMap
        /// is merged into this one. If a path is excluded, its
        /// expansion rule is set to UsdTokens->exclude.
        using PathExpansionRuleMap = std::unordered_map<SdfPath, 
              TfToken, SdfPath::Hash>;

        /// Default Constructor, creates an empty MembershipQuery object for 
        /// passing into UsdCollectionAPI::ComputeMembershipQuery() via a 
        /// pointer.
        MembershipQuery() {}
        
        /// \overload
        /// Returns whether the given path is included in the collection from 
        /// which this MembershipQuery object was computed. This is the API that 
        /// clients should use for determining if a given object is a member of 
        /// the collection. To enumerate all the members of a collection, use 
        /// \ref UsdCollectionAPI::ComputeIncludedObjects or 
        /// \ref UsdCollectionAPI::ComputeIncludedPaths.
        /// 
        /// If \p expansionRule is not NULL, it is set to the expansion-
        /// rule value that caused the path to be included in or excluded from 
        /// the collection. If \p path is not included in the collection, 
        /// \p expansionRule is set to UsdTokens->exclude. 
        /// 
        /// It is useful to specify this parameter and use this overload of 
        /// IsPathIncluded(), when you're interested in traversing a subtree 
        /// and want to know whether the root of the subtree is included in a 
        /// collection. For evaluating membership of descendants of the root, 
        /// please use the other overload of IsPathIncluded(), that takes both 
        /// a path and the parent expansionRule.
        /// 
        /// The python version of this method only returns the boolean result. 
        /// It does not return \p expansionRule.
        USD_API
        bool IsPathIncluded(const SdfPath &path, 
                            TfToken *expansionRule=nullptr) const;

        /// \overload
        /// Returns whether the given path, \p path is included in the 
        /// collection from which this MembershipQuery object was computed, 
        /// given the parent-path's inherited expansion rule, 
        /// \p parentExpansionRule.
        /// 
        /// If \p expansionRule is not NULL, it is set to the expansion-
        /// rule value that caused the path to be included in or excluded from 
        /// the collection. If \p path is not included in the collection, 
        /// \p expansionRule is set to UsdTokens->exclude. 
        /// 
        /// The python version of this method only returns the boolean result. 
        /// It does not return \p expansionRule.
        USD_API
        bool IsPathIncluded(const SdfPath &path,
                            const TfToken &parentExpansionRule,
                            TfToken *expansionRule=nullptr) const;

        /// Returns true if the collection excludes one or more paths below an
        /// included path.
        bool HasExcludes() const {
            return _hasExcludes;
        }

        /// Equality operator
        bool operator==(MembershipQuery const& rhs) const {
            return _hasExcludes == rhs._hasExcludes &&
                _pathExpansionRuleMap == rhs._pathExpansionRuleMap;
        }

        /// Inequality operator
        bool operator!=(MembershipQuery const& rhs) const {
            return !(*this == rhs);
        }

        /// Hash functor
        struct Hash {
            USD_API
            size_t operator()(MembershipQuery const& query) const;
        };

        /// Hash function
        inline size_t GetHash() const {
            return Hash()(*this);
        }

        /// Returns a raw map of the paths included or excluded in the 
        /// collection along with the expansion rules for the included 
        /// paths.
        USD_API
        PathExpansionRuleMap GetAsPathExpansionRuleMap() const {
            return _pathExpansionRuleMap;
        }

    private:

        // Add \p path as an included path in the MembershipQuery with the 
        // given expansion rule, \p expansionRule.
        void _AppendIncludedPath(
            const SdfPath &path,
            const TfToken &expansionRule);

        // Adds \p path as an excluded path in this MembershipQuery.
        // Note that this opinion is not pruning (i.e. descendant paths 
        // beneath the excluded path may be included).
        void _AppendExcludedPath(const SdfPath &path);

        // Merge the given flattened membership-query object into this one.
        // Opinions in \p query will be stronger that (i.e. will override) any 
        // existing opinions for overlapping paths.
        void _MergeMembershipQuery(const MembershipQuery &query);

        friend class UsdCollectionAPI;

        PathExpansionRuleMap _pathExpansionRuleMap;

        // A cached flag indicating whether _pathExpansionRuleMap contains
        // any exclude rules.
        bool _hasExcludes=false;
    };

    /// Returns the canonical path that represents this collection. 
    /// This points to a property named "collection:{collectionName}" on the 
    /// prim defining the collection (which won't really exist as a property 
    /// on the UsdStage, but will be used to refer to the collection).
    /// This is the path to be used to "include" this collection in another
    /// collection.
    USD_API
    SdfPath GetCollectionPath() const;

    /// Returns the canonical path to the collection named, \p name on the given
    /// prim, \p prim.
    /// 
    /// \sa GetCollectionPath()
    USD_API
    static SdfPath GetNamedCollectionPath(
        const UsdPrim &prim, 
        const TfToken &collectionName);

    /// Returns true if \p path points to a collection.
    /// i.e., if it is a property path and the property name is of the 
    /// form "collection:{collectionName}.
    /// If \p collectionName is a valid pointer to a string, it is 
    /// populated with the name of the collection if path is a valid 
    /// collection path.
    USD_API
    static bool IsCollectionPath(
        const SdfPath &path,
        TfToken *collectionName);

    /// Computes and returns a MembershipQuery object which can be used to query
    /// inclusion or exclusion of paths in the collection.
    USD_API
    MembershipQuery ComputeMembershipQuery() const; 

    /// Populates the MembershipQuery object with data from this collection, so 
    /// it can be used to query inclusion or exclusion of paths.
    USD_API
    void ComputeMembershipQuery(MembershipQuery *query) const; 

    /// Returns true if the collection has nothing included in it.
    /// This requires both that the includes relationship have no
    /// target paths, and that the includeRoot attribute be false.
    /// Note that there may be cases where the collection has no objects 
    /// included in it even when HasNoIncludedPaths() returns false.
    /// For example, if the included objects are unloaded or if the
    /// included objects are also excluded.
    USD_API
    bool HasNoIncludedPaths() const;

    /// Returns all the usd objects that satisfy the predicate, \p pred in the
    /// collection represented by the MembershipQuery object, \p query.
    /// 
    /// The results depends on the load state of the UsdStage, \p stage.
    USD_API
    static std::set<UsdObject> ComputeIncludedObjects(
        const MembershipQuery &query,
        const UsdStageWeakPtr &stage,
        const Usd_PrimFlagsPredicate &pred=UsdPrimDefaultPredicate);

    /// Returns all the paths that satisfy the predicate, \p pred in the
    /// collection represented by the MembershipQuery object, \p query.
    /// 
    /// The result depends on the load state of the UsdStage, \p stage.
    USD_API
    static SdfPathSet ComputeIncludedPaths(
        const MembershipQuery &query,
        const UsdStageWeakPtr &stage,
        const Usd_PrimFlagsPredicate &pred=UsdPrimDefaultPredicate);

    /// @}

    /// \anchor UsdCollectionAPI_AuthoringAPI
    /// \name Collection Authoring API
    /// 
    /// Convenience API for adding or removing prims and properties to (or 
    /// from) a collection..
    /// 
    /// @{

    /// Includes or adds the given path, \p pathToInclude in the collection. 
    /// 
    /// This does nothing if the path is already included in the collection. 
    /// 
    /// This does not modify the expansion-rule of the collection. Hence, if the 
    /// expansionRule is <i>expandPrims</i> or <i>expandPrimsAndProperties</i>, 
    /// then the descendants of \p pathToInclude will be also included in the 
    /// collection unless explicitly excluded.
    /// 
    /// \sa UsdCollectionAPI::ExcludePath()
    USD_API 
    bool IncludePath(const SdfPath &pathToInclude) const;

    /// Excludes or removes the given path, \p pathToExclude from the collection.
    /// 
    /// If the collection is empty, the collection becomes one that
    /// includes all paths except the givne path.  Otherwise, this does
    /// nothing if the path is not included in the collection. 
    ///
    /// This does not modify the expansion-rule of the collection. Hence, if the 
    /// expansionRule is <i>expandPrims</i> or <i>expandPrimsAndProperties</i>, 
    /// then the descendants of \p pathToExclude will also be excluded from the 
    /// collection, unless explicitly included.
    ///
    /// \sa UsdCollectionAPI::IncludePath()
    USD_API
    bool ExcludePath(const SdfPath &pathToExclude) const; 

    /// @}
    
    /// Validates the collection by checking the following rules:
    /// * a collection's expansionRule should be one of "explicitOnly", 
    ///   "expandPrims" or "expandPrimsAndProperties".
    /// * a collection should not have have a circular dependency on 
    ///   another collection.
    /// * a collection should not have both includes and excludes
    ///   among its top-level rules
    USD_API 
    bool Validate(std::string *reason) const; 

    /// Resets the collection by clearing both the includes and excludes 
    /// targets of the collection in the current UsdEditTarget.
    /// 
    /// \note This does not modify the "includeRoot" attribute which is used 
    /// to include or exclude everything (i.e. the pseudoRoot) in the USD stage.
    USD_API
    bool ResetCollection() const; 

    /// Blocks the targets of the includes and excludes relationships of the 
    /// collection, making it 
    /// <* <i>empty</i> if "includeRoot" is false (or unset) or
    /// * <i>include everything</i> if "includeRoot" is true.
    /// (assuming there are no opinions in stronger edit targets).
    USD_API
    bool BlockCollection() const;

private:

    // Returns the name of the property belonging to this collection, given the 
    // base name of the attribute. Eg, if baseName is 'includes', this 
    // returns 'collection:<name>:includes'.
    TfToken _GetCollectionPropertyName(const TfToken &baseName=TfToken()) const;

    // Helper method for computing the MembershipQuery object for a collection. 
    // This makes recursive calls if the collection includes other collections.
    // \p chainedCollectionPaths is used to pass in the set of all seen and
    // included collections in the dependency chain and is used to detect 
    // circular dependencies.
    // If \p foundCircularDependency is not NULL, it is set to true if a 
    // circular dependency is detected amongst the included collections.
    // If it is NULL, a warning is issued when a circular dependency is 
    // detected.
    void _ComputeMembershipQueryImpl(
        MembershipQuery *query,
        const SdfPathSet &chainedCollectionPaths,
        bool *foundCircularDependency=nullptr) const; 

    // Helper method used by ComputeIncludedObjects() and 
    // ComputeIncludedPaths().
    static void _ComputeIncludedImpl(
        const MembershipQuery &query,
        const UsdStageWeakPtr &stage,
        const Usd_PrimFlagsPredicate &pred,
        std::set<UsdObject> *includedObjects,
        SdfPathSet *includedPaths);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
