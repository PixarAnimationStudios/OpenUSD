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

/// \file usdMaya/translatorSkel.h

#ifndef PXRUSDMAYA_TRANSLATOR_SKEL_H
#define PXRUSDMAYA_TRANSLATOR_SKEL_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/base/vt/array.h"


PXR_NAMESPACE_OPEN_SCOPE


class UsdSkelSkeleton;
class UsdSkelSkeletonQuery;
class UsdSkelSkinningQuery;


struct UsdMayaTranslatorSkel
{
    /// Returns true if \p joint is being used to identify the root of
    /// a UsdSkelSkeleton.
    PXRUSDMAYA_API
    static bool IsUsdSkeleton(const MDagPath& joint);

    /// Returns true if \p Skeleton was originally generated from Maya.
    /// This is based on bool metadata Maya:generated, and is used to
    /// determine whether or not a joint should be created to represent a
    /// Skeleton when importing a Skeleton from USD that was originally
    /// created in Maya.
    PXRUSDMAYA_API
    static bool IsSkelMayaGenerated(const UsdSkelSkeleton& skel);

    /// Mark a Skeleton as being originally exported from Maya.
    PXRUSDMAYA_API
    static void MarkSkelAsMayaGenerated(const UsdSkelSkeleton& skel);

    /// Create joint nodes for each joint in \p skelQuery.
    /// Animation is applied to the joints if \p args enable it.
    PXRUSDMAYA_API
    static bool CreateJointHierarchy(const UsdSkelSkeletonQuery& skelQuery,
                                     MObject& parentNode,
                                     const UsdMayaPrimReaderArgs& args,
                                     UsdMayaPrimReaderContext* context,
                                     VtArray<MObject>* joints);

    /// Find the set of MObjects joint objects for a skeleton.
    PXRUSDMAYA_API
    static bool GetJoints(const UsdSkelSkeletonQuery& skelQuery,
                          UsdMayaPrimReaderContext* context,
                          VtArray<MObject>* joints);

    /// Create a dagPose node holding a bind pose for skel \p skelQuery.
    PXRUSDMAYA_API
    static bool CreateBindPose(const UsdSkelSkeletonQuery& skelQuery,
                               const VtArray<MObject>& joints,
                               UsdMayaPrimReaderContext* context,
                               MObject* bindPoseNode);

    /// Find the bind pose for a Skeleton.
    PXRUSDMAYA_API
    static MObject GetBindPose(const UsdSkelSkeletonQuery& skelQuery,
                               UsdMayaPrimReaderContext* context);

    /// Create a skin cluster for skinning \p primToSkin.
    /// The skinning cluster is wired up to be driven by the joints
    /// created by CreateJoints().
    /// This currently only supports mesh objects.
    PXRUSDMAYA_API
    static bool CreateSkinCluster(const UsdSkelSkeletonQuery& skelQuery,
                                  const UsdSkelSkinningQuery& skinningQuery,
                                  const VtArray<MObject>& joints,
                                  const UsdPrim& primToSkin,
                                  const UsdMayaPrimReaderArgs& args,
                                  UsdMayaPrimReaderContext* context,
                                  const MObject& bindPose=MObject());
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
