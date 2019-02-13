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
#ifndef VMP_USD_USDVMP_H
#define VMP_USD_USDVMP_H

// Glew must be included before gl.h
#include "pxr/imaging/glf/glew.h"

#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>

#include <boost/shared_ptr.hpp>

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"

#include <FnViewerModifier/plugin/FnViewerModifier.h>
#include <FnAttribute/FnGroupBuilder.h>
#include <FnAttribute/FnAttribute.h>
#include <FnViewerModifier/plugin/FnViewerModifierInput.h>
#include <FnLogging/FnLogging.h>

#include "debugCodes.h"

FnLogSetup("USDVMP")

namespace FnKat = Foundry::Katana;


typedef boost::shared_ptr<PXR_NS::UsdImagingGLEngine> 
    UsdImagingGLEngineSharedPtr;

//--------------------------------------------------------------------------------
// USDVMP
//--------------------------------------------------------------------------------

class USDVMP : public FnKat::ViewerModifier
{
public:
    USDVMP(FnKat::GroupAttribute args);

    virtual ~USDVMP();

    static FnKat::ViewerModifier* create(FnKat::GroupAttribute args);

    static FnKat::GroupAttribute getArgumentTemplate();

    static const char* getLocationType();

    virtual void deepSetup(FnKat::ViewerModifierInput& input);

    virtual void setup(FnKat::ViewerModifierInput& input);

    virtual void draw(FnKat::ViewerModifierInput& input);

    virtual void cleanup(FnKat::ViewerModifierInput& input);

    virtual void deepCleanup(FnKat::ViewerModifierInput& input);

    static void onFrameBegin() {}
    
    static void onFrameEnd() {}
    
    // DEPRECATED: getLocalSpaceBoundingBox is preferred
    virtual FnKat::DoubleAttribute getWorldSpaceBoundingBox(
        FnKat::ViewerModifierInput& input);

    static void flush();

private:
    void _loadSubtreeForCurrentPrim();

    PXR_NS::UsdStageRefPtr _stage;
    UsdImagingGLEngineSharedPtr _renderer;
    PXR_NS::UsdImagingGLRenderParams _params;
    PXR_NS::UsdPrim _prim;

    PXR_NS::GfMatrix4d _viewMatrix;
    
    // XXX store the last time we SetRootTransform so that we can do a sloppy
    //     comparison. The current reliable technique for getting the model
    //     matrix is from GL state (due to our clever abuse of the katana
    //     proxy system to trigger a VMP in the proxy sub-scene.)
    double _lastModelMatrix[16];
    
};

#endif 
