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
#include "pxr/usd/usd/variantSets.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"

#include "pxr/usd/pcp/composeSite.h"
#include "pxr/usd/pcp/primIndex.h"

using std::string;
using std::vector;

// ---------------------------------------------------------------------- //
// UsdVariantSet: Public Methods
// ---------------------------------------------------------------------- //

bool
UsdVariantSet::FindOrCreateVariant(const std::string& variantName)
{
    if (SdfVariantSetSpecHandle varSet = _FindOrCreateVariantSet()){
        // If the variant spec already exists, we don't need to create it
        for (const auto& variant : varSet->GetVariants()) {
            if (variant->GetName() == variantName){
                return true;
            }
        }
        return SdfVariantSpec::New(varSet, variantName);
    }
    return false;
}

vector<string>
UsdVariantSet::GetVariantNames() const
{
    std::set<std::string> namesSet;
    TF_REVERSE_FOR_ALL(i, _prim.GetPrimIndex().GetNodeRange()) {
        if (i->GetSite().path.IsPrimPath()) {
            PcpComposeSiteVariantSetOptions(
                i->GetSite(), _variantSetName, &namesSet);
        }
    }

    return vector<string>(namesSet.begin(), namesSet.end());
}

bool 
UsdVariantSet::HasAuthoredVariant(const std::string& variantName) const
{
    std::vector<std::string> variants = GetVariantNames();

    return std::find(variants.begin(), variants.end(), variantName) 
        != variants.end();
}


string
UsdVariantSet::GetVariantSelection() const
{
    // Scan the composed prim for variant arcs for this variant set and
    // return the first selection found.  This ensures that we reflect
    // whatever composition process selected the variant, such as fallbacks.
    for (auto nodeIter = _prim.GetPrimIndex().GetNodeRange().first;
         nodeIter != _prim.GetPrimIndex().GetNodeRange().second;
         ++nodeIter) 
    {
        if (nodeIter->GetArcType() == PcpArcTypeVariant) {
            std::pair<std::string, std::string> vsel =
                nodeIter->GetSite().path.GetVariantSelection();
            if (vsel.first == _variantSetName) {
                return vsel.second;
            }
        }
    }
    return std::string();
}

bool
UsdVariantSet::HasAuthoredVariantSelection(std::string *value) const
{
    for (auto nodeIter = _prim.GetPrimIndex().GetNodeRange().first;
         nodeIter != _prim.GetPrimIndex().GetNodeRange().second;
         ++nodeIter) 
    {
        string sel;
        if (not value) {
            value = &sel;
        }
        if (PcpComposeSiteVariantSelection(
                nodeIter->GetSite(), _variantSetName, value)) {
            return true;
        }
    }
    return false;
}

bool
UsdVariantSet::SetVariantSelection(const std::string& variantName)
{
    if (SdfPrimSpecHandle spec = _CreatePrimSpecForEditing(_prim.GetPrimPath())) {
        spec->SetVariantSelection(_variantSetName, variantName);
        return true;
    }

    return false;
}

bool
UsdVariantSet::ClearVariantSelection()
{
    // empty selection is how you clear in SdfPrimSpec... don't want to
    // adopt that pattern in our API.  Let's be "clear" about it!
    return SetVariantSelection(string());
}

UsdEditTarget
UsdVariantSet::GetVariantEditTarget(const SdfLayerHandle &layer) const
{
    using std::pair;
    UsdEditTarget target;

    // Obtain the current VariantSet name & selection.  If there is no
    // selection, there is no context to pursue
    pair<string, string> curVarSel(_variantSetName, GetVariantSelection());
    if (curVarSel.second.empty())
        return target;

    UsdStagePtr stage = _prim.GetStage();
    const SdfLayerHandle &lyr = layer ? layer : 
        _prim.GetStage()->GetEditTarget().GetLayer();
    
    if (not stage->HasLocalLayer(lyr)){
        TF_CODING_ERROR("Layer %s is not a local layer of stage rooted at "
                        "layer %s", lyr->GetIdentifier().c_str(),
                        stage->GetRootLayer()->GetIdentifier().c_str());
        return target;
    }
    
    return UsdEditTarget::ForLocalDirectVariant(
        lyr,
        _prim.GetPath().AppendVariantSelection(curVarSel.first,
                                               curVarSel.second),
        stage->_cache->GetLayerStackIdentifier());
}

