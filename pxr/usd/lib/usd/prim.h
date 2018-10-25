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
#ifndef USD_PRIM_H
#define USD_PRIM_H

/// \file usd/prim.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/primFlags.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"

#include "pxr/usd/sdf/path.h"

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range/iterator_range.hpp>

#include <string>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;
class UsdPrimRange;
class Usd_PrimData;

class UsdAttribute;
class UsdRelationship;
class UsdReferences;
class UsdSchemaBase;
class UsdAPISchemaBase;
class UsdInherits;
class UsdSpecializes;
class UsdVariantSets;
class UsdVariantSet;

class SdfPayload;

class UsdPrimSiblingIterator;
typedef boost::iterator_range<UsdPrimSiblingIterator> UsdPrimSiblingRange;

class UsdPrimSubtreeIterator;
typedef boost::iterator_range<UsdPrimSubtreeIterator> UsdPrimSubtreeRange;

/// \class UsdPrim
///
/// UsdPrim is the sole persistent scenegraph object on a UsdStage, and
/// is the embodiment of a "Prim" as described in the <em>Universal Scene
/// Description Composition Compendium</em>
///
/// A UsdPrim is the principal container of other types of scene description.
/// It provides API for accessing and creating all of the contained kinds
/// of scene description, which include:
/// \li UsdVariantSets - all VariantSets on the prim (GetVariantSets(), GetVariantSet())
/// \li UsdReferences - all references on the prim (GetReferences())
/// \li UsdInherits - all inherits on the prim (GetInherits())
/// \li UsdSpecializes - all specializes on the prim (GetSpecializes())
///
/// As well as access to the API objects for properties contained within the 
/// prim - UsdPrim as well as all of the following classes are subclasses
/// of UsdObject:
/// \li UsdProperty - generic access to all attributes and relationships.
/// A UsdProperty can be queried and cast to a UsdAttribute or UsdRelationship
/// using UsdObject::Is<>() and UsdObject::As<>(). (GetPropertyNames(), 
/// GetProperties(), GetPropertiesInNamespace(), GetPropertyOrder(),
/// SetPropertyOrder())
/// \li UsdAttribute - access to default and timesampled attribute values, as 
/// well as value resolution information, and attribute-specific metadata 
/// (CreateAttribute(), GetAttribute(), GetAttributes(), HasAttribute())
/// \li UsdRelationship - access to authoring and resolving relationships
/// to other prims and properties (CreateRelationship(), GetRelationship(), 
/// GetRelationships(), HasRelationship())
///
/// UsdPrim also provides access to iteration through its prim children,
/// optionally making use of the \ref primFlags.h "prim predicates facility" 
/// (GetChildren(), GetAllChildren(), GetFilteredChildren()).
///
/// \section Lifetime Management
///
/// Clients acquire UsdPrim objects, which act like weak/guarded pointers
/// to persistent objects owned and managed by their originating UsdStage.
/// We provide the following guarantees for a UsdPrim acquired via 
/// UsdStage::GetPrimAtPath() or UsdStage::OverridePrim() or 
/// UsdStage::DefinePrim():
/// \li As long as no further mutations to the structure of the UsdStage
///     are made, the UsdPrim will still be valid.  Loading and
///     Unloading are considered structural mutations.
/// \li When the UsdStage's structure \em is mutated, the thread performing
///     the mutation will receive a UsdNotice::ObjectsChanged notice
///     after the stage has been reconfigured, which provides details as to
///     what prims may have been created or destroyed, and what prims
///     may simply have changed in some structural way.
///
/// Prim access in "reader" threads should be limited to GetPrimAtPath(), which
/// will never cause a mutation to the Stage or its layers.
///
/// Please refer to \ref UsdNotice for a listing of
/// the events that could cause UsdNotice::ObjectsChanged to be emitted.
class UsdPrim : public UsdObject
{
public:
    /// Convenience typedefs.
    typedef UsdPrimSiblingIterator SiblingIterator;
    typedef UsdPrimSiblingRange SiblingRange;

    /// Convenience typedefs.
    typedef UsdPrimSubtreeIterator SubtreeIterator;
    typedef UsdPrimSubtreeRange SubtreeRange;

    /// Construct an invalid prim.
    UsdPrim() : UsdObject() {}

    /// Return this prim's definition from the UsdSchemaRegistry based on the
    /// prim's type if one exists, otherwise return null.
    USD_API
    SdfPrimSpecHandle GetPrimDefinition() const;

    /// Return this prim's composed specifier.
    SdfSpecifier GetSpecifier() const { return _Prim()->GetSpecifier(); };

    /// Return a list of PrimSpecs that provide opinions for this prim
    /// (i.e. the prim's metadata fields, including composition
    /// metadata). These specs are ordered from strongest to weakest opinion.
    ///
    /// \note The results returned by this method are meant for debugging
    /// and diagnostic purposes.  It is **not** advisable to retain a 
    /// PrimStack for the purposes of expedited value resolution for prim
    /// metadata, since not all metadata resolves with simple "strongest
    /// opinion wins" semantics.
    USD_API
    SdfPrimSpecHandleVector GetPrimStack() const;

    /// Author an opinion for this Prim's specifier at the current edit
    /// target.
    bool SetSpecifier(SdfSpecifier specifier) const {
        return SetMetadata(SdfFieldKeys->Specifier, specifier);
    }

    /// Return this prim's composed type name.  Note that this value is
    /// cached and is efficient to query.
    const TfToken &GetTypeName() const { return _Prim()->GetTypeName(); };

    /// Author this Prim's typeName at the current EditTarget.
    bool SetTypeName(const TfToken & typeName) const {
        return SetMetadata(SdfFieldKeys->TypeName, typeName);
    }

    /// Clear the opinion for this Prim's typeName at the current edit
    /// target.
    bool ClearTypeName() const {
        return ClearMetadata(SdfFieldKeys->TypeName);
    }

    /// Return true if a typeName has been authored.
    bool HasAuthoredTypeName() const {
        return HasAuthoredMetadata(SdfFieldKeys->TypeName);
    }

    /// Return true if this prim is active, meaning neither it nor any of its
    /// ancestors have active=false.  Return false otherwise.
    bool IsActive() const { return _Prim()->IsActive(); }

    /// Author 'active' metadata for this prim at the current EditTarget.
    bool SetActive(bool active) const {
        return SetMetadata(SdfFieldKeys->Active, active);
    }

    /// Remove the authored 'active' opinion at the current EditTarget.  Do
    /// nothing if there is no authored opinion.
    bool ClearActive() const {
        return ClearMetadata(SdfFieldKeys->Active);
    }

    /// Return true if this prim has an authored opinion for 'active', false
    /// otherwise.
    bool HasAuthoredActive() const {
        return HasAuthoredMetadata(SdfFieldKeys->Active);
    }

    /// Return true if this prim is active, and \em either it is loadable and
    /// it is loaded, \em or its nearest loadable ancestor is loaded, \em or it
    /// has no loadable ancestor; false otherwise.
    bool IsLoaded() const { return _Prim()->IsLoaded(); }

    /// Return true if this prim is a model based on its kind metadata, false
    /// otherwise.
    bool IsModel() const { return _Prim()->IsModel(); }

    /// Return true if this prim is a model group based on its kind metadata,
    /// false otherwise.  If this prim is a group, it is also necessarily a
    /// model.
    bool IsGroup() const { return _Prim()->IsGroup(); }

