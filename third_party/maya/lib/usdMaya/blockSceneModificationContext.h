//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_BLOCK_SCENE_MODIFICATION_CONTEXT_H
#define PXRUSDMAYA_BLOCK_SCENE_MODIFICATION_CONTEXT_H

/// \file usdMaya/blockSceneModificationContext.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"


PXR_NAMESPACE_OPEN_SCOPE


/// Utility class for wrapping a scope of Maya operations such that the
/// modification status of the Maya scene is preserved.
class UsdMayaBlockSceneModificationContext
{
    public:

        PXRUSDMAYA_API
        UsdMayaBlockSceneModificationContext();

        PXRUSDMAYA_API
        virtual ~UsdMayaBlockSceneModificationContext();

    private:

        /// Modification status of the scene when the context was created.
        bool _sceneWasModified;

        UsdMayaBlockSceneModificationContext(
                const UsdMayaBlockSceneModificationContext&) = delete;
        UsdMayaBlockSceneModificationContext& operator=(
                const UsdMayaBlockSceneModificationContext&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
