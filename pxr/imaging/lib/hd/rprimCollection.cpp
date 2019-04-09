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
#include "pxr/imaging/hd/rprimCollection.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/rprim.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


HdRprimCollection::HdRprimCollection()
    : _forcedRepr(false)
    , _rootPaths(1, SdfPath::AbsoluteRootPath())
{
    /*NOTHING*/
}

HdRprimCollection::HdRprimCollection(TfToken const& name,
                                     HdReprSelector const& reprSelector,
                                     bool forcedRepr,
                                     TfToken const& materialTag)
    : _name(name)
    , _reprSelector(reprSelector)
    , _forcedRepr(forcedRepr)
    , _materialTag(materialTag)
    , _rootPaths(1, SdfPath::AbsoluteRootPath())
{
}

HdRprimCollection::HdRprimCollection(TfToken const& name,
                                     HdReprSelector const& reprSelector,
                                     SdfPath const& rootPath,
                                     bool forcedRepr,
                                     TfToken const& materialTag)
    : _name(name)
    , _reprSelector(reprSelector)
    , _forcedRepr(forcedRepr)
    , _materialTag(materialTag)
{
    if (!rootPath.IsAbsolutePath()) {
        TF_CODING_ERROR("Root path must be absolute");
        _rootPaths.push_back(SdfPath::AbsoluteRootPath());
    } else {
        _rootPaths.push_back(rootPath);
    }
}

HdRprimCollection::HdRprimCollection(HdRprimCollection const& col)
{
    _name = col._name;
    _reprSelector = col._reprSelector;
    _forcedRepr = col._forcedRepr;
    _renderTags = col._renderTags;
    _rootPaths = col._rootPaths;
    _excludePaths = col._excludePaths;
    _materialTag = col._materialTag;
}

HdRprimCollection::~HdRprimCollection()
{
    /*NOTHING*/
}

HdRprimCollection
HdRprimCollection::CreateInverseCollection() const
{
    // Use the copy constructor and then swap the root and exclude paths
    HdRprimCollection invCol(*this);
    invCol._rootPaths.swap(invCol._excludePaths);

    return invCol;
}

SdfPathVector const& 
HdRprimCollection::GetRootPaths() const
{
    return _rootPaths;
}

void 
HdRprimCollection::SetRootPaths(SdfPathVector const& rootPaths)
{
    TF_FOR_ALL(pit, rootPaths) {
        if (!pit->IsAbsolutePath()) {
            TF_CODING_ERROR("Root path must be absolute (<%s>)",
                    pit->GetText());
            return;
        }
    }

    _rootPaths = rootPaths;
    std::sort(_rootPaths.begin(), _rootPaths.end());
}

void 
HdRprimCollection::SetRootPath(SdfPath const& rootPath)
{
    if (!rootPath.IsAbsolutePath()) {
        TF_CODING_ERROR("Root path must be absolute");
        return;
    }
    _rootPaths.clear();
    _rootPaths.push_back(rootPath);
}

void
HdRprimCollection::SetExcludePaths(SdfPathVector const& excludePaths)
{
    TF_FOR_ALL(pit, excludePaths) {
        if (!pit->IsAbsolutePath()) {
            TF_CODING_ERROR("Exclude path must be absolute (<%s>)",
                    pit->GetText());
            return;
        }
    }

    _excludePaths = excludePaths;
    std::sort(_excludePaths.begin(), _excludePaths.end());
}

SdfPathVector const& 
HdRprimCollection::GetExcludePaths() const
{
    return _excludePaths;
}

void 
HdRprimCollection::SetRenderTags(TfTokenVector const& renderTags)
{
    _renderTags = renderTags;
}

TfTokenVector const& 
HdRprimCollection::GetRenderTags() const
{
    return _renderTags;
}

bool
HdRprimCollection::HasRenderTag(TfToken const & renderTag) const
{
    if (_renderTags.empty()) {
        return true;  
    } 

    TF_FOR_ALL (t, _renderTags) {
        if (renderTag == *t) {
            return true;  
        } 
    }

    return false;
}

void 
HdRprimCollection::SetMaterialTag(TfToken const& tag)
{
    _materialTag = tag;
}

TfToken const& 
HdRprimCollection::GetMaterialTag() const
{
    return _materialTag;
}

size_t
HdRprimCollection::ComputeHash() const
{
    size_t h = _name.Hash();
    boost::hash_combine(h, _reprSelector.Hash());
    boost::hash_combine(h, _forcedRepr);
    TF_FOR_ALL(pathIt, _rootPaths) {
        boost::hash_combine(h, SdfPath::Hash()(*pathIt));
    }
    TF_FOR_ALL(pathIt, _excludePaths) {
        boost::hash_combine(h, SdfPath::Hash()(*pathIt));
    }
    TF_FOR_ALL(rtIt, _renderTags) {
        boost::hash_combine(h, rtIt->Hash());
    }

    boost::hash_combine(h, _materialTag);
    return h;
}

bool HdRprimCollection::operator==(HdRprimCollection const & other) const 
{
    return _name == other._name
       && _reprSelector == other._reprSelector
       && _forcedRepr == other._forcedRepr
       && _rootPaths == other._rootPaths
       && _excludePaths == other._excludePaths
       && _renderTags == other._renderTags
       && _materialTag == other._materialTag;
}

bool HdRprimCollection::operator!=(HdRprimCollection const & other) const 
{
    return !(*this == other);
}

// -------------------------------------------------------------------------- //
// VtValue requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, HdRprimCollection const & v)
{
    out << v._name
        ;
    return out;
}

size_t hash_value(HdRprimCollection const &v) {
    return v.ComputeHash();
}

PXR_NAMESPACE_CLOSE_SCOPE

