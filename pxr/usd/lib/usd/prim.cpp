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
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usd/variantSets.h"

#include "pxr/usd/pcp/primIndex.h"
#include "pxr/base/plug/registry.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/ostreamMethods.h"

#include <algorithm>
#include <vector>

UsdPrim
UsdPrim::GetChild(const TfToken &name) const
{
    return GetStage()->GetPrimAtPath(GetPath().AppendChild(name));
}

SdfPrimSpecHandle
UsdPrim::GetPrimDefinition() const
{
    const TfToken &typeName = GetTypeName();
    SdfPrimSpecHandle definition;

    if (not typeName.IsEmpty()) {
        // Look up definition from prim type name.
        definition = UsdSchemaRegistry::GetPrimDefinition(typeName);

        if (not definition) {
            // Issue a diagnostic for unknown prim types.
            TF_WARN("No definition for prim type '%s', <%s>",
                    typeName.GetText(), GetPath().GetText());
        }
    }
    return definition;
}

bool
UsdPrim::_IsA(const TfType& schemaType) const
{
    // Check Schema TfType
    if (schemaType.IsUnknown()) {
        TF_CODING_ERROR("Unknown schema type (%s) is invalid for IsA query",
                        schemaType.GetTypeName().c_str());
        return false;
    }

    // Get Prim TfType
    const std::string &typeName = GetTypeName().GetString();

    return not typeName.empty() and
        PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(typeName).
        IsA(schemaType);
}

std::vector<UsdProperty>
UsdPrim::_MakeProperties(const TfTokenVector &names) const
{
    std::vector<UsdProperty> props;
    UsdStage *stage = _GetStage();
    props.reserve(names.size());
    for (const auto& propName : names) {
        SdfSpecType specType =
            stage->_GetDefiningSpecType(*this, propName);
        if (specType == SdfSpecTypeAttribute) {
            props.push_back(GetAttribute(propName));
        } else if (TF_VERIFY(specType == SdfSpecTypeRelationship)) {
            props.push_back(GetRelationship(propName));
        }
    }

    return props;
}

// Change the order of items in 'names' so that all the things in 'order' that
// are also in 'names' are at the beginning in the order that they appear in
// 'order', followed by any remaining items in 'names' in their existing order.
static void
_ApplyOrdering(const TfTokenVector &order, TfTokenVector *names)
{
    // If order is empty or names is empty, nothing to do.
    if (order.empty() or names->empty())
        return;

    // Perf note: this walks 'order' and linear searches 'names' to find each
    // element, for O(M*N) operations, where M and N are the lengths of 'order'
    // and 'names'.  We hope 1) that propertyOrder stmts are relatively rare and
    // 2) that property lists are relatively short.  If those assumptions fail,
    // this may need revisiting.  In some quick microbenchmarking, this linear
    // search seems to outperform binary search up to about 5000 names.  We
    // suspect this is because linear search does TfToken pointer comparisons,
    // while binary search has to dereference and do string comparisons.

    typedef TfTokenVector::iterator Iter;

    Iter namesRest = names->begin(), namesEnd = names->end();
    for (const TfToken &oName: order) {
        // Look for this name from 'order' in the rest of 'names'.
        Iter i = std::find(namesRest, namesEnd, oName);
        if (i != namesEnd) {
            // Found.  Move to the front by rotating the sub-range.  Using
            // std::rotate invokes swap(), which avoids TfToken refcounting.
            // Also advance 'namesRest' to the next element.
            std::rotate(namesRest++, i, i+1);
        }
    }
}

bool 
UsdPrim::RemoveProperty(const TfToken &propName) 
{
    SdfPath propPath = GetPath().AppendProperty(propName);
    return _GetStage()->_RemoveProperty(propPath);
}

UsdProperty
UsdPrim::GetProperty(const TfToken &propName) const
{
    return UsdProperty(UsdTypeProperty, _Prim(), propName);
}

bool
UsdPrim::HasProperty(const TfToken &propName) const 
{
    return GetProperty(propName);
}

TfTokenVector
UsdPrim::GetPropertyOrder() const
{
    TfTokenVector order;
    GetMetadata(SdfFieldKeys->PropertyOrder, &order);
    return order;
}

void
UsdPrim::SetPropertyOrder(const TfTokenVector &order) const
{
    SetMetadata(SdfFieldKeys->PropertyOrder, order);
}

TfTokenVector
UsdPrim::_GetPropertyNames(bool onlyAuthored, bool applyOrder) const
{
    TfTokenVector names;

    // If we're including unauthored properties, take names from definition, if
    // present.
    if (not onlyAuthored) {
        UsdSchemaRegistry::HasField(GetTypeName(), TfToken(),
                                    SdfChildrenKeys->PropertyChildren, &names);
    }

    // Add authored names, then sort and apply ordering.
    GetPrimIndex().ComputePrimPropertyNames(&names);
    if (applyOrder) {
        sort(names.begin(), names.end(), TfDictionaryLessThan());
        _ApplyOrdering(GetPropertyOrder(), &names);
    }
    return names;
}

