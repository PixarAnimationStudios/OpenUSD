//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDUI_DATA_SOURCE_VALUE_TREE_VIEW_H
#define PXR_IMAGING_HDUI_DATA_SOURCE_VALUE_TREE_VIEW_H

#include "pxr/pxr.h"
#include "api.h"

#include "pxr/imaging/hd/dataSource.h"

#include <QTreeView>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;

class HDUI_API_CLASS HduiDataSourceValueTreeView : public QTreeView
{
    Q_OBJECT;
public:
    HduiDataSourceValueTreeView(QWidget *parent = Q_NULLPTR);
    void SetDataSource(const HdSampledDataSourceHandle &dataSource);
    void Refresh();

Q_SIGNALS:
    void JumpToPrim(const SdfPath &primPath);

private:
    HdSampledDataSourceHandle _dataSource;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
