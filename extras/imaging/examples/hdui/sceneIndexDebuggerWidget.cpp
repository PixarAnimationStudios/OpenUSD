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
#include "sceneIndexDebuggerWidget.h"
#include "dataSourceTreeWidget.h"
#include "dataSourceValueTreeView.h"
#include "sceneIndexTreeWidget.h"
#include "registeredSceneIndexChooser.h"
#include "sceneIndexObserverLoggingWidget.h"
#include "sceneIndexObserverLoggingTreeView.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/tf/stringUtils.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QWidgetAction>

#include <iostream>
#include <sstream>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

HduiSceneIndexDebuggerWidget::HduiSceneIndexDebuggerWidget(QWidget *parent)
: QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *toolbarLayout = new QHBoxLayout;
    mainLayout->addLayout(toolbarLayout);

    _siChooser = new HduiRegisteredSceneIndexChooser;
    toolbarLayout->addWidget(_siChooser);

    _goToInputButton = new QPushButton("Inputs");
    _goToInputButton->setEnabled(false);
    _goToInputButtonMenu = new QMenu(this);
    _goToInputButton->setMenu(_goToInputButtonMenu);

    toolbarLayout->addWidget(_goToInputButton);

    _nameLabel = new QLabel;
    toolbarLayout->addWidget(_nameLabel, 10);

    QPushButton * loggerButton = new QPushButton("Show Notice Logger");
    toolbarLayout->addWidget(loggerButton);

    toolbarLayout->addStretch();

    QSplitter * splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter, 10);

    _siTreeWidget = new HduiSceneIndexTreeWidget;
    splitter->addWidget(_siTreeWidget);

    _dsTreeWidget = new HduiDataSourceTreeWidget;
    splitter->addWidget(_dsTreeWidget);

    _valueTreeView = new HduiDataSourceValueTreeView;
    splitter->addWidget(_valueTreeView);

    QObject::connect(_siTreeWidget, &HduiSceneIndexTreeWidget::PrimSelected,
        [this](const SdfPath &primPath,
                HdContainerDataSourceHandle dataSource) {
            this->_valueTreeView->SetDataSource(nullptr);
            this->_dsTreeWidget->SetPrimDataSource(primPath, dataSource);
    });

    QObject::connect(_dsTreeWidget,
            &HduiDataSourceTreeWidget::DataSourceSelected,
        [this](HdDataSourceBaseHandle dataSource) {
            this->_valueTreeView->SetDataSource(
                    HdSampledDataSource::Cast(dataSource));
    });

    QObject::connect(_siTreeWidget, &HduiSceneIndexTreeWidget::PrimDirtied,
            [this] (const SdfPath &primPath,
                const HdDataSourceLocatorSet &locators){
        HdSceneIndexPrim prim = this->_currentSceneIndex->GetPrim(primPath);
        this->_dsTreeWidget->PrimDirtied(primPath, prim.dataSource, locators);
    });

    QObject::connect(_siChooser,
            &HduiRegisteredSceneIndexChooser::SceneIndexSelected,
        [this](const std::string &name,
                HdSceneIndexBaseRefPtr sceneIndex) {
            this->SetSceneIndex(name, sceneIndex, true);
    });

    QObject::connect(_goToInputButtonMenu, &QMenu::aboutToShow, this,
            &HduiSceneIndexDebuggerWidget::_FillGoToInputMenu);


    QObject::connect(loggerButton, &QPushButton::clicked,
        [this](){

            HduiSceneIndexObserverLoggingWidget *loggingWidget = 
                new HduiSceneIndexObserverLoggingWidget();

            loggingWidget->SetLabel(_nameLabel->text().toStdString());
            loggingWidget->show();
            if (this->_currentSceneIndex) {
                loggingWidget->GetTreeView()->SetSceneIndex(
                    this->_currentSceneIndex);
            }
    });
}

