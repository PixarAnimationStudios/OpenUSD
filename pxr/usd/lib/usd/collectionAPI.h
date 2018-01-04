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
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

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
/// set of paths to exclude (beneath the included paths). Whether the 
/// descendants of an included path are members of a collection are decided 
/// by it's expansion rule (see below). A collection shall not exclude a 
/// path that's not already included by it. This rule isn't enforced by the 
/// authoring API, but can be validated using the UsdCollectionAPI::Validate()
/// method. 
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
/// <li><b>rel collection:<i>collectionName</i>:includes</b> - specifies a list 
/// of targets that are included in the collection. This can target prims or 
/// properties directly. A collection can include another collection by 
/// making its <i>includes</i> relationship target the <b>collection:{collectionName}</b> 
/// property on the owning prim of the collection to be included.
/// Such a property may not (and typically does not) exist on the UsdStage, but 
/// it is the path that is used to refer to the collection. When a collection 
/// When a collection includes one or more collections, the order in which 
/// targets are added to the includes relationship may become significant, if 
/// there are conflicting opinions about the same path. Targets that are added 
/// later are considered to be stronger than earlier targets. 
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
/// <b>Creating collections in C++</b>
/// 
/// \snippet examples.cpp ApplyCollections
/// 
///
class UsdCollectionAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = false;

    /// Construct a UsdCollectionAPI on UsdPrim \p prim .
    /// Equivalent to UsdCollectionAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdCollectionAPI(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdCollectionAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdCollectionAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdCollectionAPI(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USD_API
    virtual ~UsdCollectionAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdCollectionAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdCollectionAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USD_API
    static UsdCollectionAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Mark this schema class as applied to the prim at \p path in the 
    /// current EditTarget. This information is stored in the apiSchemas
    /// metadata on prims.  
    ///
    /// \sa UsdPrim::GetAppliedSchemas()
    ///
    USD_API
    static UsdCollectionAPI 
    Apply(const UsdStagePtr &stage, const SdfPath &path);

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

    /// Returns the collection named, \p name on the given prim, \p prim.
    USD_API
    static UsdCollectionAPI GetCollection(const UsdPrim &prim, const TfToken &name);

    /// Returns all the named collections on the given USD prim. 
    USD_API
    static std::vector<UsdCollectionAPI> GetAllCollections(const UsdPrim &prim);

    /// Returns whether the collection is valid. A collection is said to be 
    /// valid if it has a non-empty name and has a valid "expansionRule" 
    /// attribute.
    explicit operator bool() {
        return !_name.IsEmpty() && _GetExpansionRuleAttr();
    }

    /// Represents a flattened view of a collection. An object of this class 
    /// is computed by calling UsdCollectionAPI::ComputeMembershipQuery() on a 
    /// collection. It can be used to answer queries about membership of paths 
    /// in the collection efficiently.
    class MembershipQuery {
    public:
        /// Default Constructor, creates an empty MembershipQuery object for 
        /// passing into UsdCollectionAPI::CreateMembershipQuery() via a 
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
    private:
        // Holds an ordered map describing membership of paths in this collection
        // and the associated expansionRule for how the paths are to be expanded. 
        // If a collection includes another collection, the included collection's
        // _PathExpansionRuleMap is merged into this one. 
        // If a path is excluded, its expansion rule is set to 
        // UsdTokens->exclude.
        using _PathExpansionRuleMap = std::map<SdfPath, TfToken>;

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

        // Returns a raw map of the paths included or excluded in the 
        // collection along with the expansion rules for the included 
        // paths.
        USD_API
        const _PathExpansionRuleMap &_GetPathExpansionRuleMap() const {
            return _pathExpansionRuleMap;
        }

        friend class UsdCollectionAPI;

        _PathExpansionRuleMap _pathExpansionRuleMap;

        bool _hasExcludes=false;
    };

    /// Returns the name of the collection.
    USD_API
    TfToken GetName() const {
        return _name;
    }

    /// Returns the canonical path that represents this collection. 
    /// This points to a property named "collection:{collectionName}" on the 
    /// prim defining the collection (which won't really exist as a property 
    /// on the UsdStage, but will be used to refer to the collection).
    /// This is the path to be used to "include" this collection in another
    /// collectio.
    USD_API
    SdfPath GetCollectionPath() const;


    /// Returns true if a property with the given base-name, \p baseName 
    /// could be collection schema property.
    USD_API
    static bool IsSchemaPropertyBaseName(const TfToken &baseName);

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
    /// Note that there may be cases where the collection has no objects 
    /// included in it even when IsEmpty() returns false. For example, 
    /// if the included objects are unloaded or if the included objects 
    /// are also excluded.
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

    /// \anchor UsdCollectionAPI_RawProperties
    /// \name Collection Property API
    /// 
    /// API for getting and creating the "raw" properties associated with a 
    /// collection. 
    /// 
    /// @{

    /// Returns the "expansionRule" attribute of the collection if it exists.
    /// 
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    ///
    USD_API 
    UsdAttribute GetExpansionRuleAttr() const;


    /// Creates the "expansionRule" attribute associated with the collection.
    /// 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default.
    ///
    /// \sa GetExpansionRuleAttr()
    ///
    USD_API
    UsdAttribute CreateExpansionRuleAttr(
        const VtValue &defaultValue=VtValue()) const;

    /// Returns the "includes" relationship of the collection if it exists.
    USD_API 
    UsdRelationship GetIncludesRel() const;
    
    /// Creates the "includes" relationship of the collection if it doesn't 
    /// already exist and returns it.
    USD_API
    UsdRelationship CreateIncludesRel() const;

    /// Returns the "excludes" relationship of the collection if it exists.
    USD_API 
    UsdRelationship GetExcludesRel() const;

    /// Creates the "excludes" relationship of the collection if it doesn't 
    /// already exist and returns it.
    USD_API 
    UsdRelationship CreateExcludesRel() const;

    /// @}

    /// Validates the collection by checking the following rules:
    /// * a collection's expansionRule should be one of "explicitOnly", 
    ///   "expandPrims" or "expandPrimsAndProperties".
    /// * a collection should not exclude any paths that are not 
    ///   included in it.
    /// * a collection should not have have a circular dependency on 
    ///   another collection.
    USD_API 
    bool Validate(std::string *reason) const; 

    /// Clears both includes and excludes targets of the collection in the 
    /// current UsdEditTarget.
    USD_API
    bool ClearCollection() const; 

    /// Blocks the targets of the includes and excludes relationships of the 
    /// collection, causing the collection to become empty (if there are no
    /// opinions in stronger edit targets).
    USD_API
    bool BlockCollection() const;

private:
    // Constructor for creating a collection with the given name on \p prim 
    // with the given \p expansionRule.
    UsdCollectionAPI(const UsdPrim& prim, 
                     const TfToken &name, 
                     const TfToken &expansionRule);
    
    // Constructor to initialize a UsdCollectionAPI object for an already 
    // existing collection.
    UsdCollectionAPI(const UsdPrim& prim, 
                     const TfToken &name);

    // Returns the collection:<name>:expansionRule attribute of the collection.
    USD_API
    UsdAttribute _GetExpansionRuleAttr(bool create=false) const;

    // Returns the collection:<name>:includes relationship.
    UsdRelationship _GetIncludesRel(bool create=false) const;

    // Returns the collection:<name>:excludes relationship.
    UsdRelationship _GetExcludesRel(bool create=false) const;

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

    // The name of the collection.
    TfToken _name;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
