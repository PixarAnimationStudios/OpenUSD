//
// Copyright 2016 Pixar
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
#ifndef PXRUSDKATANA_USDIN_ARGS_H
#define PXRUSDKATANA_USDIN_ARGS_H

#include "usdKatana/api.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/base/tf/refPtr.h"

#include <tbb/enumerable_thread_specific.h>

class PxrUsdKatanaUsdInArgs;
typedef TfRefPtr<PxrUsdKatanaUsdInArgs> PxrUsdKatanaUsdInArgsRefPtr;

/// \brief Reference counted container for op state that should be constructed
/// at an ops root and passed to read USD prims into Katana attributes.
///
/// This should hold ref pointers or shareable copies of state that should not
/// be copied at each location.
///

class PxrUsdKatanaUsdInArgs : public TfRefBase
{

public:

    typedef std::map<std::string, std::vector<std::string> > StringListMap;

    static PxrUsdKatanaUsdInArgsRefPtr New(
            UsdStageRefPtr stage,
            const std::string& rootLocation,
            const std::string& isolatePath,
            const SdfPathSet& variantSelections,
            const std::string& ignoreLayerRegex,
            double currentTime,
            double shutterOpen,
            double shutterClose,
            const std::vector<double>& motionSampleTimes,
            const StringListMap& extraAttributesOrNamespaces,
            bool verbose) {
        return TfCreateRefPtr(new PxrUsdKatanaUsdInArgs(
                    stage, 
                    rootLocation,
                    isolatePath,
                    variantSelections,
                    ignoreLayerRegex,
                    currentTime,
                    shutterOpen,
                    shutterClose, 
                    motionSampleTimes,
                    extraAttributesOrNamespaces,
                    verbose));
    }

    // bounds computation is kind of important, so we centralize it here.
    USDKATANA_API
    std::vector<GfBBox3d> ComputeBounds(const UsdPrim& prim);

    USDKATANA_API
    UsdPrim GetRootPrim() const;

    UsdStageRefPtr GetStage() const {
        return _stage;
    }

    std::string GetFileName() const {
        return _stage->GetRootLayer()->GetIdentifier();
    }

    const std::string& GetRootLocationPath() const {
        return _rootLocation;
    }

    const std::string& GetIsolatePath() const {
        return _isolatePath;
    }

    const std::set<SdfPath>& GetVariantSelections() const {
        return _variantSelections;
    }

    const std::string& GetIgnoreLayerRegex() const {
        return _ignoreLayerRegex;
    }

    double GetCurrentTimeD() const {
        return _currentTime;
    }

    double GetShutterOpen() const {
        return _shutterOpen;
    }

    double GetShutterClose() const {
        return _shutterClose;
    }

    const std::vector<double>& GetMotionSampleTimes() const {
        return _motionSampleTimes;
    }

    const StringListMap& GetExtraAttributesOrNamespaces() const {
        return _extraAttributesOrNamespaces;
    }

    bool IsVerbose() const {
        return _verbose;
    }

    USDKATANA_API
    std::vector<UsdGeomBBoxCache>& GetBBoxCache() {
        return _bboxCaches.local();
    }

private:
    USDKATANA_API
    PxrUsdKatanaUsdInArgs(
            UsdStageRefPtr stage,
            const std::string& rootLocation,
            const std::string& isolatePath,
            const SdfPathSet& variantSelections,
            const std::string& ignoreLayerRegex,
            double currentTime,
            double shutterOpen,
            double shutterClose,
            const std::vector<double>& motionSampleTimes,
            const StringListMap& extraAttributesOrNamespaces,
            bool verbose);

    USDKATANA_API
    ~PxrUsdKatanaUsdInArgs();

    UsdStageRefPtr _stage;

    std::string _rootLocation;
    std::string _isolatePath;

    std::set<SdfPath> _variantSelections;
    std::string _ignoreLayerRegex;

    double _currentTime;
    double _shutterOpen;
    double _shutterClose;
    std::vector<double> _motionSampleTimes;

    // maps the root-level attribute name to the specified attributes or namespaces
    StringListMap _extraAttributesOrNamespaces;

    bool _verbose;

    typedef tbb::enumerable_thread_specific< std::vector<UsdGeomBBoxCache> > _ThreadLocalBBoxCaches;
    _ThreadLocalBBoxCaches _bboxCaches;

};

#endif // PXRUSDKATANA_USDIN_ARGS_H
