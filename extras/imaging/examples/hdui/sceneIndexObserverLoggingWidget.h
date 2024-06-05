//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDUI_SCENE_INDEX_OBSERVER_LOGGING_WIDGET_H
#define PXR_IMAGING_HDUI_SCENE_INDEX_OBSERVER_LOGGING_WIDGET_H

#include "pxr/pxr.h"

#include "api.h"

#include <QPushButton>
#include <QLabel>

PXR_NAMESPACE_OPEN_SCOPE

class HduiSceneIndexObserverLoggingTreeView;

class HDUI_API_CLASS HduiSceneIndexObserverLoggingWidget : public QWidget
{
    Q_OBJECT;
public:
    HduiSceneIndexObserverLoggingWidget(QWidget *parent = Q_NULLPTR);

    HduiSceneIndexObserverLoggingTreeView * GetTreeView();

    void SetLabel(const std::string &labelText);

private:
    QPushButton *_startStopButton;
    QPushButton *_clearButton;
    HduiSceneIndexObserverLoggingTreeView *_treeView;

    QLabel *_label;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDUI_SCENE_INDEX_OBSERVER_LOGGING_WIDGET_H