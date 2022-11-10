//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HDUI_DATA_SOURCE_TREE_WIDGET_H
#define PXR_IMAGING_HDUI_DATA_SOURCE_TREE_WIDGET_H

#include "pxr/imaging/hd/sceneIndex.h"

#include <QTreeWidget>

PXR_NAMESPACE_OPEN_SCOPE

class HduiDataSourceTreeWidget : public QTreeWidget
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