    /// Return true if this prim or any of its ancestors is a class.
    bool IsAbstract() const { return _Prim()->IsAbstract(); }

    /// Return true if this prim and all its ancestors have defining specifiers,
    /// false otherwise. \sa SdfIsDefiningSpecifier.
    bool IsDefined() const { return _Prim()->IsDefined(); }

    /// Return true if this prim has a specifier of type SdfSpecifierDef
    /// or SdfSpecifierClass. \sa SdfIsDefiningSpecifier
    bool HasDefiningSpecifier() const { 
        return _Prim()->HasDefiningSpecifier(); 
    }

    /// Return a vector containing the names of API schemas which have
    /// been applied to this prim, using the Apply() method on
    /// the particular schema class. 
    USD_API
    TfTokenVector GetAppliedSchemas() const;

    /// Alias for the "predicate" function parameter passed into the various
    /// Get{Authored}{PropertyNames,Properties} methods.
    using PropertyPredicateFunc = 
        std::function<bool (const TfToken &propertyName)>;

    /// Return all of this prim's property names (attributes and relationships),
    /// including all builtin properties.
    /// 
    /// If a valid \p predicate is passed in, then only properties whose names 
    /// pass the predicate are included in the result. This is useful if the 
    /// client is interested only in a subset of properties on the prim. For 
    /// example, only the ones in a given namespace or only the ones needed to 
    /// compute a value.
    /// 
    /// \sa GetAuthoredPropertyNames()
    /// \sa UsdProperty::IsAuthored()
    USD_API
    TfTokenVector GetPropertyNames(
        const PropertyPredicateFunc &predicate={}) const;

    /// Return this prim's property names (attributes and relationships) that
    /// have authored scene description, ordered according to the strongest
    /// propertyOrder statement in scene description if one exists, otherwise
    /// ordered according to TfDictionaryLessThan.
    /// 
    /// If a valid \p predicate is passed in, then only the authored properties 
    /// whose names pass the predicate are included in the result. This is 
    /// useful if the client is interested only in a subset of authored 
    /// properties on the prim. For example, only the ones in a given namespace 
    /// or only the ones needed to compute a value.
    ///
    /// \sa GetPropertyNames()
    /// \sa UsdProperty::IsAuthored() 
    USD_API
    TfTokenVector GetAuthoredPropertyNames(
        const PropertyPredicateFunc &predicate={}) const;

    /// Return all of this prim's properties (attributes and relationships),
    /// including all builtin properties, ordered by name according to the
    /// strongest propertyOrder statement in scene description if one exists,
    /// otherwise ordered according to TfDictionaryLessThan.
    ///
    /// If a valid \p predicate is passed in, then only properties whose names  
    /// pass the predicate are included in the result. This is useful if the 
    /// client is interested only in a subset of properties on the prim. For 
    /// example, only the ones in a given namespace or only the ones needed to 
    /// compute a value.
    ///
    /// To obtain only either attributes or relationships, use either
    /// GetAttributes() or GetRelationships().
    ///
    /// To determine whether a property is either an attribute or a
    /// relationship, use the UsdObject::As() and UsdObject::Is() methods in
    /// C++:
    ///
    /// \code
    /// // Use As<>() to obtain a subclass instance.
    /// if (UsdAttribute attr = property.As<UsdAttribute>()) {
    ///     // use attribute 'attr'.
    /// else if (UsdRelationship rel = property.As<UsdRelationship>()) {
    ///     // use relationship 'rel'.
    /// }
    ///
    /// // Use Is<>() to discriminate only.
    /// if (property.Is<UsdAttribute>()) {
    ///     // property is an attribute.
    /// }
    /// \endcode
    ///
    /// In Python, use the standard isinstance() function:
    ///
    /// \code
    /// if isinstance(property, Usd.Attribute):
    ///     # property is a Usd.Attribute.
    /// elif isinstance(property, Usd.Relationship):
    ///     # property is a Usd.Relationship.
    /// \endcode
    ///
    /// \sa GetAuthoredProperties()
    /// \sa UsdProperty::IsAuthored()
    USD_API
    std::vector<UsdProperty> GetProperties(
        const PropertyPredicateFunc &predicate={}) const;

    /// Return this prim's properties (attributes and relationships) that have
    /// authored scene description, ordered by name according to the strongest
    /// propertyOrder statement in scene description if one exists, otherwise
    /// ordered according to TfDictionaryLessThan.
    ///
    /// If a valid \p predicate is passed in, then only authored properties 
    /// whose names pass the predicate are included in the result. This is 
    /// useful if the client is interested only in a subset of authored 
    /// properties on the prim. For example, only the ones in a given namespace 
    /// or only the ones needed to compute a value.
    ///
    /// \sa GetProperties()
    /// \sa UsdProperty::IsAuthored()
    USD_API
    std::vector<UsdProperty> GetAuthoredProperties(
        const PropertyPredicateFunc &predicate={}) const;

    /// Return this prim's properties that are inside the given property
    /// namespace ordered according to the strongest propertyOrder statement in
    /// scene description if one exists, otherwise ordered according to
    /// TfDictionaryLessThan.
    ///
    /// A \p namespaces argument whose elements are ["ri", "attribute"] will
    /// return all the properties under the namespace "ri:attribute",
    /// i.e. "ri:attribute:*". An empty \p namespaces argument is equivalent to
    /// GetProperties().
    ///
    /// For details of namespaced properties, see \ref Usd_Ordering
    USD_API
    std::vector<UsdProperty>
    GetPropertiesInNamespace(const std::vector<std::string> &namespaces) const;

    /// \overload
    /// \p namespaces must be an already-concatenated ordered set of namespaces,
    /// and may or may not terminate with the namespace-separator character. If
    /// \p namespaces is empty, this method is equivalent to GetProperties().
    USD_API
    std::vector<UsdProperty>
    GetPropertiesInNamespace(const std::string &namespaces) const;

    /// Like GetPropertiesInNamespace(), but exclude properties that do not have
    /// authored scene description from the result.  See
    /// UsdProperty::IsAuthored().
    ///
    /// For details of namespaced properties, see \ref Usd_Ordering
    USD_API
    std::vector<UsdProperty>
    GetAuthoredPropertiesInNamespace(
        const std::vector<std::string> &namespaces) const;

    /// \overload
    /// \p namespaces must be an already-concatenated ordered set of namespaces,
    /// and may or may not terminate with the namespace-separator character. If
    /// \p namespaces is empty, this method is equivalent to
    /// GetAuthoredProperties().
    USD_API
    std::vector<UsdProperty>
    GetAuthoredPropertiesInNamespace(const std::string &namespaces) const;

    /// Return the strongest propertyOrder metadata value authored on this prim.
    USD_API
    TfTokenVector GetPropertyOrder() const;

    /// Author an opinion for propertyOrder metadata on this prim at the current
    /// EditTarget.
    USD_API
    void SetPropertyOrder(const TfTokenVector &order) const;

    /// Remove all scene description for the property with the
    /// given \p propName <em>in the current UsdEditTarget</em>.
    /// Return true if the property is removed, false otherwise.
    USD_API
    bool RemoveProperty(const TfToken &propName);

