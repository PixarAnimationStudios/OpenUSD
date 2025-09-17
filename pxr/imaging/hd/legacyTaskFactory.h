//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_LEGACY_TASK_FACTORY_H
#define PXR_IMAGING_HD_LEGACY_TASK_FACTORY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/usd/sdf/path.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
using HdTaskSharedPtr = std::shared_ptr<class HdTask>;
using HdLegacyTaskFactorySharedPtr = std::shared_ptr<class HdLegacyTaskFactory>;

///
/// \class HdLegacyTaskFactory
///
/// An abstract base class to create implementations of HdTask.
///
class HdLegacyTaskFactory
{
public:
    virtual HdTaskSharedPtr Create(
        HdSceneDelegate * delegate, const SdfPath &id) = 0;

    HD_API
    virtual ~HdLegacyTaskFactory();
};

template<typename T>
class HdLegacyTaskFactory_Impl : public HdLegacyTaskFactory
{
public:
    HdTaskSharedPtr Create(
        HdSceneDelegate * const delegate, const SdfPath &id) override
    {
        return std::make_shared<T>(delegate, id);
    }
};

/// Given a subclass implementing HdTask, create a factory for that
/// subclass.
template<typename T>
HdLegacyTaskFactorySharedPtr HdMakeLegacyTaskFactory()
{
    return std::make_shared<HdLegacyTaskFactory_Impl<T>>();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