std::pair<UsdStagePtr, UsdEditTarget >
UsdVariantSet::GetVariantEditContext(const SdfLayerHandle &layer) const
{
    UsdEditTarget target = GetVariantEditTarget(layer);

    return std::make_pair(_prim.GetStage(), target);
}

SdfPrimSpecHandle
UsdVariantSet::_CreatePrimSpecForEditing(const SdfPath &path)
{
    return _prim.GetStage()->_CreatePrimSpecForEditing(path);
}


SdfVariantSetSpecHandle
UsdVariantSet::_FindOrCreateVariantSet()
{
    SdfPath const &primPath = _prim.GetPrimPath();
    SdfPrimSpecHandle primSpec = _CreatePrimSpecForEditing(primPath); 
    if (primSpec){
        // One can only create a VariantSet on a primPath.  If our current
        // EditTarget has us sitting right on a VariantSet, 
        // AppendVariantSelection() will fail with a coding error and return
        // the empty path
        SdfPath varSetPath = primSpec->GetPath()
            .AppendVariantSelection(_variantSetName, string());
        if (varSetPath.IsEmpty())
            return SdfVariantSetSpecHandle();
        SdfLayerHandle layer = primSpec->GetLayer();
        if (SdfSpecHandle spec = layer->GetObjectAtPath(varSetPath)){
            return TfDynamic_cast<SdfVariantSetSpecHandle>(spec);
        }
        // Otherwise, we must create a new spec, AND add it to the prim's
        // list.  Not sure why these need to be two separate steps...
        SdfVariantSetSpecHandle varSet = 
            SdfVariantSetSpec::New(primSpec, _variantSetName);
        SdfVariantSetNamesProxy  setsProxy = primSpec->GetVariantSetNameList();
        setsProxy.Add(_variantSetName);
        return varSet;
    }

    return SdfVariantSetSpecHandle();
}



// ---------------------------------------------------------------------- //
// UsdVariantSets: Public Methods
// ---------------------------------------------------------------------- //

UsdVariantSet
UsdVariantSets::GetVariantSet(const std::string& variantSetName) const
{
    if (not _prim) {
        TF_CODING_ERROR("Invalid prim");

        // XXX:
        // Define a sentinel?
        return UsdVariantSet(UsdPrim(), string());
    }
    return _prim.GetVariantSet(TfToken(variantSetName));
}

UsdVariantSet
UsdVariantSets::FindOrCreate(const std::string& variantSetName)
{
    UsdVariantSet   varSet = GetVariantSet(variantSetName);

    varSet._FindOrCreateVariantSet();

    // If everything went well, this will return a valid VariantSet.  If not,
    // you'll get an error when you try to use it, which seems good.
    return varSet;
}

bool 
UsdVariantSets::HasVariantSet(const std::string& variantSetName) const
{
    std::vector<std::string> sets = GetNames();
    return std::find(sets.begin(), sets.end(), variantSetName) != sets.end();
}

bool 
UsdVariantSets::GetNames(std::vector<std::string>* names) const
{
    TF_REVERSE_FOR_ALL(i, _prim.GetPrimIndex().GetNodeRange()) {
        PcpComposeSiteVariantSets(i->GetSite(), names);
    }
    return true;
}


std::vector<std::string> 
UsdVariantSets::GetNames() const
{
    std::vector<std::string> names;
    GetNames(&names);
    return names;
}

string
UsdVariantSets::GetVariantSelection(const std::string &variantSetName) const
{
    return GetVariantSet(variantSetName).GetVariantSelection();
}

bool
UsdVariantSets::SetSelection(const std::string& variantSetName,
                             const std::string& variantName)
{
    UsdVariantSet  vset(_prim, variantSetName);

    return vset.SetVariantSelection(variantName);
}

