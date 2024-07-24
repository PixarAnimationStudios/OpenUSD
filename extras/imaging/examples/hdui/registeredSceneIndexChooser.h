//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDUI_REGISTERED_SCENE_INDEX_CHOOSER_H
#define PXR_IMAGING_HDUI_REGISTERED_SCENE_INDEX_CHOOSER_H

#include "pxr/pxr.h"

#include "api.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include <QPushButton>
#include <QMenu>

PXR_NAMESPACE_OPEN_SCOPE

class HDUI_API_CLASS HduiRegisteredSceneIndexChooser : public QPushButton
{
    Q_OBJECT;
public:
    HduiRegisteredSceneIndexChooser(QWidget *parent = nullptr);
    ~HduiRegisteredSceneIndexChooser() override;

Q_SIGNALS:
    void SceneIndexSelected(
        const std::string &name,
        HdSceneIndexBaseRefPtr sceneIndex);

private:
    QMenu * _menu;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
