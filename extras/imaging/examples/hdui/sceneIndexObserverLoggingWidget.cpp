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
