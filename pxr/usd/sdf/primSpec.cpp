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
/// \file PrimSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/accessorHelpers.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/listOpListEditor.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"

#include <ostream>
#include <utility>
#include <string>
#include <vector>

using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

SDF_DEFINE_SPEC(SdfSchema, SdfSpecTypePrim, SdfPrimSpec, SdfSpec);

// register types
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define< SdfPrimSpecHandleVector >().
        Alias( TfType::GetRoot(), "SdfPrimSpecHandleVector");
    TfType::Define< SdfVariantSetSpecHandleMap >().
        Alias( TfType::GetRoot(), "map<string, SdfVariantSetSpecHandle>");
}

////////////////////////////////////////////////////////////////////////

SdfPrimSpecHandle
SdfPrimSpec::New(const SdfLayerHandle& parentLayer,
                 const std::string& name, SdfSpecifier spec,
                 const std::string& typeName)
{
    TRACE_FUNCTION();

    return _New(parentLayer ? parentLayer->GetPseudoRoot() : TfNullPtr,
                TfToken(name), spec, TfToken(typeName));
}

SdfPrimSpecHandle
SdfPrimSpec::New(const SdfPrimSpecHandle& parentPrim,
                 const std::string& name, SdfSpecifier spec,
                 const std::string& typeName)
{
    TRACE_FUNCTION();

    return _New(parentPrim, TfToken(name), spec, TfToken(typeName));
}

SdfPrimSpecHandle
SdfPrimSpec::_New(const SdfPrimSpecHandle &parentPrim,
                  const TfToken &name, SdfSpecifier spec,
                  const TfToken &typeName)
{
    SdfPrimSpec *parentPrimPtr = get_pointer(parentPrim);

    if (!parentPrimPtr) {
        TF_CODING_ERROR("Cannot create prim '%s' because the parent prim is "
                        "NULL",
                        name.GetText());
        return TfNullPtr;
    }
    if (!SdfPrimSpec::IsValidName(name)) {
        TF_RUNTIME_ERROR("Cannot create prim '%s' because '%s' is not a valid "
                         "name", 
                         parentPrimPtr->GetPath().AppendChild(name).GetText(),
                         name.GetText());
        return TfNullPtr;
    }
    
    // Group all the edits in a single change block.
    SdfChangeBlock block;

    // Use the special "pass" token if the caller tried to
    // create a typeless def
    TfToken type = (typeName.IsEmpty() && spec == SdfSpecifierDef) 
                   ? SdfTokens->AnyTypeToken : typeName;

    SdfLayerHandle layer = parentPrimPtr->GetLayer();
    SdfPath childPath = parentPrimPtr->GetPath().AppendChild(name);

    // PrimSpecs are considered inert if their specifier is
    // "over" and the type is not specified.
    bool inert = (spec == SdfSpecifierOver) && type.IsEmpty();

    if (!Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::CreateSpec(
                layer, childPath, SdfSpecTypePrim, inert)) {
        return TfNullPtr;
    }

    layer->SetField(childPath, SdfFieldKeys->Specifier, spec);
    if (!type.IsEmpty()) {
        layer->SetField(childPath, SdfFieldKeys->TypeName, type);
    }
    
    return layer->GetPrimAtPath(childPath);
}

bool
SdfPrimSpec::_ValidateEdit(const TfToken& key) const
{
    if (_IsPseudoRoot()) {
        TF_CODING_ERROR("Cannot edit %s on a pseudo-root", key.GetText());
        return false;
    }
    else {
        return true;
    }
}

//
// Name
//

const std::string&
SdfPrimSpec::GetName() const
{
    return GetPath().GetName();
}

TfToken
SdfPrimSpec::GetNameToken() const
{
    return GetPath().GetNameToken();
}

bool
SdfPrimSpec::CanSetName(const std::string& newName, std::string* whyNot) const
{
    if (_IsPseudoRoot()) {
        if (whyNot) {
            *whyNot = "The pseudo-root cannot be renamed";
        }
        return false;
    }

    return Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::CanRename(
        *this, TfToken(newName)).IsAllowed(whyNot);
}

bool
SdfPrimSpec::SetName(const std::string& name, bool validate)
{
    SdfChangeBlock changeBlock;

    const TfToken newName(name);
    const TfToken oldName = GetNameToken();
    if (!Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::Rename(
            *this, newName)) {
        return false;
    }

    if (newName == oldName) {
        // Nothing to do; just early out.
        return true;
    }

    // Also update any references to this prim in the parent spec's
    // name children order. 
    const SdfPath parentPath = GetPath().GetParentPath();
    if (SdfPrimSpecHandle parentPrim = GetLayer()->GetPrimAtPath(parentPath)) {
        SdfNameChildrenOrderProxy ordering = parentPrim->GetNameChildrenOrder();
        if (!ordering.empty()) {
            // If an entry for newName already exists in the reorder list,
            // make sure we remove it first before attempting to fixup the 
            // oldName entry. This takes care of two issues:
            //
            //   1. Duplicate entries are not allowed in the reorder list. If
            //      we didn't remove the entry, we'd get an error.
            //   2. Renaming a prim should not affect its position in the 
            //      reorder list.
            ordering.Remove(newName);
            ordering.Replace(oldName, newName);
        }
    }

    return true;
}

bool
SdfPrimSpec::IsValidName(const std::string& name)
{
    return Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::IsValidName(name);
}

//
// Namespace hierarchy
//

bool
SdfPrimSpec::_IsPseudoRoot() const
{
    return GetSpecType() == SdfSpecTypePseudoRoot;
}

SdfPrimSpecHandle
SdfPrimSpec::GetNameRoot() const
{
    return GetLayer()->GetPseudoRoot();
}

SdfPrimSpecHandle
SdfPrimSpec::GetNameParent() const
{
    return GetPath().IsRootPrimPath() ? 
        SdfPrimSpecHandle() : 
        GetLayer()->GetPrimAtPath(GetPath().GetParentPath());
}

SdfPrimSpecHandle
SdfPrimSpec::GetRealNameParent() const
{
    return GetLayer()->GetPrimAtPath(GetPath().GetParentPath());
}

SdfPrimSpec::NameChildrenView
SdfPrimSpec::GetNameChildren() const
{
    return NameChildrenView(GetLayer(), GetPath(), 
                            SdfChildrenKeys->PrimChildren);
}

void
SdfPrimSpec::SetNameChildren(const SdfPrimSpecHandleVector& nameChildrenSpecs)
{
    Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::SetChildren( 
            GetLayer(), GetPath(), nameChildrenSpecs);
}

bool
SdfPrimSpec::InsertNameChild(const SdfPrimSpecHandle& child, int index)
{
    return Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::InsertChild( 
            GetLayer(), GetPath(),  child, index);
}

bool
SdfPrimSpec::RemoveNameChild(const SdfPrimSpecHandle& child)
{
    if (child->GetLayer() != GetLayer() || 
        child->GetPath().GetParentPath() != GetPath()) { 
        TF_CODING_ERROR("Cannot remove child prim '%s' from parent '%s' "
                        "because it is not a child of that prim",
                        child->GetPath().GetText(),
                        GetPath().GetText());
        return false;
    }

    return Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::RemoveChild( 
            GetLayer(), GetPath(), child->GetNameToken());
}

SdfNameChildrenOrderProxy
SdfPrimSpec::GetNameChildrenOrder() const
{
    return SdfGetNameOrderProxy(SdfCreateHandle(this), SdfFieldKeys->PrimOrder);
}

bool
SdfPrimSpec::HasNameChildrenOrder() const
{
    return !GetNameChildrenOrder().empty();
}

void
SdfPrimSpec::SetNameChildrenOrder(const std::vector<TfToken>& names)
{
    GetNameChildrenOrder() = names;
}

void
SdfPrimSpec::InsertInNameChildrenOrder(const TfToken& name, int index)
{
    GetNameChildrenOrder().Insert(index, name);
}

void
SdfPrimSpec::RemoveFromNameChildrenOrder(const TfToken& name)
{
    GetNameChildrenOrder().Remove(name);
}

void
SdfPrimSpec::RemoveFromNameChildrenOrderByIndex(int index)
{
    GetNameChildrenOrder().Erase(index);
}

void
SdfPrimSpec::ApplyNameChildrenOrder(std::vector<TfToken>* vec) const
{
    GetNameChildrenOrder().ApplyEditsToList(vec);
}

//
// Properties
//

SdfPrimSpec::PropertySpecView
SdfPrimSpec::GetProperties() const
{
    return PropertySpecView(
        GetLayer(), GetPath(), SdfChildrenKeys->PropertyChildren);
}