    /// Return a UsdProperty with the name \a propName. The property 
    /// returned may or may not \b actually exist so it must be checked for
    /// validity. Suggested use:
    ///
    /// \code
    /// if (UsdProperty myProp = prim.GetProperty("myProp")) {
    ///    // myProp is safe to use. 
    ///    // Edits to the owning stage requires subsequent validation.
    /// } else {
    ///    // myProp was not defined/authored
    /// }
    /// \endcode
    USD_API
    UsdProperty GetProperty(const TfToken &propName) const;

    /// Return true if this prim has an property named \p propName, false
    /// otherwise.
    USD_API
    bool HasProperty(const TfToken &propName) const;

private:
    // The non-templated implementation of UsdPrim::IsA using the
    // TfType system. \p validateSchemaType is provided for python clients
    // because they can't use compile time assertions on the input type.
    USD_API
    bool _IsA(const TfType& schemaType, bool validateSchemaType) const;

    // The non-templated implementation of UsdPrim::HasAPI using the
    // TfType system. 
    // 
    // \p validateSchemaType is provided for python clients
    // because they can't use compile time assertions on the input type.
    // 
    // \p instanceName is used to determine whether a particular instance 
    // of a multiple-apply API schema has been applied to the prim.
    USD_API
    bool _HasAPI(const TfType& schemaType, bool validateSchemaType,
                 const TfToken &instanceName) const;

public:
    /// Return true if the UsdPrim is/inherits a Schema of type T.
    ///
    /// This will also return true if the UsdPrim is a schema that inherits
    /// from schema \c T.
    template <typename T>
    bool IsA() const {
        static_assert(std::is_base_of<UsdSchemaBase, T>::value,
                      "Provided type must derive UsdSchemaBase.");
        return _IsA(TfType::Find<T>(), /*validateSchemaType=*/false);
    };
    
    /// Return true if prim type is/inherits a Schema with TfType \p schemaType
    USD_API
    bool IsA(const TfType& schemaType) const;

    /// Return true if the UsdPrim has had an API schema represented by the C++ 
    /// class type <b>T</b> applied to it through the Apply() method provided 
    /// on the API schema class. 
    /// 
    /// \p instanceName, if non-empty is used to determine if a particular 
    /// instance of a multiple-apply API schema (eg. UsdCollectionAPI) has been 
    /// applied to the prim. A coding error is issued if a non-empty 
    /// \p instanceName is passed in and <b>T</b> represents a single-apply API 
    /// schema.
    /// 
    /// <b>Using HasAPI in C++</b>
    /// \code 
    /// UsdPrim prim = stage->OverridePrim("/path/to/prim");
    /// UsdModelAPI modelAPI = UsdModelAPI::Apply(prim);
    /// assert(prim.HasAPI<UsdModelAPI>());
    /// 
    /// UsdCollectionAPI collAPI = UsdCollectionAPI::Apply(prim, 
    ///         /*instanceName*/ TfToken("geom"))
    /// assert(prim.HasAPI<UsdCollectionAPI>()
    /// assert(prim.HasAPI<UsdCollectionAPI>(/*instanceName*/ TfToken("geom")))
    /// \endcode
    /// 
    /// The python version of this method takes as an argument the TfType
    /// of the API schema class. Similar validation of the schema type is 
    /// performed in python at run-time and a coding error is issued if 
    /// the given type is not a valid applied API schema.
    /// 
    /// <b>Using HasAPI in Python</b>
    /// \code{.py}
    /// prim = stage.OverridePrim("/path/to/prim")
    /// modelAPI = Usd.ModelAPI.Apply(prim)
    /// assert prim.HasAPI(Usd.ModelAPI)
    /// 
    /// collAPI = Usd.CollectionAPI.Apply(prim, "geom")
    /// assert(prim.HasAPI(Usd.CollectionAPI))
    /// assert(prim.HasAPI(Usd.CollectionAPI, instanceName="geom"))
    /// \endcode
    template <typename T>
    bool HasAPI(const TfToken &instanceName=TfToken()) const {
        static_assert(std::is_base_of<UsdAPISchemaBase, T>::value,
                      "Provided type must derive UsdAPISchemaBase.");
        static_assert(!std::is_same<UsdAPISchemaBase, T>::value,
                      "Provided type must not be UsdAPISchemaBase.");
        static_assert(
            (T::schemaType == UsdSchemaType::SingleApplyAPI
            || T::schemaType == UsdSchemaType::MultipleApplyAPI),
            "Provided schema type must be an applied API schema.");

        if (T::schemaType != UsdSchemaType::MultipleApplyAPI
            && !instanceName.IsEmpty()) {
            TF_CODING_ERROR("HasAPI: single application API schemas like %s do "
                "not contain an application instanceName ( %s ).",
                TfType::GetCanonicalTypeName(typeid(T)).c_str(),
                instanceName.GetText());
            return false;
        }

        return _HasAPI(TfType::Find<T>(), /*validateSchemaType=*/false, 
                       instanceName);
    }
    
    /// Return true if a prim has an API schema with TfType \p schemaType.
    ///
    /// \p instanceName, if non-empty is used to determine if a particular 
    /// instance of a multiple-apply API schema (eg. UsdCollectionAPI) has been 
    /// applied to the prim. A coding error is issued if a non-empty 
    /// \p instanceName is passed in and <b>T</b> represents a single-apply API 
    /// schema.
    USD_API
    bool HasAPI(const TfType& schemaType,
                const TfToken& instanceName=TfToken()) const;

    // --------------------------------------------------------------------- //
    /// \name Prim Children
    // --------------------------------------------------------------------- //

    /// Return this prim's direct child named \p name if it has one, otherwise
    /// return an invalid UsdPrim.  Equivalent to:
    /// \code
    /// prim.GetStage()->GetPrimAtPath(prim.GetPath().AppendChild(name))
    /// \endcode
    USD_API
    UsdPrim GetChild(const TfToken &name) const;

    /// Return this prim's active, loaded, defined, non-abstract children as an
    /// iterable range.  Equivalent to:
    /// \code
    /// GetFilteredChildren(UsdPrimDefaultPredicate)
    /// \endcode
    ///
    /// See \ref Usd_PrimFlags "Prim predicate flags" 
    /// and #UsdPrimDefaultPredicate for more information.
    inline SiblingRange GetChildren() const;

    /// Return all this prim's children as an iterable range.
    inline SiblingRange GetAllChildren() const;

    /// Return a subset of all of this prim's children filtered by \p predicate
    /// as an iterable range.  The \p predicate is generated by combining a
    /// series of prim flag terms with either && or || and !.
    ///
    /// Example usage:
    /// \code
    /// // Get all active model children.
    /// GetFilteredChildren(UsdPrimIsActive && UsdPrimIsModel);
    ///
    /// // Get all model children that pass the default predicate.
    /// GetFilteredChildren(UsdPrimDefaultPredicate && UsdPrimIsModel);
    /// \endcode
    ///
    /// If this prim is an instance, no children will be returned unless
    /// #UsdTraverseInstanceProxies is used to allow instance proxies to be
    /// returned, or if this prim is itself an instance proxy.
    ///
    /// See \ref Usd_PrimFlags "Prim predicate flags" 
    /// and #UsdPrimDefaultPredicate for more information.
    inline SiblingRange
    GetFilteredChildren(const Usd_PrimFlagsPredicate &predicate) const;

