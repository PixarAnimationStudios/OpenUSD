//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDUI_SCENE_INDEX_DEBUGGING_WIDGET_H
#define PXR_IMAGING_HDUI_SCENE_INDEX_DEBUGGING_WIDGET_H

#include "pxr/pxr.h"

#include "api.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QTreeWidgetItem>
#include <QSplitter>

PXR_NAMESPACE_OPEN_SCOPE

class HduiSceneIndexTreeWidget;
class HduiDataSourceTreeWidget;
class HduiDataSourceValueTreeView;
class HduiRegisteredSceneIndexChooser;

class HDUI_API_CLASS HduiSceneIndexDebuggerWidget
    : public QWidget, public TfWeakBase
{
    Q_OBJECT;
public:
    struct Options
    {
        Options() {};
        bool showInputsButton = true;
    };

    // customSceneIndexGraphWidget: clients can pass their own custom
    // widget. It will be added as first column in the debugger and is
    // in charge of selecting the scene index to be inspected.
    // Thus, we suppress the "Inputs" button to select a scene index if
    // such a custom widget is given.
    //
    HduiSceneIndexDebuggerWidget(
        QWidget *parent = Q_NULLPTR,
        const Options &options = Options());

    // Called when we select a registered (terminal) scene index.
    virtual void SetRegisteredSceneIndex(
        const std::string &registeredName,
        HdSceneIndexBaseRefPtr sceneIndex);

    // Sets which scene index we are inspecting.
    void SetSceneIndex(const std::string &displayName,
                       HdSceneIndexBaseRefPtr sceneIndex, bool pullRoot);

protected:
    QSplitter *_splitter;

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