void
SdfPrimSpec::SetProperties(const SdfPropertySpecHandleVector& propertySpecs)
{
    if (!_ValidateEdit(SdfChildrenKeys->PropertyChildren)) {
        return;
    }
    
    Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::SetChildren(
        GetLayer(), GetPath(), propertySpecs);
}

bool
SdfPrimSpec::InsertProperty(const SdfPropertySpecHandle& property, int index)
{
    if (!_ValidateEdit(SdfChildrenKeys->PropertyChildren)) {
        return false;
    }
    
    return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::InsertChild(
        GetLayer(), GetPath(), property, index);
}

void
SdfPrimSpec::RemoveProperty(const SdfPropertySpecHandle& property)
{
    if (!_ValidateEdit(SdfChildrenKeys->PropertyChildren)) {
        return;
    }

    if (property->GetLayer() != GetLayer() ||
            property->GetPath().GetParentPath() != GetPath()) { 
        TF_CODING_ERROR("Cannot remove property '%s' from prim '%s' because it "
                        "does not belong to that prim",
                        property->GetPath().GetText(),
                        GetPath().GetText());
        return;
    }

    Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::RemoveChild( 
            GetLayer(), GetPath(), property->GetNameToken());
}

SdfPrimSpec::AttributeSpecView
SdfPrimSpec::GetAttributes() const
{
    return AttributeSpecView(GetLayer(), GetPath(), 
                             SdfChildrenKeys->PropertyChildren);
}

SdfPrimSpec::RelationshipSpecView
SdfPrimSpec::GetRelationships() const
{
    return RelationshipSpecView(
        GetLayer(), GetPath(), SdfChildrenKeys->PropertyChildren);
}

SdfPropertyOrderProxy
SdfPrimSpec::GetPropertyOrder() const
{
    return SdfGetNameOrderProxy(
        SdfCreateHandle(this), SdfFieldKeys->PropertyOrder);
}

bool
SdfPrimSpec::HasPropertyOrder() const
{
    return !GetPropertyOrder().empty();
}

void
SdfPrimSpec::SetPropertyOrder(const std::vector<TfToken>& names)
{
    if (_ValidateEdit(SdfChildrenKeys->PropertyChildren)) {
        GetPropertyOrder() = names;
    }
}

void
SdfPrimSpec::InsertInPropertyOrder(const TfToken& name, int index)
{
    if (_ValidateEdit(SdfChildrenKeys->PropertyChildren)) {
        GetPropertyOrder().Insert(index, name);
    }
}

void
SdfPrimSpec::RemoveFromPropertyOrder(const TfToken& name)
{
    if (_ValidateEdit(SdfChildrenKeys->PropertyChildren)) {
        GetPropertyOrder().Remove(name);
    }
}

void
SdfPrimSpec::RemoveFromPropertyOrderByIndex(int index)
{
    if (_ValidateEdit(SdfChildrenKeys->PropertyChildren)) {
        GetPropertyOrder().Erase(index);
    }
}

void
SdfPrimSpec::ApplyPropertyOrder(std::vector<TfToken>* vec) const
{
    if (_ValidateEdit(SdfChildrenKeys->PropertyChildren)) {
        GetPropertyOrder().ApplyEditsToList(vec);
    }
}

//
// Lookup
//

SdfSpecHandle
SdfPrimSpec::GetObjectAtPath(const SdfPath& path) const
{
    if (path.IsEmpty()) {
        TF_CODING_ERROR("Cannot get object at the empty path");
        return TfNullPtr;
    }
    const SdfPath absPath = path.MakeAbsolutePath(GetPath());
    return GetLayer()->GetObjectAtPath(absPath);
}

SdfPrimSpecHandle
SdfPrimSpec::GetPrimAtPath(const SdfPath& path) const
{
    if (path.IsEmpty()) {
        TF_CODING_ERROR("Cannot get prim at the empty path");
        return TfNullPtr;
    }
    const SdfPath absPath = path.MakeAbsolutePath(GetPath());
    return GetLayer()->GetPrimAtPath(absPath);
}

SdfPropertySpecHandle
SdfPrimSpec::GetPropertyAtPath(const SdfPath& path) const
{
    if (path.IsEmpty()) {
        TF_CODING_ERROR("Cannot get property at the empty path");
        return TfNullPtr;
    }
    const SdfPath absPath = path.MakeAbsolutePath(GetPath());
    return GetLayer()->GetPropertyAtPath(absPath);
}