    /// Return this prim's active, loaded, defined, non-abstract descendants as
    /// an iterable range.  Equivalent to:
    /// \code
    /// GetFilteredDescendants(UsdPrimDefaultPredicate)
    /// \endcode
    ///
    /// \note This method is not yet available in python, pending some
    /// refactoring to make it more feasible.
    ///
    /// See \ref Usd_PrimFlags "Prim predicate flags" and 
    /// #UsdPrimDefaultPredicate for more information, UsdStage::Traverse(), 
    /// and \c UsdPrimRange for more general Stage traversal behaviors.
    inline SubtreeRange GetDescendants() const;

    /// Return all this prim's descendants as an iterable range.
    ///
    /// \note This method is not yet available in python, pending some
    /// refactoring to make it more feasible.
    ///
    /// See \ref Usd_PrimFlags "Prim predicate flags" and 
    /// #UsdPrimDefaultPredicate for more information, UsdStage::Traverse(), 
    /// and \c UsdPrimRange for more general Stage traversal behaviors.
    inline SubtreeRange GetAllDescendants() const;

    /// Return a subset of all of this prim's descendants filtered by
    /// \p predicate as an iterable range.  The \p predicate is generated by
    /// combining a series of prim flag terms with either && or || and !.
    ///
    /// Example usage:
    /// \code
    /// // Get all active model descendants.
    /// GetFilteredDescendants(UsdPrimIsActive && UsdPrimIsModel);
    /// 
    /// // Get all model descendants that pass the default predicate.
    /// GetFilteredDescendants(UsdPrimDefaultPredicate && UsdPrimIsModel);
    /// \endcode
    ///
    /// If this prim is an instance, no descendants will be returned unless
    /// #UsdTraverseInstanceProxies is used to allow instance proxies to be
    /// returned, or if this prim is itself an instance proxy.
    ///
    /// \note This method is not yet available in python, pending some
    /// refactoring to make it more feasible.
    ///
    /// See \ref Usd_PrimFlags "Prim predicate flags" and 
    /// #UsdPrimDefaultPredicate for more information, UsdStage::Traverse(), 
    /// and \c UsdPrimRange for more general Stage traversal behaviors.
    inline SubtreeRange
    GetFilteredDescendants(const Usd_PrimFlagsPredicate &predicate) const;

public:
    // --------------------------------------------------------------------- //
    /// \name Parent & Stage
    // --------------------------------------------------------------------- //

    /// Return this prim's parent prim.  Return an invalid UsdPrim if this is a
    /// root prim.
    UsdPrim GetParent() const {
        Usd_PrimDataConstPtr prim = get_pointer(_Prim());
        SdfPath proxyPrimPath = _ProxyPrimPath();
        Usd_MoveToParent(prim, proxyPrimPath);
        return UsdPrim(prim, proxyPrimPath);
    }

    /// Return this prim's next active, loaded, defined, non-abstract sibling 
    /// if it has one, otherwise return an invalid UsdPrim.  Equivalent to:
    /// \code
    /// GetFilteredNextSibling(UsdPrimDefaultPredicate)
    /// \endcode
    ///
    /// See \ref Usd_PrimFlags "Prim predicate flags" 
    /// and #UsdPrimDefaultPredicate for more information.
    USD_API
    UsdPrim GetNextSibling() const;

    /// Return this prim's next sibling that matches \p predicate if it has one,
    /// otherwise return the invalid UsdPrim.
    ///
    /// See \ref Usd_PrimFlags "Prim predicate flags" 
    /// and #UsdPrimDefaultPredicate for more information.
    USD_API
    UsdPrim GetFilteredNextSibling(
        const Usd_PrimFlagsPredicate &predicate) const;
        
    /// Returns true if the prim is the pseudo root.  
    ///
    /// Equivalent to 
    /// \code
    /// prim.GetPath() == SdfPath::AbsoluteRootPath()
    /// \endcode
    USD_API
    bool IsPseudoRoot() const;

    // --------------------------------------------------------------------- //
    /// \name Variants 
    // --------------------------------------------------------------------- //

    /// Return a UsdVariantSets object representing all the VariantSets
    /// present on this prim.
    ///
    /// The returned object also provides the API for adding new VariantSets
    /// to the prim.
    USD_API
    UsdVariantSets GetVariantSets() const;

    /// Retrieve a specifically named VariantSet for editing or constructing
    /// a UsdEditTarget.
    ///
    /// This is a shortcut for 
    /// \code
    /// prim.GetVariantSets().GetVariantSet(variantSetName)
    /// \endcode
    USD_API
    UsdVariantSet GetVariantSet(const std::string& variantSetName) const;

    /// Return true if this prim has any authored VariantSets.
    ///
    /// \note this connotes only the *existence* of one of more VariantSets,
    /// *not* that such VariantSets necessarily contain any variants or
    /// variant opinions.
    USD_API
    bool HasVariantSets() const;

    // --------------------------------------------------------------------- //
    /// \name Attributes 
    // --------------------------------------------------------------------- //

    /// Author scene description for the attribute named \a attrName at the
    /// current EditTarget if none already exists.  Return a valid attribute if
    /// scene description was successfully authored or if it already existed,
    /// return invalid attribute otherwise.  Note that the supplied \a typeName
    /// and \a custom arguments are only used in one specific case.  See below
    /// for details.
    ///
    /// Suggested use:
    /// \code
    /// if (UsdAttribute myAttr = prim.CreateAttribute(...)) {
    ///    // success. 
    /// }
    /// \endcode
    ///
    /// To call this, GetPrim() must return a valid prim.
    ///
    /// - If a spec for this attribute already exists at the current edit
    /// target, do nothing.
    ///
    /// - If a spec for \a attrName of a different spec type (e.g. a
    /// relationship) exists at the current EditTarget, issue an error.
    ///
    /// - If \a name refers to a builtin attribute according to the prim's
    /// definition, author an attribute spec with required metadata from the
    /// definition.
    ///
    /// - If \a name refers to a builtin relationship, issue an error.
    ///
    /// - If there exists an absolute strongest authored attribute spec for
    /// \a attrName, author an attribute spec at the current EditTarget by
    /// copying required metadata from that strongest spec.
    ///
    /// - If there exists an absolute strongest authored relationship spec for
    /// \a attrName, issue an error.
    ///
    /// - Otherwise author an attribute spec at the current EditTarget using
    /// the provided \a typeName and \a custom for the required metadata fields.
    /// Note that these supplied arguments are only ever used in this particular
    /// circumstance, in all other cases they are ignored.
    USD_API
    UsdAttribute
    CreateAttribute(const TfToken& name,
                    const SdfValueTypeName &typeName,
                    bool custom,
                    SdfVariability variability = SdfVariabilityVarying) const;
    /// \overload
    /// Create a custom attribute with \p name, \p typeName and \p variability.
    USD_API
    UsdAttribute
    CreateAttribute(const TfToken& name,
                    const SdfValueTypeName &typeName,
                    SdfVariability variability = SdfVariabilityVarying) const;

    /// \overload
    /// This overload of CreateAttribute() accepts a vector of name components
    /// used to construct a \em namespaced property name.  For details, see
    /// \ref Usd_Ordering
    USD_API
    UsdAttribute CreateAttribute(
        const std::vector<std::string> &nameElts,
        const SdfValueTypeName &typeName,
        bool custom,
        SdfVariability variability = SdfVariabilityVarying) const;
    /// \overload
    /// Create a custom attribute with \p nameElts, \p typeName, and
    /// \p variability.
    USD_API
    UsdAttribute CreateAttribute(
        const std::vector<std::string> &nameElts,
        const SdfValueTypeName &typeName,
        SdfVariability variability = SdfVariabilityVarying) const;

