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

/// \file translatorSkel.h

#ifndef PXRUSDMAYA_TRANSLATOR_SKEL_H
#define PXRUSDMAYA_TRANSLATOR_SKEL_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/base/vt/array.h"


PXR_NAMESPACE_OPEN_SCOPE


class UsdSkelSkeletonQuery;
class UsdSkelSkinningQuery;


struct PxrUsdMayaTranslatorSkel
{
    /// Create joint nodes for each joint in \p skelQuery.
    /// Animation is applied to the joints if \p args enable it.
    PXRUSDMAYA_API
    static bool CreateJoints(const UsdSkelSkeletonQuery& skelQuery,
                             MObject& parentNode,
                             const PxrUsdMayaPrimReaderArgs& args,
                             PxrUsdMayaPrimReaderContext* context,
                             VtArray<MObject>* joints);

    /// Create a bind psoe wired up to joint nodes created for \p skelQuery.
    PXRUSDMAYA_API
    static bool CreateBindPose(const UsdSkelSkeletonQuery& skelQuery,
                               const VtArray<MObject>& joints,
                               PxrUsdMayaPrimReaderContext* context,
                               MObject* bindPoseNode);

    /// Create a skin cluster for skinning \p primToSkin.
    /// The skinning cluster is wired up to be driven by the joints
    /// created by CreateJoints().
    /// This currently only supports mesh objects.
    PXRUSDMAYA_API
    static bool CreateSkinCluster(const UsdSkelSkeletonQuery& skelQuery,
                                  const UsdSkelSkinningQuery& skinningQuery,
                                  const VtArray<MObject>& joints,
                                  const UsdPrim& primToSkin,
                                  const PxrUsdMayaPrimReaderArgs& args,
                                  PxrUsdMayaPrimReaderContext* context,
                                  const MObject& bindPose=MObject());
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_TRANSLATOR_SKEL_H