SdfAttributeSpecHandle
SdfPrimSpec::GetAttributeAtPath(const SdfPath& path) const
{
    if (path.IsEmpty()) {
        TF_CODING_ERROR("Cannot get attribute at the empty path");
        return TfNullPtr;
    }
    const SdfPath absPath = path.MakeAbsolutePath(GetPath());
    return GetLayer()->GetAttributeAtPath(absPath);
}

SdfRelationshipSpecHandle
SdfPrimSpec::GetRelationshipAtPath(const SdfPath& path) const
{
    if (path.IsEmpty()) {
        TF_CODING_ERROR("Cannot get relationship at the empty path");
        return TfNullPtr;
    }
    const SdfPath absPath = path.MakeAbsolutePath(GetPath());
    return GetLayer()->GetRelationshipAtPath(absPath);
}

//
// Metadata
//

// Initialize accessor helper macros to associate with this class and use
// member function _ValidateEdit as access predicate
#define SDF_ACCESSOR_CLASS                   SdfPrimSpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   _ValidateEdit(key_)

SDF_DEFINE_GET(TypeName, SdfFieldKeys->TypeName, TfToken)

SDF_DEFINE_GET_SET(Comment,            SdfFieldKeys->Comment,       std::string)
SDF_DEFINE_GET_SET(Documentation,      SdfFieldKeys->Documentation, std::string)
SDF_DEFINE_GET_SET(Hidden,             SdfFieldKeys->Hidden,        bool)
SDF_DEFINE_GET_SET(SymmetryFunction,   SdfFieldKeys->SymmetryFunction, TfToken)
SDF_DEFINE_GET_SET(SymmetricPeer,      SdfFieldKeys->SymmetricPeer, std::string)
SDF_DEFINE_GET_SET(Prefix,             SdfFieldKeys->Prefix,        std::string)
SDF_DEFINE_GET_SET(Suffix,             SdfFieldKeys->Suffix,        std::string)

SDF_DEFINE_GET_SET(PrefixSubstitutions,  SdfFieldKeys->PrefixSubstitutions,
                   VtDictionary)
SDF_DEFINE_GET_SET(SuffixSubstitutions,  SdfFieldKeys->SuffixSubstitutions,
                   VtDictionary)

SDF_DEFINE_GET_SET_HAS_CLEAR(Active,         SdfFieldKeys->Active,       bool)
SDF_DEFINE_GET_SET_HAS_CLEAR(Kind,           SdfFieldKeys->Kind,    TfToken)
SDF_DEFINE_GET_SET_HAS_CLEAR(Instanceable,   SdfFieldKeys->Instanceable, bool)

SDF_DEFINE_TYPED_GET_SET(Specifier,  SdfFieldKeys->Specifier,  
                         SdfSpecifier,  SdfSpecifier)
SDF_DEFINE_TYPED_GET_SET(Permission, SdfFieldKeys->Permission, 
                         SdfPermission, SdfPermission)

SDF_DEFINE_DICTIONARY_GET_SET(GetSymmetryArguments,
                              SetSymmetryArgument, 
                              SdfFieldKeys->SymmetryArguments);
SDF_DEFINE_DICTIONARY_GET_SET(GetCustomData,
                              SetCustomData,
                              SdfFieldKeys->CustomData);
SDF_DEFINE_DICTIONARY_GET_SET(GetAssetInfo,
                              SetAssetInfo,
                              SdfFieldKeys->AssetInfo);

// Clean up macro shenanigans
#undef SDF_ACCESSOR_CLASS
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE

void
SdfPrimSpec::SetTypeName(const std::string& value)
{
    if (value.empty() && GetSpecifier() != SdfSpecifierOver) {
        TF_CODING_ERROR("Cannot set empty type name on prim '%s'", 
                        GetPath().GetText());
    } else {
        if (_ValidateEdit(SdfFieldKeys->TypeName)) {
            SetField(SdfFieldKeys->TypeName, TfToken(value));
        }
    }
}

//
// Inherits
//

SdfInheritsProxy
SdfPrimSpec::GetInheritPathList() const
{
    return SdfGetPathEditorProxy(
        SdfCreateHandle(this), SdfFieldKeys->InheritPaths);
}

