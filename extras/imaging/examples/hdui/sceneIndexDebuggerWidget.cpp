//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "sceneIndexDebuggerWidget.h"
#include "dataSourceTreeWidget.h"
#include "dataSourceValueTreeView.h"
#include "sceneIndexTreeWidget.h"
#include "registeredSceneIndexChooser.h"
#include "sceneIndexObserverLoggingWidget.h"
#include "sceneIndexObserverLoggingTreeView.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/instanceProxyViewSceneIndex.h"
#include "pxr/imaging/hd/utils.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/stringUtils.h"

#include <QHBoxLayout>
#include <QShortcut>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <fstream>
#include <iostream>
#include <sstream>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDUI_ENABLE_HSD_FILTER, true,
    "Enable the path expression / regex filter UI in the HSD widget.");

HduiSceneIndexDebuggerWidget::HduiSceneIndexDebuggerWidget(
    QWidget *parent,
    const Options &options)
: QWidget(parent)
, _goToInputButton(Q_NULLPTR)
, _goToInputButtonMenu(Q_NULLPTR)
, _filterAsYouTypeCheckBox(Q_NULLPTR)
, _filterLineEdit(Q_NULLPTR)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *toolbarLayout = new QHBoxLayout;
    mainLayout->addLayout(toolbarLayout);

    _siChooser = new HduiRegisteredSceneIndexChooser;
    toolbarLayout->addWidget(_siChooser);

    if (options.showInputsButton) {
        _goToInputButton = new QPushButton("Inputs");
        _goToInputButton->setEnabled(false);
        _goToInputButtonMenu = new QMenu(this);
        _goToInputButton->setMenu(_goToInputButtonMenu);
        toolbarLayout->addWidget(_goToInputButton);
    }

    _nameLabel = new QLabel;
    toolbarLayout->addWidget(_nameLabel, 10);

    QPushButton * loggerButton = new QPushButton("Show Notice Logger");
    toolbarLayout->addWidget(loggerButton);

    QPushButton * writeToFileButton = new QPushButton("Write to file");
    toolbarLayout->addWidget(writeToFileButton);

    toolbarLayout->addStretch();

    QHBoxLayout *viewOptionsLayout = new QHBoxLayout;
    mainLayout->addLayout(viewOptionsLayout);

    {
        _instanceProxyViewCheckBox = new QCheckBox("Instance Proxy View");
        viewOptionsLayout->addWidget(_instanceProxyViewCheckBox);
    }

    // Filter UI.
    QAction *filterClearAction = nullptr;
    QShortcut *filterEscShortcut = nullptr;
    if (TfGetEnvSetting(HDUI_ENABLE_HSD_FILTER)) {
        QFrame *filterSeparator = new QFrame;
        filterSeparator->setFrameShape(QFrame::VLine);
        filterSeparator->setFrameShadow(QFrame::Sunken);
        viewOptionsLayout->addWidget(filterSeparator);

        _filterAsYouTypeCheckBox = new QCheckBox("Filter as you type");
        _filterAsYouTypeCheckBox->setToolTip(
            "When checked, the tree is filtered on every keystroke.\n"
            "When unchecked, type a filter expression and press Return to apply.");
        viewOptionsLayout->addWidget(_filterAsYouTypeCheckBox);

        _filterLineEdit = new QLineEdit;
        _filterLineEdit->setPlaceholderText("path expression or regex");

        filterClearAction = _filterLineEdit->addAction(
            _filterLineEdit->style()->standardIcon(
                QStyle::SP_LineEditClearButton),
            QLineEdit::TrailingPosition);
        filterClearAction->setVisible(false);

        filterEscShortcut = new QShortcut(
            Qt::Key_Escape, _filterLineEdit,
            nullptr, nullptr, Qt::WidgetShortcut);

        viewOptionsLayout->addWidget(_filterLineEdit, /*stretch=*/5);
        viewOptionsLayout->addStretch(/*stretch=*/5);
    }

    _splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(_splitter, 10);

    QFrame *statusBarSeparator = new QFrame;
    statusBarSeparator->setFrameShape(QFrame::HLine);
    statusBarSeparator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(statusBarSeparator);

    _statusBar = new QStatusBar(this);
    _statusBar->setSizeGripEnabled(false);
    mainLayout->addWidget(_statusBar);

    _siTreeWidget = new HduiSceneIndexTreeWidget;
    _splitter->addWidget(_siTreeWidget);

    _dsTreeWidget = new HduiDataSourceTreeWidget;
    _splitter->addWidget(_dsTreeWidget);

    _valueTreeView = new HduiDataSourceValueTreeView;
    _splitter->addWidget(_valueTreeView);

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

    auto showStatus = [this](const QString &msg) {
        _statusBar->showMessage(msg);
    };

    QObject::connect(_siChooser,
        &HduiRegisteredSceneIndexChooser::StatusMessage, showStatus);
    QObject::connect(_siTreeWidget,
        &HduiSceneIndexTreeWidget::StatusMessage, showStatus);

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
            this->SetRegisteredSceneIndex(name, sceneIndex);
    });

    QObject::connect(_valueTreeView, &HduiDataSourceValueTreeView::JumpToPrim,
        [this](const SdfPath &primPath) {
            this->_siTreeWidget->SetSelectedPrimPath(primPath);
    });

    if (_goToInputButtonMenu) {
        QObject::connect(_goToInputButtonMenu, &QMenu::aboutToShow, this,
                         &HduiSceneIndexDebuggerWidget::_FillGoToInputMenu);
    }


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

    if (TfGetEnvSetting(HDUI_ENABLE_HSD_FILTER)) {
        auto clearFilter = [this, filterClearAction]() {
            this->_filterLineEdit->clear();
            this->_filterLineEdit->setStyleSheet("");
            this->_statusBar->clearMessage();
            this->_siTreeWidget->ResetFilter();
            filterClearAction->setVisible(false);
        };

        // Apply the current line edit text as a filter. Called when the
        // checkbox is on and the user types, when the checkbox is toggled on,
        // or when Return is pressed.
        auto applyCurrentFilter = [this, clearFilter]() {
            const QString text = this->_filterLineEdit->text();
            if (text.isEmpty()) {
                clearFilter();
                return;
            }
            HduiSceneIndexTreeWidget::FilterVariant filter;
            if (HduiSceneIndexTreeWidget::IsValidFilter(text, &filter)) {
                this->_filterLineEdit->setStyleSheet("");
                this->_siTreeWidget->SetFilter(filter);
            } else {
                this->_filterLineEdit->setStyleSheet(
                    "QLineEdit { background-color: rgba(255, 0, 0, 128); }");
                // Reset so an unfiltered view is shown rather than an empty one.
                this->_siTreeWidget->ResetFilter();
                this->_statusBar->showMessage("Filter: invalid expression");
            }
        };

        // The clear action and Escape both explicitly clear text and filter
        // state, bypassing the checkbox.
        QObject::connect(filterClearAction, &QAction::triggered, clearFilter);
        QObject::connect(filterEscShortcut, &QShortcut::activated, clearFilter);

        // Update the clear button visibility as the user types, and apply
        // the filter when "filter as you type" is on.
        QObject::connect(_filterLineEdit, &QLineEdit::textChanged,
            [this, filterClearAction, applyCurrentFilter](const QString &text) {
                filterClearAction->setVisible(!text.isEmpty());
                if (this->_filterAsYouTypeCheckBox->isChecked()) {
                    applyCurrentFilter();
                }
            });

        // Toggling the checkbox on applies the current text; toggling it off
        // leaves the active filter in place so the user can still see what
        // they typed.
        QObject::connect(_filterAsYouTypeCheckBox, &QCheckBox::toggled,
            [applyCurrentFilter](bool checked) {
                if (checked) {
                    applyCurrentFilter();
                }
            });

        // Return always applies, regardless of checkbox state.
        QObject::connect(_filterLineEdit, &QLineEdit::returnPressed,
            [applyCurrentFilter]() { applyCurrentFilter(); });
    }

    QObject::connect(_instanceProxyViewCheckBox, &QCheckBox::toggled,
        [this](bool checked) {
            if (!_targetSceneIndex) {
                return;
            }
            if (checked) {
                _currentSceneIndex =
                    HdInstanceProxyViewSceneIndex::New(_targetSceneIndex);
            } else {
                _currentSceneIndex = _targetSceneIndex;
            }
            
            _siTreeWidget->SetSceneIndex(_currentSceneIndex);
            _siTreeWidget->Requery();
        });

    QObject::connect(writeToFileButton, &QPushButton::clicked,
        [this](){
            const HdSceneIndexBaseRefPtr si = this->_currentSceneIndex;
            if (si) {
                const std::string fileNamePrefix =
                    "si_" + si->GetDisplayName() + "_";
                
                std::string filePath;
                if (ArchMakeTmpFile(fileNamePrefix, &filePath) == -1) {
                    TF_RUNTIME_ERROR(
                        "Could not create file to write out scene index.");
                    return;
                }

                // XXX May be useful to allow a subtree to be dumped.
                //     For now, use the absolute root.
                const SdfPath &rootPath = SdfPath::AbsoluteRootPath();

                std::fstream output(filePath, std::ios::out);
                HdUtils::PrintSceneIndex(output, si, rootPath);
                output.close();

                std::cerr << "Wrote scene index contents to "
                          << filePath << std::endl;
            }
    });
}

