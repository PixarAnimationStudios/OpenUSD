//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "registeredSceneIndexChooser.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HduiRegisteredSceneIndexChooser::HduiRegisteredSceneIndexChooser(
    QWidget *parent)
: QPushButton("Choose Scene Index", parent)
, _menu(new QMenu)
{
    setMenu(_menu);

    QObject::connect(_menu, &QMenu::aboutToShow, [this]() {
        this->_menu->clear();
        bool noneFound = true;
        for (const std::string &name :
                HdSceneIndexNameRegistry::GetInstance(
                    ).GetRegisteredNames()) {
            this->_menu->addAction(name.c_str());
            noneFound = false;
        }
        if (noneFound) {
            this->_menu->addAction("No Registered Names")->setEnabled(false);
        }
    });

    QObject::connect(_menu, &QMenu::triggered, [this](QAction *action) {
        std::string name = action->text().toStdString();
        if (HdSceneIndexBaseRefPtr sceneIndex =
                HdSceneIndexNameRegistry::GetInstance().GetNamedSceneIndex(
                    name)) {
            Q_EMIT this->SceneIndexSelected(name, sceneIndex);
        }
    });
}

HduiRegisteredSceneIndexChooser::~HduiRegisteredSceneIndexChooser()
{
    delete _menu;
}

PXR_NAMESPACE_CLOSE_SCOPE