bool
SdfPrimSpec::HasInheritPaths() const
{
    return GetInheritPathList().HasKeys();
}

void
SdfPrimSpec::ClearInheritPathList()
{
    if (_ValidateEdit(SdfFieldKeys->InheritPaths)) {
        GetInheritPathList().ClearEdits();
    }
}

//
// Specializes
//

SdfSpecializesProxy
SdfPrimSpec::GetSpecializesList() const
{
    return SdfGetPathEditorProxy(
        SdfCreateHandle(this), SdfFieldKeys->Specializes);
}

bool
SdfPrimSpec::HasSpecializes() const
{
    return GetSpecializesList().HasKeys();
}

void
SdfPrimSpec::ClearSpecializesList()
{
    if (_ValidateEdit(SdfFieldKeys->Specializes)) {
        GetSpecializesList().ClearEdits();
    }
}

//
// Payloads
//

SdfPayloadsProxy
SdfPrimSpec::GetPayloadList() const
{
    return SdfGetPayloadEditorProxy(
        SdfCreateHandle(this), SdfFieldKeys->Payload);
}

bool
SdfPrimSpec::HasPayloads() const
{
    return GetPayloadList().HasKeys();
}

void
SdfPrimSpec::ClearPayloadList()
{
    if (_ValidateEdit(SdfFieldKeys->Payload)) {
        GetPayloadList().ClearEdits();
    }
}

//
// References
//

SdfReferencesProxy
SdfPrimSpec::GetReferenceList() const
{
    return SdfGetReferenceEditorProxy(
        SdfCreateHandle(this), SdfFieldKeys->References);
}

bool
SdfPrimSpec::HasReferences() const
{
    return GetReferenceList().HasKeys();
}

void
SdfPrimSpec::ClearReferenceList()
{
    if (_ValidateEdit(SdfFieldKeys->References)) {
        GetReferenceList().ClearEdits();
    }
}

//
// Variants
//

SdfVariantSetNamesProxy
SdfPrimSpec::GetVariantSetNameList() const
{
    boost::shared_ptr<Sdf_ListEditor<SdfNameKeyPolicy> > editor( 
            new Sdf_ListOpListEditor<SdfNameKeyPolicy>( 
                SdfCreateHandle(this), SdfFieldKeys->VariantSetNames));
    return SdfVariantSetNamesProxy(editor);
}

bool
SdfPrimSpec::HasVariantSetNames() const
{
    return GetVariantSetNameList().HasKeys();
}

std::vector<std::string> 
SdfPrimSpec::GetVariantNames(const std::string& name) const
{
    std::vector<std::string> variantNames;

    // Neither the pseudo root nor variants can have variant sets.
    if (_IsPseudoRoot() || !GetPath().IsPrimPath()) {
        return std::vector<std::string>();
    }
    SdfPath variantSetPath = GetPath().AppendVariantSelection(name, "");
    std::vector<TfToken> variantNameTokens =
        GetLayer()->GetFieldAs<std::vector<TfToken> >(variantSetPath,
            SdfChildrenKeys->VariantChildren);

    variantNames.reserve(variantNameTokens.size());
    TF_FOR_ALL(i, variantNameTokens) {
        variantNames.push_back(i->GetString());
    }

    return variantNames;
}

SdfVariantSetsProxy
SdfPrimSpec::GetVariantSets() const
{
    return SdfVariantSetsProxy(SdfVariantSetView(GetLayer(),
            GetPath(), SdfChildrenKeys->VariantSetChildren),
            "variant sets", SdfVariantSetsProxy::CanErase);
}

void
SdfPrimSpec::RemoveVariantSet(const std::string& name)
{
    if (_ValidateEdit(SdfChildrenKeys->VariantSetChildren)) {
        GetVariantSets().erase(name);
    }
}

SdfVariantSelectionProxy
SdfPrimSpec::GetVariantSelections() const
{
    if (!_IsPseudoRoot()) {
        return SdfVariantSelectionProxy(
            SdfCreateHandle(this), SdfFieldKeys->VariantSelection);
    }
    else {
        return SdfVariantSelectionProxy();
    }
}

