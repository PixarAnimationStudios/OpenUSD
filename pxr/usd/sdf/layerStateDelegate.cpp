//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    const SdfPath& path,
    const TfToken& field,
    const VtValue& value,
    VtValue *oldValue)
{
    _OnSetField(path, field, value);
    _layer->_PrimSetField(
        path, field, value, oldValue, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::SetField(
    const SdfPath &path,
    const TfToken& field,
    const SdfAbstractDataConstValue& value,
    VtValue *oldValue)
{
    _OnSetField(path, field, value);
    _layer->_PrimSetField(
        path, field, value, oldValue, /* useDelegate = */ false);
}
void 
SdfLayerStateDelegateBase::SetFieldDictValueByKey(
    const SdfPath& path,
    const TfToken& field,
    const TfToken& keyPath,
    const VtValue& value,
    VtValue *oldValue)
{
    _OnSetFieldDictValueByKey(path, field, keyPath, value);
    _layer->_PrimSetFieldDictValueByKey(
        path, field, keyPath, value, oldValue, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::SetFieldDictValueByKey(
    const SdfPath& path,
    const TfToken& field,
    const TfToken& keyPath,
    const SdfAbstractDataConstValue& value,
    VtValue *oldValue)
{
    _OnSetFieldDictValueByKey(path, field, keyPath, value);
    _layer->_PrimSetFieldDictValueByKey(
        path, field, keyPath, value, oldValue, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::SetTimeSample(
    const SdfPath& path,
    double time,
    const VtValue& value)
{
    _OnSetTimeSample(path, time, value);
    _layer->_PrimSetTimeSample(path, time, value, /* useDelegate = */ false);
}

void 
SdfLayerStateDelegateBase::SetTimeSample(
    const SdfPath& path,
    double time,
    const SdfAbstractDataConstValue& value)
{
    _OnSetTimeSample(path, time, value);
    _layer->_PrimSetTimeSample(path, time, value, /* useDelegate = */ false);
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
    const SdfPath& path,
    const TfToken& fieldName,
    const VtValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetField(
    const SdfPath& path,
    const TfToken& fieldName,
    const SdfAbstractDataConstValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetFieldDictValueByKey(
    const SdfPath& path,
    const TfToken& fieldName,
    const TfToken& keyPath,
    const VtValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetFieldDictValueByKey(
    const SdfPath& path,
    const TfToken& fieldName,
    const TfToken& keyPath,
    const SdfAbstractDataConstValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetTimeSample(
    const SdfPath& path,
    double time,
    const VtValue& value)
{
    _dirty = true;
}

void 
SdfSimpleLayerStateDelegate::_OnSetTimeSample(
    const SdfPath& path,
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
