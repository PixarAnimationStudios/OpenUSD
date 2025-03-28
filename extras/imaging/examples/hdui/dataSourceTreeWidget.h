//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDUI_DATA_SOURCE_TREE_WIDGET_H
#define PXR_IMAGING_HDUI_DATA_SOURCE_TREE_WIDGET_H

#include "pxr/pxr.h"

#include "api.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include <QTreeWidget>

PXR_NAMESPACE_OPEN_SCOPE

class HDUI_API_CLASS HduiDataSourceTreeWidget : public QTreeWidget
{
    Q_OBJECT;

public:
    HduiDataSourceTreeWidget(QWidget *parent = Q_NULLPTR);

    void SetPrimDataSource(const SdfPath &primPath,
                           HdContainerDataSourceHandle const & dataSource);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

public Q_SLOTS:
    void PrimDirtied(
        const SdfPath &primPath,
        const HdContainerDataSourceHandle &primDataSource,
        const HdDataSourceLocatorSet &locators);

Q_SIGNALS:
    void DataSourceSelected(HdDataSourceBaseHandle dataSource);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