TfTokenVector
UsdPrim::GetPropertyNames() const
{
    return _GetPropertyNames(/*onlyAuthored=*/ false);
}

TfTokenVector
UsdPrim::GetAuthoredPropertyNames() const
{
    return _GetPropertyNames(/*onlyAuthored=*/ true);
}

std::vector<UsdProperty>
UsdPrim::GetProperties() const
{
    return _MakeProperties(GetPropertyNames());
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredProperties() const
{
    return _MakeProperties(GetAuthoredPropertyNames());
}

std::vector<UsdProperty>
UsdPrim::_GetPropertiesInNamespace(const std::string &namespaces,
                                   bool onlyAuthored) const
{
    // XXX Would be nice to someday plumb the prefix search down through pcp
    if (namespaces.empty())
        return onlyAuthored ? GetAuthoredProperties() : GetProperties();

    const char delim = UsdObject::GetNamespaceDelimiter();

    TfTokenVector names = _GetPropertyNames(onlyAuthored, /*applyOrder=*/false);

    // Set terminator to the expected position of the delimiter after all the
    // supplied namespaces.  We perform an explicit test for this char below
    // so that we don't need to allocate a new string if namespaces does not
    // already end with the delimiter
    const size_t terminator = namespaces.size() -
        (*namespaces.rbegin() == delim);

    // Prune out non-matches before we sort
    size_t insertionPt = 0;
    for (const auto& name : names) {
        const std::string &s = name.GetString();
        if (s.size() > terminator and
            TfStringStartsWith(s, namespaces) and
            s[terminator] == delim) {

            names[insertionPt++] = name;
        }
    }
    names.resize(insertionPt);
    sort(names.begin(), names.end(), TfDictionaryLessThan());
    _ApplyOrdering(GetPropertyOrder(), &names);
    return _MakeProperties(names);
}

std::vector<UsdProperty>
UsdPrim::GetPropertiesInNamespace(const std::string &namespaces) const
{
    return _GetPropertiesInNamespace(namespaces, /*onlyAuthored=*/false);
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredPropertiesInNamespace(const std::string &namespaces) const
{
    return _GetPropertiesInNamespace(namespaces, /*onlyAuthored=*/true);
}

std::vector<UsdProperty>
UsdPrim::GetPropertiesInNamespace(const std::vector<std::string> &namespaces) const
{
    return GetPropertiesInNamespace(SdfPath::JoinIdentifier(namespaces));
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredPropertiesInNamespace(
    const std::vector<std::string> &namespaces) const
{
    return GetAuthoredPropertiesInNamespace(
        SdfPath::JoinIdentifier(namespaces));
}

UsdAttribute
UsdPrim::CreateAttribute(const TfToken& name,
                         const SdfValueTypeName &typeName,
                         bool custom,
                         SdfVariability variability) const
{
    UsdAttribute attr = GetAttribute(name);
    attr._Create(typeName, custom, variability);
    return attr;
}

UsdAttribute
UsdPrim::CreateAttribute(const TfToken& name,
                         const SdfValueTypeName &typeName,
                         SdfVariability variability) const
{
    return CreateAttribute(name, typeName, /*custom=*/true, variability);
}

UsdAttribute
UsdPrim::CreateAttribute(const std::vector<std::string> &nameElts,
                         const SdfValueTypeName &typeName,
                         bool custom,
                         SdfVariability variability) const
{
    return CreateAttribute(TfToken(SdfPath::JoinIdentifier(nameElts)),
                           typeName, custom, variability);
}

UsdAttribute
UsdPrim::CreateAttribute(const std::vector<std::string> &nameElts,
                         const SdfValueTypeName &typeName,
                         SdfVariability variability) const
{
    return CreateAttribute(nameElts, typeName, /*custom=*/true, variability);
}

UsdAttributeVector
UsdPrim::_GetAttributes(bool onlyAuthored) const
{
    const TfTokenVector names = _GetPropertyNames(onlyAuthored);
    UsdAttributeVector attrs;

    // PERFORMANCE: This is sloppy, since property names are a superset of
    // attribute names, however this vector is likely short lived and worth the
    // trade off of repeated reallocation.
    attrs.reserve(names.size());
    for (const auto& propName : names) {
        if (UsdAttribute attr = GetAttribute(propName)) {
            attrs.push_back(attr);
        }
    }

    return attrs;
}

UsdAttributeVector
UsdPrim::GetAttributes() const
{
    return _GetAttributes(/*onlyAuthored=*/false);
}

UsdAttributeVector
UsdPrim::GetAuthoredAttributes() const
{
    return _GetAttributes(/*onlyAuthored=*/true);
}

UsdAttribute
UsdPrim::GetAttribute(const TfToken& attrName) const
{
    // An invalid prim will present a coding error, and then return an
    // invalid attribute
    return UsdAttribute(_Prim(), attrName);
}

bool
UsdPrim::HasAttribute(const TfToken& attrName) const
{
    return GetAttribute(attrName);
}

UsdRelationship
UsdPrim::CreateRelationship(const TfToken& name, bool custom) const
{
    UsdRelationship rel = GetRelationship(name);
    rel._Create(custom);
    return rel;
}

UsdRelationship
UsdPrim::CreateRelationship(const std::vector<std::string> &nameElts, 
                            bool custom) const
{
    return CreateRelationship(TfToken(SdfPath::JoinIdentifier(nameElts)),
                              custom);
}

UsdRelationshipVector
UsdPrim::_GetRelationships(bool onlyAuthored) const
{
    const TfTokenVector names = _GetPropertyNames(onlyAuthored);
    UsdRelationshipVector rels;

    // PERFORMANCE: This is sloppy, since property names are a superset of
    // relationship names, however this vector is likely short lived and worth
    // the trade off of repeated reallocation.
    rels.reserve(names.size());
    for (const auto& propName : names) {
        if (UsdRelationship rel = GetRelationship(propName)) {
            rels.push_back(rel);
        }
    }

    return rels;
}

UsdRelationshipVector
UsdPrim::GetRelationships() const
{
    return _GetRelationships(/*onlyAuthored=*/false);
}

UsdRelationshipVector
UsdPrim::GetAuthoredRelationships() const
{
    return _GetRelationships(/*onlyAuthored=*/true);
}

UsdRelationship
UsdPrim::GetRelationship(const TfToken& relName) const
{
    return UsdRelationship(_Prim(), relName);
}

bool
UsdPrim::HasRelationship(const TfToken& relName) const
{
    return GetRelationship(relName);
}

bool
UsdPrim::HasVariantSets() const
{
    return HasMetadata(SdfFieldKeys->VariantSetNames);
}

UsdVariantSets
UsdPrim::GetVariantSets() const
{
    return UsdVariantSets(*this);
}

UsdVariantSet
UsdPrim::GetVariantSet(const std::string& variantSetName) const
{
    return UsdVariantSet(*this, variantSetName);
}


UsdInherits
UsdPrim::GetInherits() const
{
    return UsdInherits(*this);
}

bool
UsdPrim::HasAuthoredInherits() const
{
    return HasMetadata(SdfFieldKeys->InheritPaths);
}

UsdSpecializes
UsdPrim::GetSpecializes() const
{
    return UsdSpecializes(*this);
}

bool
UsdPrim::HasAuthoredSpecializes() const
{
    return HasMetadata(SdfFieldKeys->Specializes);
}

UsdReferences
UsdPrim::GetReferences() const
{
    return UsdReferences(*this);
}

bool
UsdPrim::HasAuthoredReferences() const
{
    return HasMetadata(SdfFieldKeys->References);
}

// --------------------------------------------------------------------- //
/// \name Payloads, Load and Unload
// --------------------------------------------------------------------- //

bool
UsdPrim::HasPayload() const
{
    return _Prim()->HasPayload();
}

bool
UsdPrim::GetPayload(SdfPayload* payload) const
{
    return GetMetadata(SdfFieldKeys->Payload, payload);
}

bool
UsdPrim::SetPayload(const SdfPayload& payload) const
{
    return SetMetadata(SdfFieldKeys->Payload, payload);
}

bool
UsdPrim::SetPayload(const std::string& assetPath, const SdfPath& primPath) const
{
    return SetMetadata(SdfFieldKeys->Payload, SdfPayload(assetPath, primPath));
}

bool
UsdPrim::SetPayload(const SdfLayerHandle& layer, const SdfPath& primPath) const
{
    return SetMetadata(SdfFieldKeys->Payload,
                       SdfPayload(layer->GetIdentifier(), primPath));
}

bool
UsdPrim::ClearPayload() const
{
    return ClearMetadata(SdfFieldKeys->Payload);
}

void
UsdPrim::Load() const
{
    _GetStage()->Load(GetPath());
}

void
UsdPrim::Unload() const
{
    _GetStage()->Unload(GetPath());
}

UsdPrim
UsdPrim::GetNextSibling() const
{
    return GetFilteredNextSibling(UsdPrimIsActive and UsdPrimIsDefined and
                                  UsdPrimIsLoaded and not UsdPrimIsAbstract);
}

UsdPrim
UsdPrim::GetFilteredNextSibling(const Usd_PrimFlagsPredicate &pred) const
{
    Usd_PrimDataPtr s = _Prim()->GetNextSibling();
    while (s and not pred(s))
        s = s->GetNextSibling();

    return UsdPrim(s);
}

UsdPrim
UsdPrim::GetMaster() const
{
    return UsdPrim(
        _GetStage()->_GetMasterForInstance(get_pointer(_Prim())));
}

SdfPrimSpecHandleVector 
UsdPrim::GetPrimStack() const
{
    SdfPrimSpecHandleVector primStack;

    for (Usd_Resolver resolver(&(_Prim()->GetPrimIndex())); 
                      resolver.IsValid(); resolver.NextLayer()) {

        auto primSpec = resolver.GetLayer()
            ->GetPrimAtPath(resolver.GetLocalPath());

        if (primSpec) { 
            primStack.push_back(primSpec); 
        }
    }

    return primStack;
}