void
SdfPrimSpec::SetVariantSelection(const std::string& variantSetName,
                            const std::string& variantName)
{
    if (_ValidateEdit(SdfFieldKeys->VariantSelection)) {
        SdfVariantSelectionProxy proxy = GetVariantSelections();
        if (proxy) {
            if (variantName.empty()) {
                proxy.erase(variantSetName);
            }
            else {
                SdfChangeBlock block;
                proxy[variantSetName] = variantName;
            }
        }
    }
}

void
SdfPrimSpec::BlockVariantSelection(const std::string& variantSetName)
{
    if (_ValidateEdit(SdfFieldKeys->VariantSelection)) {
        SdfVariantSelectionProxy proxy = GetVariantSelections();
        if (proxy) {
            SdfChangeBlock block;
            proxy[variantSetName] = std::string();
        }
    }
}

//
// Relocates
//

SdfRelocatesMapProxy
SdfPrimSpec::GetRelocates() const
{
    if (!_IsPseudoRoot()) {
        return SdfRelocatesMapProxy(
            SdfCreateHandle(this), SdfFieldKeys->Relocates);
    }
    else {
        return SdfRelocatesMapProxy();
    }
}

void
SdfPrimSpec::SetRelocates(const SdfRelocatesMap& newMap)
{
    if (_ValidateEdit(SdfFieldKeys->Relocates)) {
        GetRelocates() = newMap;
    }
}

bool
SdfPrimSpec::HasRelocates() const
{
    return HasField(SdfFieldKeys->Relocates);
}

void
SdfPrimSpec::ClearRelocates()
{
    if (_ValidateEdit(SdfFieldKeys->Relocates)) {
        ClearField(SdfFieldKeys->Relocates);
    }
}

//
// Utilities
//

static bool
_FindOrCreateVariantSpec(SdfLayer *layer, const SdfPath &vsPath)
{
    SdfVariantSetSpecHandle varSetSpec;

    SdfPrimSpecHandle primSpec = layer->GetPrimAtPath(vsPath.GetParentPath());
    if (!TF_VERIFY(primSpec)) {
        return false;
    }

    pair<string, string> varSel = vsPath.GetVariantSelection();

    // Try to find existing variant set.
    const SdfVariantSetsProxy &variantSets = primSpec->GetVariantSets();
    TF_FOR_ALL(varSetIt, variantSets) {
        if (varSetIt->first == varSel.first) {
            varSetSpec = varSetIt->second;
            break;
        }
    }

    // Create a new variant set spec and add it to the variant set list.
    if (!varSetSpec) {
        if ((varSetSpec = SdfVariantSetSpec::New(primSpec, varSel.first)))
            primSpec->GetVariantSetNameList().Add(varSel.first);
    }

    if (!TF_VERIFY(varSetSpec, "Failed to create variant set for '%s' in @%s@",
                   vsPath.GetText(), layer->GetIdentifier().c_str())) {
        return false;
    }

    // Now try to find an existing variant with the requested name.
    TF_FOR_ALL(it, varSetSpec->GetVariants()) {
        if ((*it)->GetName() == varSel.second) {
            return true;
        }
    }

    return static_cast<bool>(SdfVariantSpec::New(varSetSpec, varSel.second));
}

static bool
_IsValidPath(const SdfPath& path)
{
    // Can't use SdfCreatePrimInLayer with non-prim, non-variant paths.
    if (!path.IsAbsoluteRootOrPrimPath() &&
        !path.IsPrimVariantSelectionPath()) {
        return false;
    }
    
    // SdfPath says paths like /A/B{v=} are prim variant selection paths, but
    // such paths identify variant sets, *not* variant prims. So, we need
    // to check for this.
    //
    // We also need to check for paths like /A/B{v=}C, which are not valid
    // prim paths.
    //
    // XXX: Perhaps these conditions should be encoded in SdfPath itself?
    if (path.ContainsPrimVariantSelection()) {
        for (SdfPath p = path.MakeAbsolutePath(SdfPath::AbsoluteRootPath()); 
             p != SdfPath::AbsoluteRootPath(); p = p.GetParentPath()) {
            
            const pair<string, string> varSel = p.GetVariantSelection();
            if (!varSel.first.empty() && varSel.second.empty()) {
                return false;
            }
        }
    }

    return true;
}

