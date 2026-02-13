//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "registeredSceneIndexChooser.h"

#include <QTimer>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Not all scene indices are registered with the HdSceneIndexNameRegistry.
// Rather than say "Choose Scene Index", clarify that we are choosing from
// registered scene indices.
static const char* _defaultButtonText = "Choose Registered Scene Index";

HduiRegisteredSceneIndexChooser::HduiRegisteredSceneIndexChooser(
    QWidget *parent)
: QPushButton(_defaultButtonText, parent)
, _menu(new QMenu)
{
    // Associate the dropdown menu with this button.
    setMenu(_menu);

    auto refreshMenu = [this]() -> size_t {
        this->_menu->clear();
        const auto registeredSiNames = 
            HdSceneIndexNameRegistry::GetInstance().GetRegisteredNames();
        
        if (registeredSiNames.empty()) {
            this->_menu->addAction("No Registered Names")->setEnabled(false);
        } else {
            for (const std::string &name : registeredSiNames) {
                this->_menu->addAction(name.c_str());
            }
        }

        return registeredSiNames.size();
    };

    QObject::connect(_menu, &QMenu::aboutToShow, refreshMenu);

    // Register the signal handler prior to invoking refreshMenu, so that
    // we can auto-select the only registered scene index if there is just one.
    QObject::connect(_menu, &QMenu::triggered, [this](QAction *action) {
        std::string name = action->text().toStdString();
        if (HdSceneIndexBaseRefPtr sceneIndex =
                HdSceneIndexNameRegistry::GetInstance().GetNamedSceneIndex(
                    name)) {

            // Update button text to reflect selection.
            this->setText(name.c_str());
            Q_EMIT this->SceneIndexSelected(name, sceneIndex);
        }
    });

    // Populate menu items.
    const size_t numSi = refreshMenu();

    // If only one scene index is registered, select it by default.
    if (numSi == 1) {
        QAction *onlyAction = this->_menu->actions().first();

        QTimer::singleShot(0, [this, onlyAction]() {
            onlyAction->trigger();
        });
    }
}

HduiRegisteredSceneIndexChooser::~HduiRegisteredSceneIndexChooser()
{
    delete _menu;
}

PXR_NAMESPACE_CLOSE_SCOPE