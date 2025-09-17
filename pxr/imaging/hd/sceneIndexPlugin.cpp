//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdSceneIndexPlugin>();
}

//
// As this class is a pure interface class, it does not need a
// vtable.  However, it is possible that some users will use rtti.
// This will cause a problem for some of our compilers:
//
// In particular clang will throw a warning: -wweak-vtables
// For gcc, there is an issue were the rtti typeid's are different.
//
// As destruction of the class is not on the performance path,
// the body of the deleter is provided here, so a vtable is created
// in this compilation unit.
HdSceneIndexPlugin::~HdSceneIndexPlugin() = default;

// base implementation is a no-op which returns the input. Note that calls the
// _AppendSceneIndex virtual with no renderInstanceId as a shim in order to
// support the old plugin API that didn't get a renderInstanceId. That way,
// plugin implementers can override either method and will get invoked either
// way.
HdSceneIndexBaseRefPtr
HdSceneIndexPlugin::_AppendSceneIndex(
    const std::string &renderInstanceId,
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _AppendSceneIndex(inputScene, inputArgs);
}

HdSceneIndexBaseRefPtr
HdSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return inputScene;
}


HdSceneIndexBaseRefPtr
HdSceneIndexPlugin::AppendSceneIndex(
    const std::string &renderInstanceId,
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _AppendSceneIndex(renderInstanceId, inputScene, inputArgs);
}


PXR_NAMESPACE_CLOSE_SCOPE