    /// Like GetProperties(), but exclude all relationships from the result.
    USD_API
    std::vector<UsdAttribute> GetAttributes() const;

    /// Like GetAttributes(), but exclude attributes without authored scene
    /// description from the result.  See UsdProperty::IsAuthored().
    USD_API
    std::vector<UsdAttribute> GetAuthoredAttributes() const;

    /// Return a UsdAttribute with the name \a attrName. The attribute 
    /// returned may or may not \b actually exist so it must be checked for
    /// validity. Suggested use:
    ///
    /// \code
    /// if (UsdAttribute myAttr = prim.GetAttribute("myAttr")) {
    ///    // myAttr is safe to use. 
    ///    // Edits to the owning stage requires subsequent validation.
    /// } else {
    ///    // myAttr was not defined/authored
    /// }
    /// \endcode
    USD_API
    UsdAttribute GetAttribute(const TfToken& attrName) const;

    /// Return true if this prim has an attribute named \p attrName, false
    /// otherwise.
    USD_API
    bool HasAttribute(const TfToken& attrName) const;

    /// Search the prim subtree rooted at this prim for attributes for which
    /// \p predicate returns true, collect their connection source paths and
    /// return them in an arbitrary order.  If \p recurseOnSources is true,
    /// act as if this function was invoked on the connected prims and owning
    /// prims of connected properties also and return the union.
    USD_API
    SdfPathVector
    FindAllAttributeConnectionPaths(
        std::function<bool (UsdAttribute const &)> const &pred = nullptr,
        bool recurseOnSources = false) const;

    // --------------------------------------------------------------------- //
    /// \name Relationships
    // --------------------------------------------------------------------- //

    /// Author scene description for the relationship named \a relName at the
    /// current EditTarget if none already exists.  Return a valid relationship
    /// if scene description was successfully authored or if it already existed,
    /// return an invalid relationship otherwise.
    ///
    /// Suggested use:
    /// \code
    /// if (UsdRelationship myRel = prim.CreateRelationship(...)) {
    ///    // success. 
    /// }
    /// \endcode
    ///
    /// To call this, GetPrim() must return a valid prim.
    ///
    /// - If a spec for this relationship already exists at the current edit
    /// target, do nothing.
    ///
    /// - If a spec for \a relName of a different spec type (e.g. an
    /// attribute) exists at the current EditTarget, issue an error.
    ///
    /// - If \a name refers to a builtin relationship according to the prim's
    /// definition, author a relationship spec with required metadata from the
    /// definition.
    ///
    /// - If \a name refers to a builtin attribute, issue an error.
    ///
    /// - If there exists an absolute strongest authored relationship spec for
    /// \a relName, author a relationship spec at the current EditTarget by
    /// copying required metadata from that strongest spec.
    ///
    /// - If there exists an absolute strongest authored attribute spec for \a
    /// relName, issue an error.
    ///
    /// - Otherwise author a uniform relationship spec at the current
    /// EditTarget, honoring \p custom .
    ///
    USD_API
    UsdRelationship CreateRelationship(const TfToken& relName,
                                       bool custom=true) const;

    /// \overload 
    /// This overload of CreateRelationship() accepts a vector of
    /// name components used to construct a \em namespaced property name.
    /// For details, see \ref Usd_Ordering
    USD_API
    UsdRelationship CreateRelationship(const std::vector<std::string> &nameElts,
                                       bool custom=true)
        const;

    /// Like GetProperties(), but exclude all attributes from the result.
    USD_API
    std::vector<UsdRelationship> GetRelationships() const;

    /// Like GetRelationships(), but exclude relationships without authored
    /// scene description from the result.  See UsdProperty::IsAuthored().
    USD_API
    std::vector<UsdRelationship> GetAuthoredRelationships() const;

    /// Return a UsdRelationship with the name \a relName. The relationship
    /// returned may or may not \b actually exist so it must be checked for
    /// validity. Suggested use:
    ///
    /// \code
    /// if (UsdRelationship myRel = prim.GetRelationship("myRel")) {
    ///    // myRel is safe to use.
    ///    // Edits to the owning stage requires subsequent validation.
    /// } else {
    ///    // myRel was not defined/authored
    /// }
    /// \endcode
    USD_API
    UsdRelationship GetRelationship(const TfToken& relName) const;

    /// Return true if this prim has a relationship named \p relName, false
    /// otherwise.
    USD_API
    bool HasRelationship(const TfToken& relName) const;

    /// Search the prim subtree rooted at this prim for relationships for which
    /// \p predicate returns true, collect their target paths and return them in
    /// an arbitrary order.  If \p recurseOnTargets is true, act as if this
    /// function was invoked on the targeted prims and owning prims of targeted
    /// properties also (but not of forwarding relationships) and return the
    /// union.
    USD_API
    SdfPathVector
    FindAllRelationshipTargetPaths(
        std::function<bool (UsdRelationship const &)> const &pred = nullptr,
        bool recurseOnTargets = false) const;

    // --------------------------------------------------------------------- //
    /// \name Payloads, Load and Unload 
    // --------------------------------------------------------------------- //

    /// Clears the payload at the current EditTarget for this prim. 
    /// Return false if the payload could not be cleared.
    USD_API
    bool ClearPayload() const;

    /// Return true if a payload is present on this prim.
    ///
    /// \sa \ref Usd_Payloads
    USD_API
    bool HasPayload() const;

    /// Author payload metadata for this prim at the current edit
    /// target. Return true on success, false if the value could not be set. 
    ///
    /// \sa \ref Usd_Payloads
    USD_API
    bool SetPayload(const SdfPayload& payload) const;

    /// Shorthand for SetPayload(SdfPayload(assetPath, primPath)).
    USD_API
    bool SetPayload(
        const std::string& assetPath, const SdfPath& primPath) const;
    
    /// Shorthand for SetPayload(SdfPayload(layer->GetIdentifier(),
    /// primPath)).
    USD_API
    bool SetPayload(const SdfLayerHandle& layer, const SdfPath& primPath) const;

    /// Load this prim, all its ancestors, and by default all its descendants.
    /// If \p loadPolicy is UsdLoadWithoutDescendants, then load only this prim
    /// and its ancestors.
    ///
    /// See UsdStage::Load for additional details.
    USD_API
    void Load(UsdLoadPolicy policy = UsdLoadWithDescendants) const;

    /// Unloads this prim and all its descendants.
    ///
    /// See UsdStage::Unload for additional details.
    USD_API
    void Unload() const;

    // --------------------------------------------------------------------- //
    /// \name References 
    // --------------------------------------------------------------------- //

    /// Return a UsdReferences object that allows one to add, remove, or
    /// mutate references <em>at the currently set UsdEditTarget</em>.
    ///
    /// There is currently no facility for \em listing the currently authored
    /// references on a prim... the problem is somewhat ill-defined, and
    /// requires some thought.
    USD_API
    UsdReferences GetReferences() const;

    /// Return true if this prim has any authored references.
    USD_API
    bool HasAuthoredReferences() const;

    // --------------------------------------------------------------------- //
    /// \name Inherits 
    // --------------------------------------------------------------------- //
    
