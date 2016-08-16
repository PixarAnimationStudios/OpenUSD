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
#ifndef SDF_LAYER_STATE_DELEGATE_H
#define SDF_LAYER_STATE_DELEGATE_H

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

SDF_DECLARE_HANDLES(SdfLayer);

TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerStateDelegateBase);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfSimpleLayerStateDelegate);
TF_DECLARE_WEAK_PTRS(SdfAbstractData);

class SdfAbstractDataSpecId;
class SdfAbstractDataConstValue;
class SdfPath;
class TfToken;

/// \class SdfLayerStateDelegateBase
///
/// Maintains authoring state information for an associated layer. 
///
/// For example, layers rely on a state delegate to determine whether or
/// not they have been dirtied by authoring operations.
///
/// A layer's state delegate is invoked on every authoring operation on
/// that layer. The delegate may keep track of these operations for various
/// purposes.
///
class SdfLayerStateDelegateBase
    : public TfRefBase
    , public TfWeakBase
{
public:
    virtual ~SdfLayerStateDelegateBase();

    bool IsDirty();

    void SetField(
        const SdfAbstractDataSpecId& id,
        const TfToken& field,
        const VtValue& value,
        const VtValue *oldValue=NULL);
    void SetField(
        const SdfAbstractDataSpecId& id,
        const TfToken& field,
        const SdfAbstractDataConstValue& value,
        const VtValue *oldValue=NULL);
    void SetFieldDictValueByKey(
        const SdfAbstractDataSpecId& id,
        const TfToken& field,
        const TfToken& keyPath,
        const VtValue& value,
        const VtValue *oldValue=NULL);
    void SetFieldDictValueByKey(
        const SdfAbstractDataSpecId& id,
        const TfToken& field,
        const TfToken& keyPath,
        const SdfAbstractDataConstValue& value,
        const VtValue *oldValue=NULL);

    void SetTimeSample(
        const SdfAbstractDataSpecId& id,
        double time,
        const VtValue& value);
    void SetTimeSample(
        const SdfAbstractDataSpecId& id,
        double time,
        const SdfAbstractDataConstValue& value);

    void CreateSpec(
        const SdfPath& path,
        SdfSpecType specType,
        bool inert);

    void DeleteSpec(
        const SdfPath& path,
        bool inert);

    void MoveSpec(
        const SdfPath& oldPath,
        const SdfPath& newPath);

protected:
    SdfLayerStateDelegateBase();

    /// Returns the layer associated with this state delegate.
    /// May be NULL if no layer is associated.
    SdfLayerHandle _GetLayer() const;

    /// Returns the underlying data object for the layer associated with
    /// this state delegate. May be NULL if no layer is associated.
    SdfAbstractDataPtr _GetLayerData() const;

    /// Returns true if the associated layer has been authored to since
    /// the last time the layer was marked clean, false otherwise.
    virtual bool _IsDirty() = 0;

    /// Mark the current state of the layer as clean, i.e. unchanged from its
    /// persistent representation.
    virtual void _MarkCurrentStateAsClean() = 0;

    /// Mark the current state of the layer as dirty, i.e. modified from its
    /// persistent representation.
    virtual void _MarkCurrentStateAsDirty() = 0;

    /// Invoked when the state delegate is associated with layer \p layer.
    /// \p layer may be NULL if the state delegate is being removed.
    virtual void _OnSetLayer(
        const SdfLayerHandle& layer) = 0;

    /// Invoked when a field is being changed on the associated layer.
    virtual void _OnSetField(
        const SdfAbstractDataSpecId& id,
        const TfToken& fieldName,
        const VtValue& value) = 0;
    virtual void _OnSetField(
        const SdfAbstractDataSpecId& id,
        const TfToken& fieldName,
        const SdfAbstractDataConstValue& value) = 0;

    /// Invoked when a field dict key is being changed on the associated layer.
    virtual void _OnSetFieldDictValueByKey(
        const SdfAbstractDataSpecId& id,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const VtValue& value) = 0;
    virtual void _OnSetFieldDictValueByKey(
        const SdfAbstractDataSpecId& id,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const SdfAbstractDataConstValue& value) = 0;

    /// Invoked when a time sample is being changed on the associated layer.
    virtual void _OnSetTimeSample(
        const SdfAbstractDataSpecId& id,
        double time,
        const VtValue& value) = 0;
    virtual void _OnSetTimeSample(
        const SdfAbstractDataSpecId& id,
        double time,
        const SdfAbstractDataConstValue& value) = 0;

    /// Invoked when a new spec is created on the associated layer.
    virtual void _OnCreateSpec(
        const SdfPath& path,
        SdfSpecType specType,
        bool inert) = 0;

    /// Invoked when a spec and its children are deleted from the associated 
    /// layer.
    virtual void _OnDeleteSpec(
        const SdfPath& path,
        bool inert) = 0;

    /// Invoked when a spec and its children are moved.
    virtual void _OnMoveSpec(
        const SdfPath& oldPath,
        const SdfPath& newPath) = 0;

private:
    friend class SdfLayer;
    void _SetLayer(const SdfLayerHandle& layer);

private:
    SdfLayerHandle _layer;
};

/// \class SdfSimpleLayerStateDelegate
/// A layer state delegate that simply records whether any changes have
/// been made to a layer.
class SdfSimpleLayerStateDelegate
    : public SdfLayerStateDelegateBase
{
public:
    static SdfSimpleLayerStateDelegateRefPtr New();

protected:
    SdfSimpleLayerStateDelegate();

    // SdfLayerStateDelegateBase overrides
    virtual bool _IsDirty();
    virtual void _MarkCurrentStateAsClean();
    virtual void _MarkCurrentStateAsDirty();

    virtual void _OnSetLayer(
        const SdfLayerHandle& layer);

    virtual void _OnSetField(
        const SdfAbstractDataSpecId& id,
        const TfToken& fieldName,
        const VtValue& value);
    virtual void _OnSetField(
        const SdfAbstractDataSpecId& id,
        const TfToken& fieldName,
        const SdfAbstractDataConstValue& value);
    virtual void _OnSetFieldDictValueByKey(
        const SdfAbstractDataSpecId& id,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const VtValue& value);
    virtual void _OnSetFieldDictValueByKey(
        const SdfAbstractDataSpecId& id,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const SdfAbstractDataConstValue& value);

    virtual void _OnSetTimeSample(
        const SdfAbstractDataSpecId& id,
        double time,
        const VtValue& value);
    virtual void _OnSetTimeSample(
        const SdfAbstractDataSpecId& id,
        double time,
        const SdfAbstractDataConstValue& value);

    virtual void _OnCreateSpec(
        const SdfPath& path,
        SdfSpecType specType,
        bool inert);

    virtual void _OnDeleteSpec(
        const SdfPath& path,
        bool inert);

    virtual void _OnMoveSpec(
        const SdfPath& oldPath,
        const SdfPath& newPath);

private:
    bool _dirty;
};

#endif // SDF_LAYER_STATE_DELEGATE_H
