//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "sceneIndexObserverLoggingWidget.h"
#include "sceneIndexObserverLoggingTreeView.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

PXR_NAMESPACE_OPEN_SCOPE

HduiSceneIndexObserverLoggingWidget::HduiSceneIndexObserverLoggingWidget(
    QWidget *parent)
: QWidget(parent)
{
    setWindowTitle("Scene Index Notice Logger");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *toolbarLayout = new QHBoxLayout;
    mainLayout->addLayout(toolbarLayout);

    _startStopButton = new QPushButton("Start Recording");
    _clearButton = new QPushButton("Clear");

    _label = new QLabel("");

    toolbarLayout->addWidget(_startStopButton);
    toolbarLayout->addWidget(_label, 10);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(_clearButton);

    _treeView = new HduiSceneIndexObserverLoggingTreeView;

    mainLayout->addWidget(_treeView, 10);

    QObject::connect(_startStopButton, &QPushButton::clicked,
        [this](){
        if (this->_treeView->IsRecording()) {
            this->_treeView->StopRecording();
        } else {
            this->_treeView->StartRecording();
        }
    });

    QObject::connect(_clearButton, &QPushButton::clicked,
        [this](){
        this->_treeView->Clear();
    });

    QObject::connect(_treeView,
        &HduiSceneIndexObserverLoggingTreeView::RecordingStarted, [this]() {
        this->_startStopButton->setText("Stop Recording");
    });

    QObject::connect(_treeView,
        &HduiSceneIndexObserverLoggingTreeView::RecordingStopped, [this]() {
        this->_startStopButton->setText("Start Recording");
    });
}


HduiSceneIndexObserverLoggingTreeView *
HduiSceneIndexObserverLoggingWidget::GetTreeView()
{
    return _treeView;
}

void
HduiSceneIndexObserverLoggingWidget::SetLabel(const std::string &labelText)
{
    _label->setText(labelText.c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE
