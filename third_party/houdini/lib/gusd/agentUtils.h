//
// Copyright 2019 Pixar
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
#ifndef GUSD_AGENTUTILS_H
#define GUSD_AGENTUTILS_H

/// \file gusd/agentUtils.h
/// \ingroup group_gusd_Agents
/// Utilities for translating agents to/from USD.
///
/// These do not provide complete, automatic conversion to/from USD at this
/// stage. Rather, these utilities may be used to build out a conversion
/// pipeline, such as generating all of the various JSON files needed to
/// build out the components of GU_Agent primitives.
///

#include "api.h"
#include "purpose.h"

#include "pxr/base/vt/array.h"
#include "pxr/usd/usd/timeCode.h"

#include <GU/GU_AgentDefinition.h>
#include <GU/GU_AgentLayer.h>
#include <GU/GU_AgentRig.h>
#include <GU/GU_AgentShapeLib.h>
#include <UT/UT_StringArray.h>


PXR_NAMESPACE_OPEN_SCOPE


class UsdSkelBinding;
class UsdSkelSkeleton;
class UsdSkelSkinningQuery;
class UsdSkelTopology;


/// Create an agent rig from a \p skel.
GUSD_API GU_AgentRigPtr
GusdCreateAgentRig(const UsdSkelSkeleton& skel);


/// Create an agent rig from \p topology and \p jointNames.
/// Each joint name must be unique.
GUSD_API GU_AgentRigPtr
GusdCreateAgentRig(const char* name,
                   const UsdSkelTopology& topology,
                   const VtTokenArray& jointNames);


/// Create a shape library where every skinning target of \p binding is
/// a separate shape.
/// The \p sev defines the error severity when reading in each shape.
/// If the severity is less than UT_ERROR_ABORT, the invalid shape is
/// skipped. Otherwise, creation of the shape lib fails if errors are
/// produced processing any shapes.
GUSD_API GU_AgentShapeLibPtr
GusdCreateAgentShapeLib(const UsdSkelBinding& binding,
                        UsdTimeCode time=UsdTimeCode::EarliestTime(),
                        const char* lod=nullptr,
                        GusdPurposeSet purpose=GUSD_PURPOSE_PROXY,
                        UT_ErrorSeverity sev=UT_ERROR_WARNING);


/// Read in a skinnable prim given by \p skinningQuery into \p gd.
/// The \p jointNames array provides the names of the joints of the bound
/// Skeleton, using the ordering specified on the Skeleton.
/// The \p invBindTransforms array holds the inverse of the Skeleton's
/// bind transforms.
/// Errors encountered while reading the skinnable primitive are reported
/// with a severity of \p sev.
GUSD_API bool
GusdReadSkinnablePrim(GU_Detail& gd,
                      const UsdSkelSkinningQuery& skinningQuery,
                      const VtTokenArray& jointNames,
                      const VtMatrix4dArray& invBindTransforms,
                      UsdTimeCode time=UsdTimeCode::EarliestTime(),
                      const char* lod=nullptr,
                      GusdPurposeSet purpose=GUSD_PURPOSE_PROXY,
                      UT_ErrorSeverity sev=UT_ERROR_ABORT);


/// Helper for writing out a rig, shape library and layer, for 
/// the skinnable primitives in \p binding.
///
/// The Skeleton primitive is converted to a GU_AgentRig, and written to
/// \p rigFile.
///
/// Each skinnable primitive is converted to a shape inside of a
/// GU_AgentShapeLibrary, with the shape named according to the prim path.
/// The resulting shape library is written to \p shapeLibFile.
///
/// Finally, a new GU_AgentLayer, named \p layerName, is created for the full
/// set of shapes in the shape library. The layer is saved as \p layerFile.
///
/// @warning: This is a *TEMPORARY* method to facilitate conversion of UsdSkel
/// based assets to GU agents for testing purposes. This method may be removed
/// in a future release of Gusd, when more robust import mechanisms have been
/// put in place.
bool
GusdWriteAgentFiles(const UsdSkelBinding& binding,
                    const char* rigFile,
                    const char* shapeLibFile,
                    const char* layerFile,
                    const char* layerName="default");


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GUSD_AGENTUTILS_H