namespace {

// This structure exists so that we can support relative paths to
// SdfCreatePrimInLayer/SdfJustCreatePrimInLayer without doing any path copies
// or refcount operations in the common case where we are given an absolute
// path.
struct _AbsPathHelper
{
    // Construct with \p inPath.  If \p inPath is an absolute path, then
    // GetAbsPath() returns \p inPath.  Otherwise \p inPath is made absolute by
    // MakeAbsolutePath(SdfPath::AbsoluteRootPath()), stored in a member
    // variable and returned by GetAbsPath().
    explicit _AbsPathHelper(SdfPath const &inPath)
        : _inPath(inPath) {
        if (ARCH_LIKELY(_inPath.IsAbsolutePath())) {
            _absPath = &_inPath;
        }
        else {
            _tmpPath = _inPath.MakeAbsolutePath(SdfPath::AbsoluteRootPath());
            _absPath = &_tmpPath;
        }
    }
    explicit _AbsPathHelper(SdfPath &&inPath) = delete;
    inline SdfPath const &GetAbsPath() const {
        return *_absPath;
    }
    inline SdfPath const &GetOriginalPath() const {
        return _inPath;
    }
private:
    SdfPath const &_inPath;
    SdfPath const *_absPath;  // points to either _inPath or _tmpPath.
    SdfPath _tmpPath;
};

} // anon

bool
Sdf_UncheckedCreatePrimInLayer(
    SdfLayer *layerPtr, const SdfPath& primPath)
{
    // If a prim already exists then just return it.
    if (layerPtr->HasSpec(primPath)) {
        return true;
    }

    SdfPathVector ancestors;
    ancestors.reserve(primPath.GetPathElementCount());

    bool maybeVariantSelPaths = primPath.ContainsPrimVariantSelection();
    SdfPath path = primPath;
    do {
        ancestors.emplace_back(std::move(path));
        path = ancestors.back().GetParentPath();
    } while (!layerPtr->HasSpec(path));

    // Create each prim from root-most to the prim at primPath.
    while (!ancestors.empty()) {
        SdfPath ancPath = std::move(ancestors.back());
        ancestors.pop_back();
        if (maybeVariantSelPaths && ancPath.IsPrimVariantSelectionPath()) {
            // Variant selection case.
            if (!_FindOrCreateVariantSpec(layerPtr, ancPath)) {
                return false;
            }
        } else {
            // Ordinary prim child case.
            if (ARCH_UNLIKELY(
                    !Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::CreateSpec(
                        layerPtr, ancPath, SdfSpecTypePrim, /*inert=*/true))) {
                TF_RUNTIME_ERROR("Failed to create prim at path '%s' in "
                                 "layer @%s@", ancPath.GetText(),
                                 layerPtr->GetIdentifier().c_str());
                return false;
            }
        }
    }
    return true;
}

static inline bool
Sdf_CanCreatePrimInLayer(SdfLayer *layer, _AbsPathHelper const &absPath)
{
    SdfPath const &path = absPath.GetAbsPath();
    
    if (ARCH_UNLIKELY(!_IsValidPath(path))) {
        TF_CODING_ERROR("Cannot create prim at path '%s' because it is not a "
                        "valid prim or prim variant selection path",
                        absPath.GetOriginalPath().GetText());
        return false;
    }

    if (ARCH_UNLIKELY(!layer)) {
        TF_CODING_ERROR("Cannot create prim at path '%s' in null or expired "
                        "layer", absPath.GetOriginalPath().GetText());
        return false;
    }

    return true;
}

SdfPrimSpecHandle
SdfCreatePrimInLayer(const SdfLayerHandle& layer, const SdfPath& primPath)
{
    const _AbsPathHelper abs(primPath);
    SdfLayer *layerPtr = get_pointer(layer);
    if (Sdf_CanCreatePrimInLayer(layerPtr, abs)) {
        SdfChangeBlock changeBlock;
        SdfPath const &absPath = abs.GetAbsPath();
        if (Sdf_UncheckedCreatePrimInLayer(layerPtr, absPath)) {
            return layer->GetPrimAtPath(absPath);
        }
    }
    return TfNullPtr;
}

bool
SdfJustCreatePrimInLayer(const SdfLayerHandle& layer, const SdfPath& primPath)
{
    const _AbsPathHelper abs(primPath);
    SdfLayer *layerPtr = get_pointer(layer);
    if (Sdf_CanCreatePrimInLayer(layerPtr, abs)) {
        SdfChangeBlock changeBlock;
        return Sdf_UncheckedCreatePrimInLayer(layerPtr, abs.GetAbsPath());
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