void
HduiSceneIndexDebuggerWidget::SetSceneIndex(const std::string &displayName,
    HdSceneIndexBaseRefPtr sceneIndex, bool pullRoot)
{
    _currentSceneIndex = sceneIndex;

    bool inputsPresent = false;
    if (HdFilteringSceneIndexBaseRefPtr filteringSi =
            TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(sceneIndex)) {
        if (!filteringSi->GetInputScenes().empty()) {
            inputsPresent = true;
        }
    }

    _goToInputButton->setEnabled(inputsPresent);

    std::ostringstream buffer;
    if (sceneIndex) {
        buffer << "<b><i>(";
        buffer << sceneIndex->GetDisplayName();
        buffer << ")</i></b> ";
    }
    buffer << displayName;

    _nameLabel->setText(buffer.str().c_str());

    this->_nameLabel->setText(buffer.str().c_str());
    this->_dsTreeWidget->SetPrimDataSource(SdfPath(), nullptr);
    this->_valueTreeView->SetDataSource(nullptr);

    _siTreeWidget->SetSceneIndex(sceneIndex);

    if (pullRoot) {
        _siTreeWidget->Requery();
    }
}

namespace
{
    class _InputSelectionItem : public QTreeWidgetItem
    {
    public:
        _InputSelectionItem(QTreeWidgetItem * parent)
        : QTreeWidgetItem(parent)
        {}

        HdSceneIndexBasePtr sceneIndex;
    };
}

void
HduiSceneIndexDebuggerWidget::_FillGoToInputMenu()
{
    QMenu *menu = _goToInputButtonMenu;
    menu->clear();

    QTreeWidget *menuTreeWidget = new QTreeWidget;
    menuTreeWidget->setHeaderHidden(true);
    menuTreeWidget->setAllColumnsShowFocus(true);
    menuTreeWidget->setMouseTracking(true);
    menuTreeWidget->setSizeAdjustPolicy(
            QAbstractScrollArea::AdjustToContentsOnFirstShow);

    QObject::connect(menuTreeWidget, &QTreeWidget::itemEntered,
        [menuTreeWidget](QTreeWidgetItem *item, int column) {
            menuTreeWidget->setCurrentItem(
                item, 0, QItemSelectionModel::Select | QItemSelectionModel::Clear);
    });

    QObject::connect(menuTreeWidget, &QTreeWidget::itemClicked,
        [this, menu, menuTreeWidget](QTreeWidgetItem *item, int column) {

            if (_InputSelectionItem *selectionItem =
                    dynamic_cast<_InputSelectionItem*>(item)) {

                this->SetSceneIndex("", selectionItem->sceneIndex, true);
                menu->close();
            }
    });

    _AddSceneIndexToTreeMenu(menuTreeWidget->invisibleRootItem(),
            _currentSceneIndex, false);

    QWidgetAction *widgetAction = new QWidgetAction(menu);
    widgetAction->setDefaultWidget(menuTreeWidget);
    menu->addAction(widgetAction);
}

void
HduiSceneIndexDebuggerWidget::_AddSceneIndexToTreeMenu(
    QTreeWidgetItem *parentItem, HdSceneIndexBaseRefPtr sceneIndex,
        bool includeSelf)
{
    if (!sceneIndex) {
        return;
    }

    if (includeSelf) {
        _InputSelectionItem *item = new _InputSelectionItem(parentItem);
        item->setText(0,
            sceneIndex->GetDisplayName().c_str());

        item->sceneIndex = sceneIndex;
        
        parentItem = item;
    }

    if (HdFilteringSceneIndexBaseRefPtr filteringSi =
            TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(sceneIndex)) {
        // TODO, handling multi-input branching
        std::vector<HdSceneIndexBaseRefPtr> sceneIndices =
            filteringSi->GetInputScenes();
        if (!sceneIndices.empty()) {
            parentItem->setExpanded(true);
            for (HdSceneIndexBaseRefPtr childSceneIndex : sceneIndices) {
                _AddSceneIndexToTreeMenu(parentItem, childSceneIndex, true);
            }
        } 
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