void
HduiSceneIndexDebuggerWidget::SetRegisteredSceneIndex(
    const std::string &registeredName,
    HdSceneIndexBaseRefPtr sceneIndex)
{
    SetSceneIndex(registeredName, std::move(sceneIndex), true);
}

void
HduiSceneIndexDebuggerWidget::SetSceneIndex(const std::string &displayName,
    HdSceneIndexBaseRefPtr sceneIndex, bool pullRoot)
{
    if (!sceneIndex) {
        TF_CODING_ERROR("Null scene index provided to SetSceneIndex");
        return;
    }

    _targetSceneIndex = sceneIndex;

    if (_instanceProxyViewCheckBox &&
            _instanceProxyViewCheckBox->isChecked()) {
        _currentSceneIndex = HdInstanceProxyViewSceneIndex::New(
            _targetSceneIndex);
    } else {
        _currentSceneIndex = _targetSceneIndex;
    }

    std::ostringstream buffer;
    if (sceneIndex) {
        buffer << "<b><i>(";
        buffer << sceneIndex->GetDisplayName();
        buffer << ")</i></b> ";
    }
    buffer << displayName;

    _nameLabel->setText(buffer.str().c_str());
    _dsTreeWidget->SetPrimDataSource(SdfPath(), nullptr);
    _valueTreeView->SetDataSource(nullptr);

    _siTreeWidget->SetSceneIndex(_currentSceneIndex);

    if (pullRoot) {
        _siTreeWidget->Requery();
    }

    _UpdateInputsButton(_targetSceneIndex);
}