    /// Return a UsdInherits object that allows one to add, remove, or
    /// mutate inherits <em>at the currently set UsdEditTarget</em>.
    ///
    /// There is currently no facility for \em listing the currently authored
    /// inherits on a prim... the problem is somewhat ill-defined, and
    /// requires some thought.
    USD_API
    UsdInherits GetInherits() const;

    /// Return true if this prim has any authored inherits.
    USD_API
    bool HasAuthoredInherits() const;

    // --------------------------------------------------------------------- //
    /// \name Specializes 
    // --------------------------------------------------------------------- //
    
    /// Return a UsdSpecializes object that allows one to add, remove, or
    /// mutate specializes <em>at the currently set UsdEditTarget</em>.
    ///
    /// There is currently no facility for \em listing the currently authored
    /// specializes on a prim... the problem is somewhat ill-defined, and
    /// requires some thought.
    USD_API
    UsdSpecializes GetSpecializes() const;

    /// Returns true if this prim has any authored specializes.
    USD_API
    bool HasAuthoredSpecializes() const;

    // --------------------------------------------------------------------- //
    /// \name Instancing
    /// See \ref Usd_Page_ScenegraphInstancing for more details.
    /// @{
    // --------------------------------------------------------------------- //

    /// Return true if this prim has been marked as instanceable.
    ///
    /// Note that this is not the same as IsInstance(). A prim may return
    /// true for IsInstanceable() and false for IsInstance() if this prim
    /// is not active or if it is marked as instanceable but contains no 
    /// instanceable data.
    bool IsInstanceable() const { 
        bool instanceable = false;
        return GetMetadata(SdfFieldKeys->Instanceable, &instanceable) &&
            instanceable;
    }

    /// Author 'instanceable' metadata for this prim at the current
    /// EditTarget.
    bool SetInstanceable(bool instanceable) const {
        return SetMetadata(SdfFieldKeys->Instanceable, instanceable);
    }

    /// Remove the authored 'instanceable' opinion at the current EditTarget.
    /// Do nothing if there is no authored opinion.
    bool ClearInstanceable() const {
        return ClearMetadata(SdfFieldKeys->Instanceable);
    }

    /// Return true if this prim has an authored opinion for 'instanceable', 
    /// false otherwise.
    bool HasAuthoredInstanceable() const {
        return HasAuthoredMetadata(SdfFieldKeys->Instanceable);
    }

    /// Return true if this prim is an instance of a master, false
    /// otherwise.
    ///
    /// If this prim is an instance, calling GetMaster() will return
    /// the UsdPrim for the corresponding master prim.
    bool IsInstance() const { return _Prim()->IsInstance(); }

    /// Return true if this prim is an instance proxy, false otherwise.
    /// An instance proxy prim represents a descendent of an instance
    /// prim.
    bool IsInstanceProxy() const { 
        return Usd_IsInstanceProxy(_Prim(), _ProxyPrimPath());
    }

    /// Return true if this prim is a master prim, false otherwise.
    bool IsMaster() const { return _Prim()->IsMaster(); }

    /// Return true if this prim is located in a subtree of prims
    /// rooted at a master prim, false otherwise.
    ///
    /// If this function returns true, this prim is either a master prim
    /// or a descendent of a master prim.
    bool IsInMaster() const { 
        return (IsInstanceProxy() ? 
            _PrimPathIsInMaster() : _Prim()->IsInMaster());
    }

    /// If this prim is an instance, return the UsdPrim for the corresponding
    /// master. Otherwise, return an invalid UsdPrim.
    USD_API
    UsdPrim GetMaster() const;

    /// If this prim is an instance proxy, return the UsdPrim for the
    /// corresponding prim in the instance's master. Otherwise, return an
    /// invalid UsdPrim.
    UsdPrim GetPrimInMaster() const {
        if (IsInstanceProxy()) {
            return UsdPrim(_Prim(), SdfPath());
        }
        return UsdPrim();
    }

    /// @}

    // --------------------------------------------------------------------- //
    /// \name Composition Structure
    /// @{
    // --------------------------------------------------------------------- //

    /// Return the cached prim index containing all sites that contribute 
    /// opinions to this prim.
    ///
    /// The prim index can be used to examine the composition arcs and scene 
    /// description sites that contribute to this prim's property and metadata 
    /// values. 
    ///
    /// The prim index returned by this function is optimized and may not
    /// include sites that do not contribute opinions to this prim. Use 
    /// UsdPrim::ComputeExpandedPrimIndex to compute a prim index that includes 
    /// all possible sites that could contribute opinions.
    ///
    /// This prim index will be empty for master prims. This ensures that these 
    /// prims do not provide any attribute or metadata values. For all other 
    /// prims in masters, this is the prim index that was chosen to be shared 
    /// with all other instances. In either case, the prim index's path will 
    /// not be the same as the prim's path.
    ///
    /// Prim indexes may be invalidated by changes to the UsdStage and cannot
    /// detect if they are expired. Clients should avoid keeping copies of the 
    /// prim index across such changes, which include scene description
    /// changes or changes to load state.
    const PcpPrimIndex &GetPrimIndex() const { return _Prim()->GetPrimIndex(); }

    /// Compute the prim index containing all sites that could contribute
    /// opinions to this prim.
    ///
    /// This function is similar to UsdPrim::GetPrimIndex. However, the
    /// returned prim index includes all sites that could possibly contribute 
    /// opinions to this prim, not just the sites that currently do so. This is 
    /// useful in certain situations; for example, this could be used to 
    /// generate a list of sites where clients could make edits to affect this 
    /// prim, or for debugging purposes.
    ///
    /// This function may be relatively slow, since it will recompute the prim
    /// index on every call. Clients should prefer UsdPrim::GetPrimIndex unless 
    /// the additional site information is truly needed.
    USD_API
    PcpPrimIndex ComputeExpandedPrimIndex() const;

    /// @}

private:
    friend class UsdObject;
    friend class UsdPrimSiblingIterator;
    friend class UsdPrimSubtreeIterator;
    friend class UsdProperty;
    friend class UsdSchemaBase;
    friend class UsdAPISchemaBase;
    friend class UsdStage;
    friend class UsdPrimRange;
    friend class Usd_PrimData;
    friend class Usd_PrimFlagsPredicate;
    friend struct UsdPrim_RelTargetFinder;
    friend struct UsdPrim_AttrConnectionFinder;

    // Prim constructor.
    UsdPrim(const Usd_PrimDataHandle &primData,
            const SdfPath &proxyPrimPath) 
        : UsdObject(primData, proxyPrimPath) { }

    // General constructor.
    UsdPrim(UsdObjType objType,
            const Usd_PrimDataHandle &prim, 
            const SdfPath &proxyPrimPath,
            const TfToken &propName)
        : UsdObject(objType, prim, proxyPrimPath, propName) {}

    // Helper to make a sibling range.
    inline SiblingRange
    _MakeSiblingRange(const Usd_PrimFlagsPredicate &pred) const;

    // Helper to make a range of descendants.
    inline SubtreeRange
    _MakeDescendantsRange(const Usd_PrimFlagsPredicate &pred) const;

    // Helper to make a vector of properties from names.
    std::vector<UsdProperty>
    _MakeProperties(const TfTokenVector &names) const;

    // Helper for Get{Authored}{PropertyNames,Properties} 
    TfTokenVector _GetPropertyNames(
        bool onlyAuthored,
        bool applyOrder=true,
        const PropertyPredicateFunc &predicate={}) const;

