//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DATA_SOURCE_LEGACY_TASK_PRIM_H
#define PXR_IMAGING_HD_DATA_SOURCE_LEGACY_TASK_PRIM_H

#include "pxr/usd/sdf/path.h"

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/dataSource.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdLegacyTaskFactorySharedPtr = std::shared_ptr<class HdLegacyTaskFactory>;
class HdSceneDelegate;

/// \class HdDataSourceLegacyTaskPrim
///
/// This is an HdContainerDataSource which represents a prim-level data source
/// for a task for adapting HdSceneDelegate calls into the forms defined by
/// HdSchemas during emulation of legacy scene delegates.
///
class HdDataSourceLegacyTaskPrim : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdDataSourceLegacyTaskPrim);

    ~HdDataSourceLegacyTaskPrim() override;

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    HdDataSourceLegacyTaskPrim(
        const SdfPath& id,
        HdSceneDelegate *sceneDelegate,
        HdLegacyTaskFactorySharedPtr factory);

    const SdfPath _id;
    HdSceneDelegate * const _sceneDelegate;
    HdLegacyTaskFactorySharedPtr const _factory;
};

HD_DECLARE_DATASOURCE_HANDLES(HdDataSourceLegacyTaskPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
