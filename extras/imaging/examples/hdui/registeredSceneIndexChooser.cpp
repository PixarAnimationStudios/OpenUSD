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