    // Helper for Get(Authored)PropertiesInNamespace.
    std::vector<UsdProperty>
    _GetPropertiesInNamespace(const std::string &namespaces,
                              bool onlyAuthored) const;

    // Helper for Get(Authored)Attributes.
    std::vector<UsdAttribute>
    _GetAttributes(bool onlyAuthored, bool applyOrder=false) const;

    // Helper for Get(Authored)Relationships.
    std::vector<UsdRelationship>
    _GetRelationships(bool onlyAuthored, bool applyOrder=false) const;

    // Helper for determining whether this prim is in a master based
    // on prim path.
    USD_API
    bool _PrimPathIsInMaster() const;

    friend const PcpPrimIndex &Usd_PrimGetSourcePrimIndex(const UsdPrim&);
    // Return a const reference to the source PcpPrimIndex for this prim.
    //
    // For all prims in masters (which includes the master prim itself), 
    // this is the prim index for the instance that was chosen to serve
    // as the master for all other instances.  This prim index will not
    // have the same path as the prim's path.
    //
    // This is a private helper but is also wrapped out to Python
    // for testing and debugging purposes.
    const PcpPrimIndex &_GetSourcePrimIndex() const
    { return _Prim()->GetSourcePrimIndex(); }
};

#ifdef doxygen

/// Forward traversal iterator of sibling ::UsdPrim s.  This is a
/// standard-compliant iterator that may be used with STL algorithms, etc.
class UsdPrimSiblingIterator {
public:
    /// Iterator value type.
    typedef UsdPrim value_type;
    /// Iterator reference type, in this case the same as \a value_type.
    typedef value_type reference;
    /// Iterator difference type.
    typedef unspecified-integral-type difference_type;
    /// Dereference.
    reference operator*() const;
    /// Indirection.
    unspecified-type operator->() const;
    /// Postincrement.
    UsdPrimSiblingIterator &operator++();
    /// Preincrement.
    UsdPrimSiblingIterator operator++(int);
private:
    /// Equality.
    friend bool operator==(const UsdPrimSiblingIterator &lhs,
                           const UsdPrimSiblingIterator &rhs);
    /// Inequality.
    friend bool operator!=(const UsdPrimSiblingIterator &lhs,
                           const UsdPrimSiblingIterator &rhs);
};

/// Forward iterator range of sibling ::UsdPrim s.  This range type contains a
/// pair of UsdPrimSiblingIterator s, denoting a half-open range of UsdPrim
/// siblings.  It provides a subset of container-like API, such as begin(),
/// end(), front(), empty(), etc.
class UsdPrimSiblingRange {
public:
    /// Iterator type.
    typedef UsdPrimSiblingIterator iterator;
    /// Const iterator type.
    typedef UsdPrimSiblingIterator const_iterator;
    /// Iterator difference type.
    typedef unspecified-integral-type difference_type;
    /// Iterator value_type.
    typedef iterator::value_type value_type;
    /// Iterator reference_type.
    typedef iterator::reference reference;

    /// Construct with a pair of iterators.
    UsdPrimSiblingRange(UsdPrimSiblingIterator begin,
                        UsdPrimSiblingIterator end);

    /// Construct/convert from another compatible range type.
    template <class ForwardRange>
    UsdPrimSiblingRange(const ForwardRange &r);

    /// Assign from another compatible range type.
    template <class ForwardRange>
    UsdPrimSiblingRange &operator=(const ForwardRange &r);

    /// First iterator.
    iterator begin() const;

    /// Past-the-end iterator.
    iterator end() const;

    /// Return !empty().
    operator unspecified_bool_type() const;

    /// Equality compare.
    bool equal(const iterator_range&) const;

    /// Return *begin().  This range must not be empty.
    reference front() const;

    /// Advance this range's begin iterator.
    iterator_range& advance_begin(difference_type n);

    /// Advance this range's end iterator.
    iterator_range& advance_end(difference_type n);

    ;    /// Return begin() == end().
    bool empty() const;

private:
    /// Equality comparison.
    friend bool operator==(const UsdPrimSiblingRange &lhs,
                           const UsdPrimSiblingRange &rhs);
    /// Inequality comparison.
    friend bool operator!=(const UsdPrimSiblingRange &lhs,
                           const UsdPrimSiblingRange &rhs);
};

#else

// Sibling iterator class.  Converts ref to weak and filters according to a
// supplied predicate.
class UsdPrimSiblingIterator : public boost::iterator_adaptor<
    UsdPrimSiblingIterator,                       // crtp base.
    const Usd_PrimData *,                         // base iterator.
    UsdPrim,                                      // value type.
    boost::forward_traversal_tag,                 // traversal
    UsdPrim>                                      // reference type.
{
public:
    // Default ctor.
    UsdPrimSiblingIterator() {}

private:
    friend class UsdPrim;

    // Constructor used by Prim.
    UsdPrimSiblingIterator(const base_type &i, const SdfPath& proxyPrimPath,
                           const Usd_PrimFlagsPredicate &predicate)
        : iterator_adaptor_(i)
        , _proxyPrimPath(proxyPrimPath)
        , _predicate(predicate) {
        // Need to advance iterator to first matching element.
        if (base() && !Usd_EvalPredicate(_predicate, base(), _proxyPrimPath))
            increment();
    }

    // Core implementation invoked by iterator_adaptor.
    friend class boost::iterator_core_access;
    bool equal(const UsdPrimSiblingIterator &other) const {
        return base() == other.base() && 
            _proxyPrimPath == other._proxyPrimPath &&
            _predicate == other._predicate;
    }

    void increment() {
        base_type &base = base_reference();
        if (Usd_MoveToNextSiblingOrParent(base, _proxyPrimPath, _predicate)) {
            base = nullptr;
            _proxyPrimPath = SdfPath();
        }
    }

    reference dereference() const {
        return UsdPrim(base(), _proxyPrimPath);
    }

    SdfPath _proxyPrimPath;
    Usd_PrimFlagsPredicate _predicate;
};

// Typedef iterator range.
typedef boost::iterator_range<UsdPrimSiblingIterator> UsdPrimSiblingRange;

// Inform TfIterator it should feel free to make copies of the range type.
template <>
struct Tf_ShouldIterateOverCopy<
    UsdPrimSiblingRange> : boost::true_type {};
template <>
struct Tf_ShouldIterateOverCopy<
    const UsdPrimSiblingRange> : boost::true_type {};

#endif // doxygen


UsdPrimSiblingRange
UsdPrim::GetFilteredChildren(const Usd_PrimFlagsPredicate &pred) const
{
    return _MakeSiblingRange(
        Usd_CreatePredicateForTraversal(_Prim(), _ProxyPrimPath(), pred));
}

UsdPrimSiblingRange
UsdPrim::GetAllChildren() const
{
    return GetFilteredChildren(UsdPrimAllPrimsPredicate);
}

UsdPrimSiblingRange
UsdPrim::GetChildren() const
{
    return GetFilteredChildren(UsdPrimDefaultPredicate);
}

// Helper to make a sibling range.
UsdPrim::SiblingRange
UsdPrim::_MakeSiblingRange(const Usd_PrimFlagsPredicate &pred) const {
    Usd_PrimDataConstPtr firstChild = get_pointer(_Prim());
    SdfPath firstChildPath = _ProxyPrimPath();
    if (!Usd_MoveToChild(firstChild, firstChildPath, pred)) {
        firstChild = nullptr;
        firstChildPath = SdfPath();
    }

    return SiblingRange(
        SiblingIterator(firstChild, firstChildPath, pred),
        SiblingIterator(nullptr, SdfPath(), pred));
}

