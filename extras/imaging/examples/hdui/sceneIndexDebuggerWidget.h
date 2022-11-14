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
#ifndef PXR_IMAGING_HDUI_SCENE_INDEX_DEBUGGING_WIDGET_H
#define PXR_IMAGING_HDUI_SCENE_INDEX_DEBUGGING_WIDGET_H

#include "pxr/imaging/hd/sceneIndex.h"

#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QTreeWidgetItem>

PXR_NAMESPACE_OPEN_SCOPE

class HduiSceneIndexTreeWidget;
class HduiDataSourceTreeWidget;
class HduiDataSourceValueTreeView;
class HduiRegisteredSceneIndexChooser;

class HduiSceneIndexDebuggerWidget : public QWidget, public TfWeakBase
{
    Q_OBJECT;
public:

    HduiSceneIndexDebuggerWidget(QWidget *parent = Q_NULLPTR);

    void SetSceneIndex(const std::string &displayName,
        HdSceneIndexBaseRefPtr sceneIndex, bool pullRoot);

private Q_SLOTS:
    void _FillGoToInputMenu();
    void _AddSceneIndexToTreeMenu(QTreeWidgetItem *parentItem,
            HdSceneIndexBaseRefPtr sceneIndex, bool includeSelf);

private:
    HduiSceneIndexTreeWidget *_siTreeWidget;
    HduiDataSourceTreeWidget *_dsTreeWidget;
    HduiRegisteredSceneIndexChooser *_siChooser;
    HduiDataSourceValueTreeView *_valueTreeView;
    QLabel *_nameLabel;
    QPushButton *_goToInputButton;
    QMenu *_goToInputButtonMenu;

    HdSceneIndexBasePtr _currentSceneIndex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
