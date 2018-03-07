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
#include "pxr/pxr.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/stl.h"

#include <boost/iterator/transform_iterator.hpp>

using boost::make_transform_iterator;

PXR_NAMESPACE_OPEN_SCOPE

// Register the notice class
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdNotice::StageNotice, TfType::Bases<TfNotice> >();

    TfType::Define<
        UsdNotice::StageContentsChanged,
        TfType::Bases<UsdNotice::StageNotice> >();

    TfType::Define<
        UsdNotice::StageEditTargetChanged,
        TfType::Bases<UsdNotice::StageNotice> >();

    TfType::Define<
        UsdNotice::ObjectsChanged,
        TfType::Bases<UsdNotice::StageNotice> >();
}


UsdNotice::StageNotice::StageNotice(const UsdStageWeakPtr& stage) :
    _stage(stage)
{
}

UsdNotice::StageNotice::~StageNotice() {}

UsdNotice::StageContentsChanged::~StageContentsChanged() {}

UsdNotice::StageEditTargetChanged::~StageEditTargetChanged() {}

TfTokenVector 
UsdNotice::ObjectsChanged::PathRange::const_iterator::GetChangedFields() const
{
    TfTokenVector fields;
    for (const SdfChangeList::Entry* entry : base()->second) {
        fields.insert(fields.end(),
            make_transform_iterator(entry->infoChanged.begin(), TfGet<0>()),
            make_transform_iterator(entry->infoChanged.end(), TfGet<0>()));
    }

    std::sort(fields.begin(), fields.end());
    fields.erase(std::unique(fields.begin(), fields.end()), fields.end());
    return fields;
}

bool 
UsdNotice::ObjectsChanged::PathRange::const_iterator::HasChangedFields() const
{
    for (const SdfChangeList::Entry* entry : base()->second) {
        if (!entry->infoChanged.empty()) {
            return true;
        }
    }
    return false;
}

UsdNotice::ObjectsChanged::~ObjectsChanged() {}

bool 
UsdNotice::ObjectsChanged::ResyncedObject(const UsdObject &obj) const 
{
    // XXX: We don't need the longest prefix here, we just need to know if
    // a prefix exists in the map.
    return SdfPathFindLongestPrefix(
        make_transform_iterator(_resyncChanges->begin(), TfGet<0>()),
        make_transform_iterator(_resyncChanges->end(), TfGet<0>()),
        obj.GetPath()).base() != _resyncChanges->end();
}

bool 
UsdNotice::ObjectsChanged::ChangedInfoOnly(const UsdObject &obj) const 
{
    // XXX: We don't need the longest prefix here, we just need to know if
    // a prefix exists in the map.
    return SdfPathFindLongestPrefix(
        make_transform_iterator(_infoChanges->begin(), TfGet<0>()),
        make_transform_iterator(_infoChanges->end(), TfGet<0>()),
        obj.GetPath()).base() != _infoChanges->end();
}

UsdNotice::ObjectsChanged::PathRange
UsdNotice::ObjectsChanged::GetResyncedPaths() const
{
    return PathRange(_resyncChanges);
}

UsdNotice::ObjectsChanged::PathRange
UsdNotice::ObjectsChanged::GetChangedInfoOnlyPaths() const
{
    return PathRange(_infoChanges);
}

TfTokenVector 
UsdNotice::ObjectsChanged::GetChangedFields(const UsdObject &obj) const
{
    return GetChangedFields(obj.GetPath());
}

TfTokenVector 
UsdNotice::ObjectsChanged::GetChangedFields(const SdfPath &path) const
{
    PathRange range = GetResyncedPaths();
    PathRange::const_iterator it = range.find(path);
    if (it != range.end()) {
        return it.GetChangedFields();
    }

    range = GetChangedInfoOnlyPaths();
    it = range.find(path);
    if (it != range.end()) {
        return it.GetChangedFields();
    }

    return TfTokenVector();
}

bool
UsdNotice::ObjectsChanged::HasChangedFields(const UsdObject &obj) const
{
    return HasChangedFields(obj.GetPath());
}

bool
UsdNotice::ObjectsChanged::HasChangedFields(const SdfPath &path) const
{
    PathRange range = GetResyncedPaths();
    PathRange::const_iterator it = range.find(path);
    if (it != range.end()) {
        return it.HasChangedFields();
    }

    range = GetChangedInfoOnlyPaths();
    it = range.find(path);
    if (it != range.end()) {
        return it.HasChangedFields();
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