#ifdef doxygen

/// Forward traversal iterator of sibling ::UsdPrim s.  This is a
/// standard-compliant iterator that may be used with STL algorithms, etc.
class UsdPrimSubtreeIterator {
public:
    /// Iterator value type.
    typedef UsdPrim value_type;
    /// Iterator reference type, in this case the same as \a value_type.
    typedef value_type reference;
    /// Iterator difference type.
    typedef unspecified-integral-type difference_type;
    /// Dereference.
    reference operator*() const;
    /// Indirection.
    unspecified-type operator->() const;
    /// Postincrement.
    UsdPrimSubtreeIterator &operator++();
    /// Preincrement.
    UsdPrimSubtreeIterator operator++(int);
private:
    /// Equality.
    friend bool operator==(const UsdPrimSubtreeIterator &lhs,
                           const UsdPrimSubtreeIterator &rhs);
    /// Inequality.
    friend bool operator!=(const UsdPrimSubtreeIterator &lhs,
                           const UsdPrimSubtreeIterator &rhs);
};

/// Forward iterator range of sibling ::UsdPrim s.  This range type contains a
/// pair of UsdPrimSubtreeIterator s, denoting a half-open range of UsdPrim
/// siblings.  It provides a subset of container-like API, such as begin(),
/// end(), front(), empty(), etc.
class UsdPrimSubtreeRange {
public:
    /// Iterator type.
    typedef UsdPrimSubtreeIterator iterator;
    /// Const iterator type.
    typedef UsdPrimSubtreeIterator const_iterator;
    /// Iterator difference type.
    typedef unspecified-integral-type difference_type;
    /// Iterator value_type.
    typedef iterator::value_type value_type;
    /// Iterator reference_type.
    typedef iterator::reference reference;

    /// Construct with a pair of iterators.
    UsdPrimSubtreeRange(UsdPrimSubtreeIterator begin,
                        UsdPrimSubtreeIterator end);

    /// Construct/convert from another compatible range type.
    template <class ForwardRange>
    UsdPrimSubtreeRange(const ForwardRange &r);

    /// Assign from another compatible range type.
    template <class ForwardRange>
    UsdPrimSubtreeRange &operator=(const ForwardRange &r);

    /// First iterator.
    iterator begin() const;

    /// Past-the-end iterator.
    iterator end() const;

    /// Return !empty().
    operator unspecified_bool_type() const;

    /// Equality compare.
    bool equal(const iterator_range&) const;

    /// Return *begin().  This range must not be empty.
    reference front() const;

    /// Advance this range's begin iterator.
    iterator_range& advance_begin(difference_type n);

    /// Advance this range's end iterator.
    iterator_range& advance_end(difference_type n);

    /// Return begin() == end().
    bool empty() const;

private:
    /// Equality comparison.
    friend bool operator==(const UsdPrimSubtreeRange &lhs,
                           const UsdPrimSubtreeRange &rhs);
    /// Inequality comparison.
    friend bool operator!=(const UsdPrimSubtreeRange &lhs,
                           const UsdPrimSubtreeRange &rhs);
};

#else

// Subtree iterator class.  Converts ref to weak and filters according to a
// supplied predicate.
class UsdPrimSubtreeIterator : public boost::iterator_adaptor<
    UsdPrimSubtreeIterator,                       // crtp base.
    const Usd_PrimData *,                         // base iterator.
    UsdPrim,                                      // value type.
    boost::forward_traversal_tag,                 // traversal
    UsdPrim>                                      // reference type.
{
public:
    // Default ctor.
    UsdPrimSubtreeIterator() {}

private:
    friend class UsdPrim;

    // Constructor used by Prim.
    UsdPrimSubtreeIterator(const base_type &i, const SdfPath &proxyPrimPath,
                           const Usd_PrimFlagsPredicate &predicate)
        : iterator_adaptor_(i)
        , _proxyPrimPath(proxyPrimPath)
        , _predicate(predicate) {
        // Need to advance iterator to first matching element.
        base_type &base = base_reference();
        if (base && !Usd_EvalPredicate(_predicate, base, _proxyPrimPath)) {
            if (Usd_MoveToNextSiblingOrParent(base, _proxyPrimPath, 
                                              _predicate)) {
                base = nullptr;
                _proxyPrimPath = SdfPath();
            }
        }
    }

    // Core implementation invoked by iterator_adaptor.
    friend class boost::iterator_core_access;
    bool equal(const UsdPrimSubtreeIterator &other) const {
        return base() == other.base() && 
            _proxyPrimPath == other._proxyPrimPath &&
            _predicate == other._predicate;
    }

    void increment() {
        base_type &base = base_reference();
        if (!Usd_MoveToChild(base, _proxyPrimPath, _predicate)) {
            while (Usd_MoveToNextSiblingOrParent(base, _proxyPrimPath, 
                                                 _predicate)) {}
        }
    }

    reference dereference() const {
        return UsdPrim(base(), _proxyPrimPath);
    }

    SdfPath _proxyPrimPath;
    Usd_PrimFlagsPredicate _predicate;
};

// Typedef iterator range.
typedef boost::iterator_range<UsdPrimSubtreeIterator> UsdPrimSubtreeRange;

// Inform TfIterator it should feel free to make copies of the range type.
template <>
struct Tf_ShouldIterateOverCopy<
    UsdPrimSubtreeRange> : boost::true_type {};
template <>
struct Tf_ShouldIterateOverCopy<
    const UsdPrimSubtreeRange> : boost::true_type {};

#endif // doxygen

UsdPrimSubtreeRange
UsdPrim::GetFilteredDescendants(const Usd_PrimFlagsPredicate &pred) const
{
    return _MakeDescendantsRange(
        Usd_CreatePredicateForTraversal(_Prim(), _ProxyPrimPath(), pred));
}

UsdPrimSubtreeRange
UsdPrim::GetAllDescendants() const
{
    return GetFilteredDescendants(UsdPrimAllPrimsPredicate);
}

UsdPrimSubtreeRange
UsdPrim::GetDescendants() const
{
    return GetFilteredDescendants(UsdPrimDefaultPredicate);
}

// Helper to make a sibling range.
UsdPrim::SubtreeRange
UsdPrim::_MakeDescendantsRange(const Usd_PrimFlagsPredicate &pred) const {
    Usd_PrimDataConstPtr firstChild = get_pointer(_Prim());
    SdfPath firstChildPath = _ProxyPrimPath();
    Usd_PrimDataConstPtr endChild = firstChild;
    SdfPath endChildPath = firstChildPath;
    if (Usd_MoveToChild(firstChild, firstChildPath, pred)) {
        while (Usd_MoveToNextSiblingOrParent(endChild, endChildPath, pred)) {}
    }

    return SubtreeRange(
        SubtreeIterator(firstChild, firstChildPath, pred),
        SubtreeIterator(endChild, endChildPath, pred));
}


////////////////////////////////////////////////////////////////////////
// UsdObject methods that require UsdPrim be a complete type.

inline UsdPrim
UsdObject::GetPrim() const
{
    return UsdPrim(_prim, _proxyPrimPath);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PRIM_H

