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
#ifndef EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CONTEXT_H
#define EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CONTEXT_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/coordSys.h"

#include "Riley.h"
#include <thread>
#include <unordered_map>
#include <mutex>

// Compile-time limit on max time samples.
// The idea is to avoid heap allocation of sample buffers in the Sync()
// calls by using fixed-size stack arrays with configured capacity.
// The capacity is indicated to the scene delegate when requesting
// time samples.
#define HDPRMAN_MAX_TIME_SAMPLES 4

class RixRiCtl;
class RixParamList;

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class HdSceneDelegate;

// Context for HdPrman to communicate with an instance of PRMan.
struct HdPrman_Context
{
    // Top-level entrypoint to PRMan.
    // Singleton used to access RixInterfaces.
    RixContext *rix = nullptr;
    // RixInterface for PRManBegin/End.
    RixRiCtl *ri = nullptr;
    // RixInterface for Riley.
    RixRileyManager *mgr = nullptr;
    // Riley instance.
    riley::Riley *riley = nullptr;

    // A fallback material to use for any geometry that
    // does not have a bound material.
    riley::MaterialId fallbackMaterial;
    riley::MaterialId fallbackVolumeMaterial;

    // Convert any Hydra primvars that should be Riley instance attributes.
    HDPRMAN_API
    RixParamList*
    ConvertAttributes(HdSceneDelegate *sceneDelegate, SdfPath const& id);

    // A vector of Riley coordinate system id's.
    typedef std::vector<riley::CoordinateSystemId> RileyCoordSysIdVec;
    // A ref-counting ptr to a vector of coordinate systems.
    typedef std::shared_ptr<RileyCoordSysIdVec> RileyCoordSysIdVecRefPtr;

    /// Convert any coordinate system bindings for the given rprim id
    /// into a Riley equivalent form.  Retain the result internally
    /// in a cache, so that we may re-use the result with other
    /// rprims with the same set of bindings.
    HDPRMAN_API
    RileyCoordSysIdVecRefPtr
    ConvertAndRetainCoordSysBindings(
        HdSceneDelegate *sceneDelegate,
        SdfPath const& id);

    /// Convert a list of categories returned by Hydra to
    /// equivalent Prman grouping attributes.
    HDPRMAN_API
    void
    ConvertCategoriesToAttributes(SdfPath const& id,
                                  VtArray<TfToken> const& categories,
                                  RixParamList *attrs);

    /// Release any coordinate system bindings cached for the given
    /// rprim id.
    HDPRMAN_API
    void ReleaseCoordSysBindings(SdfPath const& id);

    HDPRMAN_API
    void IncrementLightLinkCount(TfToken const& name);

    HDPRMAN_API
    void DecrementLightLinkCount(TfToken const& name);

    HDPRMAN_API
    bool IsLightLinkUsed(TfToken const& name);

    HDPRMAN_API
    void IncrementLightFilterCount(TfToken const& name);

    HDPRMAN_API
    void DecrementLightFilterCount(TfToken const& name);

    HDPRMAN_API
    bool IsLightFilterUsed(TfToken const& name);

    virtual ~HdPrman_Context() = default;

private:
    // Refcounts for each category mentioned by a light link.
    // This is used to convey information from lights back to the
    // geometry -- in Renderman, geometry must subscribe
    // to the linked lights.
    std::unordered_map<TfToken, size_t, TfToken::HashFunctor> _lightLinkRefs;

    // Mutex protecting lightLinkRefs.
    std::mutex _lightLinkMutex;

    std::unordered_map<TfToken, size_t, TfToken::HashFunctor> _lightFilterRefs;

    // Mutex protecting lightFilterRefs.
    std::mutex _lightFilterMutex;

    // Map from Hydra coordinate system vector pointer to Riley equivalent.
    typedef std::unordered_map<
        HdIdVectorSharedPtr, RileyCoordSysIdVecRefPtr>
        _HdToRileyCoordSysMap;
    // Map from Hydra id to cached, converted coordinate systems.
    typedef std::unordered_map<
        SdfPath, HdIdVectorSharedPtr, SdfPath::Hash>
        _GeomToHdCoordSysMap;

    // Coordinate system conversion cache.
    _GeomToHdCoordSysMap _geomToHdCoordSysMap;
    _HdToRileyCoordSysMap _hdToRileyCoordSysMap;
    std::mutex _coordSysMutex;
};

// Helper to convert matrix types, handling double->float conversion.
inline RtMatrix4x4
HdPrman_GfMatrixToRtMatrix(const GfMatrix4d &m)
{
    const double *d = m.GetArray();
    return RtMatrix4x4(
        d[0], d[1], d[2], d[3],
        d[4], d[5], d[6], d[7],
        d[8], d[9], d[10], d[11],
        d[12], d[13], d[14], d[15]);
}

// Helper to convert matrix types, handling float->double conversion.
inline GfMatrix4d
HdPrman_RtMatrixToGfMatrix(const RtMatrix4x4 &m)
{
    return GfMatrix4d(
        m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
        m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
        m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
        m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]);
}

// Convert any Hydra primvars that should be Riley primvars.
void
HdPrman_ConvertPrimvars(HdSceneDelegate *sceneDelegate, SdfPath const& id,
                        RixParamList *primvars, int numUniform, int numVertex,
                        int numVarying, int numFaceVarying);

// Resolve Hd material ID to the corresponding Riley material & displacement
bool
HdPrman_ResolveMaterial(HdSceneDelegate *sceneDelegate,
                        SdfPath const& hdMaterialId,
                        riley::MaterialId *materialId,
                        riley::DisplacementId *dispId);


/// Update the supplied list of options using searchpaths
/// pulled from envrionment variables:
///
/// - RMAN_SHADERPATH
/// - RMAN_TEXTUREPATH
/// - RMAN_RIXPLUGINPATH
///
HDPRMAN_API
void 
HdPrman_UpdateSearchPathsFromEnvironment(RixParamList *options);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CONTEXT_H
