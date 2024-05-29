//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/rprimCollection.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/base/tf/hash.h"

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
    return TfHash()(*this);
}

bool HdRprimCollection::operator==(HdRprimCollection const & other) const 
{
    return _name == other._name
       && _reprSelector == other._reprSelector
       && _forcedRepr == other._forcedRepr
       && _rootPaths == other._rootPaths
       && _excludePaths == other._excludePaths
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
    out << "name: " << v._name
        << ", repr sel: " << v._reprSelector
        << ", mat tag: " << v._materialTag
        ;
    return out;
}

size_t hash_value(HdRprimCollection const &v) {
    return v.ComputeHash();
}

PXR_NAMESPACE_CLOSE_SCOPE

