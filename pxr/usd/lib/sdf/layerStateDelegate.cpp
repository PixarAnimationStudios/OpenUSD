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
#include "pxr/usd/sdf/layerStateDelegate.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/staticData.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

SdfLayerStateDelegateBase::SdfLayerStateDelegateBase()
{
}

SdfLayerStateDelegateBase::~SdfLayerStateDelegateBase()
{
}

bool 
SdfLayerStateDelegateBase::IsDirty()
{
    return _IsDirty();
}

void 
SdfLayerStateDelegateBase::SetField(
    const SdfAbstractDataSpecId& id,
    const TfToken& field,
    const VtValue& value,
    const VtValue *oldValue)
{
    _OnSetField(id, field, value);
    _layer->_PrimSetField(id, field, value, oldValue, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::SetField(
    const SdfAbstractDataSpecId& id,
    const TfToken& field,
    const SdfAbstractDataConstValue& value,
    const VtValue *oldValue)
{
    _OnSetField(id, field, value);
    _layer->_PrimSetField(id, field, value, oldValue, /* useDelegate = */ false);
}
void 
SdfLayerStateDelegateBase::SetFieldDictValueByKey(
    const SdfAbstractDataSpecId& id,
    const TfToken& field,
    const TfToken& keyPath,
    const VtValue& value,
    const VtValue *oldValue)
{
    _OnSetFieldDictValueByKey(id, field, keyPath, value);
    _layer->_PrimSetFieldDictValueByKey(
        id, field, keyPath, value, oldValue, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::SetFieldDictValueByKey(
    const SdfAbstractDataSpecId& id,
    const TfToken& field,
    const TfToken& keyPath,
    const SdfAbstractDataConstValue& value,
    const VtValue *oldValue)
{
    _OnSetFieldDictValueByKey(id, field, keyPath, value);
    _layer->_PrimSetFieldDictValueByKey(
        id, field, keyPath, value, oldValue, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::SetTimeSample(
    const SdfAbstractDataSpecId& id,
    double time,
    const VtValue& value)
{
    _OnSetTimeSample(id, time, value);
    _layer->_PrimSetTimeSample(id, time, value, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::SetTimeSample(
    const SdfAbstractDataSpecId& id,
    double time,
    const SdfAbstractDataConstValue& value)
{
    _OnSetTimeSample(id, time, value);
    _layer->_PrimSetTimeSample(id, time, value, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::CreateSpec(
    const SdfPath& path,
    SdfSpecType specType,
    bool inert)
{
    _OnCreateSpec(path, specType, inert);
    _layer->_PrimCreateSpec(path, specType, inert, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::DeleteSpec(
    const SdfPath& path,
    bool inert)
{
    _OnDeleteSpec(path, inert);
    _layer->_PrimDeleteSpec(path, inert, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::MoveSpec(
    const SdfPath& oldPath,
    const SdfPath& newPath)
{
    _OnMoveSpec(oldPath, newPath);
    _layer->_PrimMoveSpec(oldPath, newPath, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::PushChild(
    const SdfPath& parentPath,
    const TfToken& field,
    const TfToken& value)
{
    _OnPushChild(parentPath, field, value);
    _layer->_PrimPushChild(parentPath, field, value, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::PushChild(
    const SdfPath& parentPath,
    const TfToken& field,
    const SdfPath& value)
{
    _OnPushChild(parentPath, field, value);
    _layer->_PrimPushChild(parentPath, field, value, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::PopChild(
    const SdfPath& parentPath,
    const TfToken& field,
    const TfToken& oldValue)
{
    _OnPopChild(parentPath, field, oldValue);
    _layer->_PrimPopChild<TfToken>(parentPath, field,
                                   /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::PopChild(
    const SdfPath& parentPath,
    const TfToken& field,
    const SdfPath& oldValue)
{
    _OnPopChild(parentPath, field, oldValue);
    _layer->_PrimPopChild<SdfPath>(parentPath, field,
                                   /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::_SetLayer(const SdfLayerHandle& layer)
{
    _layer = layer;
    _OnSetLayer(_layer);
}

SdfLayerHandle 
SdfLayerStateDelegateBase::_GetLayer() const
{
    return _layer;
}

SdfAbstractDataPtr 
SdfLayerStateDelegateBase::_GetLayerData() const
{
    return _layer ? SdfAbstractDataPtr(_layer->_data) : SdfAbstractDataPtr();
}

// ------------------------------------------------------------

SdfSimpleLayerStateDelegateRefPtr 
SdfSimpleLayerStateDelegate::New()
{
    return TfCreateRefPtr(new SdfSimpleLayerStateDelegate);
}

SdfSimpleLayerStateDelegate::SdfSimpleLayerStateDelegate()
    : _dirty(false)
{
}

bool
SdfSimpleLayerStateDelegate::_IsDirty()
{
    return _dirty;
}

void
SdfSimpleLayerStateDelegate::_MarkCurrentStateAsClean()
{
    _dirty = false;
}

void
SdfSimpleLayerStateDelegate::_MarkCurrentStateAsDirty()
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetLayer(
    const SdfLayerHandle& layer)
{
}

void 
SdfSimpleLayerStateDelegate::_OnSetField(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName,
    const VtValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetField(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName,
    const SdfAbstractDataConstValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetFieldDictValueByKey(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName,
    const TfToken& keyPath,
    const VtValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetFieldDictValueByKey(
    const SdfAbstractDataSpecId& id,
    const TfToken& fieldName,
    const TfToken& keyPath,
    const SdfAbstractDataConstValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetTimeSample(
    const SdfAbstractDataSpecId& id,
    double time,
    const VtValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetTimeSample(
    const SdfAbstractDataSpecId& id,
    double time,
    const SdfAbstractDataConstValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnCreateSpec(
    const SdfPath& path,
    SdfSpecType specType,
    bool inert)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnDeleteSpec(
    const SdfPath& path,
    bool inert)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnMoveSpec(
    const SdfPath& oldPath,
    const SdfPath& newPath)
{
    _dirty = true;
}

void
SdfSimpleLayerStateDelegate::_OnPushChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const TfToken& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnPushChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const SdfPath& value)
{
    _dirty = true;
}

void
SdfSimpleLayerStateDelegate::_OnPopChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const TfToken& oldValue)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnPopChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const SdfPath& oldValue)
{
    _dirty = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