void
HduiSceneIndexDebuggerWidget::_UpdateInputsButton(
    HdSceneIndexBaseRefPtr sceneIndex)
{
    if (!_goToInputButton) {
        return;
    }

    bool inputsPresent = false;
    if (HdFilteringSceneIndexBaseRefPtr filteringSi =
            TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(sceneIndex)) {
        if (!filteringSi->GetInputScenes().empty()) {
            inputsPresent = true;
        }
    }
    _goToInputButton->setEnabled(inputsPresent);
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

    QSizePolicy policy = menu->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    menu->setSizePolicy(policy);

    QTreeWidget *menuTreeWidget = new QTreeWidget;
    menuTreeWidget->setHeaderHidden(true);
    menuTreeWidget->setAllColumnsShowFocus(true);
    menuTreeWidget->setMouseTracking(true);
    menuTreeWidget->setSizeAdjustPolicy(
            QAbstractScrollArea::AdjustToContents);

    QObject::connect(menuTreeWidget, &QTreeWidget::itemEntered,
        [menuTreeWidget](QTreeWidgetItem *item, int column) {
            menuTreeWidget->setCurrentItem(
                item, 0, QItemSelectionModel::Select | QItemSelectionModel::Clear);
    });

    QObject::connect(menuTreeWidget, &QTreeWidget::itemClicked,
        [this, menu](QTreeWidgetItem *item, int column) {

            if (_InputSelectionItem *selectionItem =
                    dynamic_cast<_InputSelectionItem*>(item)) {

                this->SetSceneIndex("", selectionItem->sceneIndex, true);
                menu->close();
            }
    });

    _AddSceneIndexToTreeMenu(menuTreeWidget->invisibleRootItem(),
        _targetSceneIndex, false);

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
        item->treeWidget()->resizeColumnToContents(0);
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
