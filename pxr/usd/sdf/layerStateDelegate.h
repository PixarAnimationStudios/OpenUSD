//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_LAYER_STATE_DELEGATE_H
#define PXR_USD_SDF_LAYER_STATE_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerStateDelegateBase);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfSimpleLayerStateDelegate);
TF_DECLARE_WEAK_PTRS(SdfAbstractData);

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
    SDF_API
    virtual ~SdfLayerStateDelegateBase();

    SDF_API
    bool IsDirty();

    SDF_API
    void SetField(
        const SdfPath& path,
        const TfToken& field,
        const VtValue& value,
        VtValue *oldValue=NULL);

    SDF_API
    void SetField(
        const SdfPath& path,
        const TfToken& field,
        const SdfAbstractDataConstValue& value,
        VtValue *oldValue=NULL);

    SDF_API
    void SetFieldDictValueByKey(
        const SdfPath& path,
        const TfToken& field,
        const TfToken& keyPath,
        const VtValue& value,
        VtValue *oldValue=NULL);

    SDF_API
    void SetFieldDictValueByKey(
        const SdfPath& path,
        const TfToken& field,
        const TfToken& keyPath,
        const SdfAbstractDataConstValue& value,
        VtValue *oldValue=NULL);

    SDF_API
    void SetTimeSample(
        const SdfPath& path,
        double time,
        const VtValue& value);

    SDF_API
    void SetTimeSample(
        const SdfPath& path,
        double time,
        const SdfAbstractDataConstValue& value);

    SDF_API
    void CreateSpec(
        const SdfPath& path,
        SdfSpecType specType,
        bool inert);

    SDF_API
    void DeleteSpec(
        const SdfPath& path,
        bool inert);

    SDF_API
    void MoveSpec(
        const SdfPath& oldPath,
        const SdfPath& newPath);

    SDF_API
    void PushChild(
        const SdfPath& parentPath,
        const TfToken& field,
        const TfToken& value);

    SDF_API
    void PushChild(
        const SdfPath& parentPath,
        const TfToken& field,
        const SdfPath& value);

    SDF_API
    void PopChild(
        const SdfPath& parentPath,
        const TfToken& field,
        const TfToken& oldValue);

    SDF_API
    void PopChild(
        const SdfPath& parentPath,
        const TfToken& field,
        const SdfPath& oldValue);

protected:
    SDF_API
    SdfLayerStateDelegateBase();

    /// Returns the layer associated with this state delegate.
    /// May be NULL if no layer is associated.
    SDF_API
    SdfLayerHandle _GetLayer() const;

    /// Returns the underlying data object for the layer associated with
    /// this state delegate. May be NULL if no layer is associated.
    SDF_API
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
        const SdfPath& path,
        const TfToken& fieldName,
        const VtValue& value) = 0;
    virtual void _OnSetField(
        const SdfPath& path,
        const TfToken& fieldName,
        const SdfAbstractDataConstValue& value) = 0;

    /// Invoked when a field dict key is being changed on the associated layer.
    virtual void _OnSetFieldDictValueByKey(
        const SdfPath& path,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const VtValue& value) = 0;
    virtual void _OnSetFieldDictValueByKey(
        const SdfPath& path,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const SdfAbstractDataConstValue& value) = 0;

    /// Invoked when a time sample is being changed on the associated layer.
    virtual void _OnSetTimeSample(
        const SdfPath& path,
        double time,
        const VtValue& value) = 0;
    virtual void _OnSetTimeSample(
        const SdfPath& path,
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

    /// Invoked when a child spec is pushed onto a parent's list of children.
    virtual void _OnPushChild(
        const SdfPath& parentPath,
        const TfToken& fieldName,
        const TfToken& value) = 0;

    /// Invoked when a child spec is pushed onto a parent's list of children.
    virtual void _OnPushChild(
        const SdfPath& parentPath,
        const TfToken& fieldName,
        const SdfPath& value) = 0;

    /// Invoked when a child spec is popped off a parent's list of children.
    virtual void _OnPopChild(
        const SdfPath& parentPath,
        const TfToken& fieldName,
        const TfToken& oldValue) = 0;

    /// Invoked when a child spec is popped off a parent's list of children.
    virtual void _OnPopChild(
        const SdfPath& parentPath,
        const TfToken& fieldName,
        const SdfPath& oldValue) = 0;

private:
    friend class SdfLayer;
    SDF_API void _SetLayer(const SdfLayerHandle& layer);

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
    SDF_API
    static SdfSimpleLayerStateDelegateRefPtr New();

protected:
    SDF_API
    SdfSimpleLayerStateDelegate();

    // SdfLayerStateDelegateBase overrides
    SDF_API
    virtual bool _IsDirty() override;

    SDF_API
    virtual void _MarkCurrentStateAsClean() override;

    SDF_API
    virtual void _MarkCurrentStateAsDirty() override;

    SDF_API
    virtual void _OnSetLayer(
        const SdfLayerHandle& layer) override;

    SDF_API
    virtual void _OnSetField(
        const SdfPath& path,
        const TfToken& fieldName,
        const VtValue& value) override;

    SDF_API
    virtual void _OnSetField(
        const SdfPath& path,
        const TfToken& fieldName,
        const SdfAbstractDataConstValue& value) override;

    SDF_API
    virtual void _OnSetFieldDictValueByKey(
        const SdfPath& path,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const VtValue& value) override;

    SDF_API
    virtual void _OnSetFieldDictValueByKey(
        const SdfPath& path,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const SdfAbstractDataConstValue& value) override;

    SDF_API
    virtual void _OnSetTimeSample(
        const SdfPath& path,
        double time,
        const VtValue& value) override;

    SDF_API
    virtual void _OnSetTimeSample(
        const SdfPath& path,
        double time,
        const SdfAbstractDataConstValue& value) override;

    SDF_API
    virtual void _OnCreateSpec(
        const SdfPath& path,
        SdfSpecType specType,
        bool inert) override;

    SDF_API
    virtual void _OnDeleteSpec(
        const SdfPath& path,
        bool inert) override;

    SDF_API
    virtual void _OnMoveSpec(
        const SdfPath& oldPath,
        const SdfPath& newPath) override;

    SDF_API
    virtual void _OnPushChild(
        const SdfPath& path,
        const TfToken& fieldName,
        const TfToken& value) override;

    SDF_API
    virtual void _OnPushChild(
        const SdfPath& path,
        const TfToken& fieldName,
        const SdfPath& value) override;

    SDF_API
    virtual void _OnPopChild(
        const SdfPath& path,
        const TfToken& fieldName,
        const TfToken& oldValue) override;

    SDF_API
    virtual void _OnPopChild(
        const SdfPath& path,
        const TfToken& fieldName,
        const SdfPath& oldValue) override;

private:
    bool _dirty;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_LAYER_STATE_DELEGATE_H
