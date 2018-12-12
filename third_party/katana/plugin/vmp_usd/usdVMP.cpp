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
#include "usdVMP.h"

#include "pxr/pxr.h"
#include "usdKatana/cache.h"
#include "usdKatana/locks.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/arch/systemInfo.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_ENV_SETTING(USDVMP_PROXY_OVERLAY, "ghosted",
            "Overlay effect to distinguish proxies from real geometry. "
            "Values: ghosted, none, wireframe");

TF_DEFINE_PRIVATE_TOKENS(
        _tokens,
        (ghosted)
        (none)
        (wireframe)
);

static TfToken 
_GetProxyOverlayMode()
{
    TfToken overlay(TfGetEnvSetting(USDVMP_PROXY_OVERLAY));
    if (overlay != _tokens->ghosted
        && overlay != _tokens->none
        && overlay != _tokens->wireframe)
    {
        TF_WARN("Invalid proxy USDVMP_PROXY_OVERLAY mode: %s\n", overlay.GetText());
        return _tokens->ghosted;
    }
    return overlay;
}

USDVMP::USDVMP(FnKat::GroupAttribute args) :
    FnKat::ViewerModifier(args)
{
    TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg("%s @ %p\n",
                TF_FUNC_NAME().c_str(), this);
    GlfGlewInit();
}

USDVMP::~USDVMP()
{
    TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg("%s @ %p\n",
            TF_FUNC_NAME().c_str(), this);
}

FnKat::ViewerModifier*
USDVMP::create(FnKat::GroupAttribute args)
{
    return (FnKat::ViewerModifier*)new USDVMP(args);
}

FnKat::GroupAttribute
USDVMP::getArgumentTemplate()
{
    FnKat::GroupBuilder gb;
    return gb.build();
}

const char*
USDVMP::getLocationType()
{
    return "usd";
}

void
USDVMP::setup(FnKat::ViewerModifierInput& input)
{
    TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg("%s @ %p : %s\n",
            TF_FUNC_NAME().c_str(), this, input.getFullName().c_str());

    // The multi-threaded Usd Op may be loading or unloading models on the stage
    // we need, so we grab the global lock in reader mode.
    boost::shared_lock<boost::upgrade_mutex> readerLock(UsdKatanaGetStageLock());

    // Open stage if necessary.
    if (!_stage) {

        // Get usd file, node path, and current time, 
        // needed to call TidSceneRenderer
        FnKat::StringAttribute usdFileAttr = 
                input.getAttribute("fileName");
        FnKat::StringAttribute usdRootLocationAttr = 
                input.getAttribute("rootLocation");
        FnKat::StringAttribute usdReferencePathAttr = 
                input.getAttribute("referencePath");
        
        FnKat::GroupAttribute sessionAttr =
               input.getAttribute("session");
        
        std::string sessionLocation = FnKat::StringAttribute(
                input.getAttribute("rootLocation")).getValue("", false);
        sessionLocation = FnKat::StringAttribute(
                input.getAttribute("sessionLocation")).getValue(
                        sessionLocation, false);

        
        FnKat::StringAttribute ignoreLayerAttr=
                input.getAttribute("ignoreLayerRegex");
        FnKat::FloatAttribute forcePopulateAttr = 
                input.getAttribute("forcePopulateUsdStage");

        std::string usdFile = usdFileAttr.getValue("", false);
        std::string usdRootLocation = usdRootLocationAttr.getValue("", false);
        std::string usdReferencePath = usdReferencePathAttr.getValue("",false);
        
        std::string sessionKey;
        if (sessionAttr.isValid())
        {
            sessionKey = sessionAttr.getHash().str();
        }
        
        std::string ignoreLayerRegex = ignoreLayerAttr.getValue("$^", false);
        bool forcePopulate = forcePopulateAttr.getValue((float)true,false);

        if (usdFile.empty())
            return;

        _stage = UsdKatanaCache::GetInstance().GetStage(usdFile, 
                                                        sessionAttr,
                                                        sessionLocation,
                                                        ignoreLayerRegex,
                                                        forcePopulate);

        if (!_stage) {
            TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg(
                "Cannot resolve path %s", usdFile.c_str());
            return;
        } 

        if (usdReferencePath == "")
            _prim = _stage->GetPseudoRoot();
        else
            _prim = _stage->GetPrimAtPath(SdfPath(usdReferencePath));

        if (!_prim)
            FnLogWarn(std::string("Cannot compose ") + 
                _prim.GetPath().GetString());

        _params.cullStyle = 
            UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED;

        _renderer = UsdKatanaCache::GetInstance()
                                  .GetRenderer(_stage, _prim, sessionKey);
    }
    
    // always update frame time
    FnKat::DoubleAttribute currentTimeAttr = input.getAttribute("currentTime");
    double currentTime = currentTimeAttr.getValue(0.0, false);
    _params.frame = currentTime;
    
    
    // To allow drawing of proxies, load the current prim's subtree
    // before rendering.
    _loadSubtreeForCurrentPrim();
    
}

void
USDVMP::deepSetup(FnKat::ViewerModifierInput& input)
{
    TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg("%s @ %p : %s\n",
            TF_FUNC_NAME().c_str(), this, input.getFullName().c_str());

    
    // We are taking over all drawing for this location.
    input.overrideHostGeometry();

    // store the view matrix
    FnKat::DoubleAttribute mAttr = input.getDisplayMatrix(VIEW);
    if (mAttr.isValid()) {
        FnKat::DoubleConstVector vm = mAttr.getNearestSample(0.f);
        _viewMatrix =
              GfMatrix4d(GfRotation(GfVec3d(-1,0,0), 90), GfVec3d(0))
            * GfMatrix4d(vm[0], vm[1], vm[2], vm[3],
                         vm[4], vm[5], vm[6], vm[7],
                         vm[8], vm[9], vm[10], vm[11],
                         vm[12], vm[13], vm[14], vm[15]);
    } else {
        _viewMatrix = GfMatrix4d(1);
    }
}


void
USDVMP::draw(FnKat::ViewerModifierInput& input) 
{
    TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg("%s @ %p : %s\n",
            TF_FUNC_NAME().c_str(), this, _prim.GetPath().GetString().c_str());

    // Render
    if (_stage) {
        // Get draw options needed for styling.
        bool isSelected = input.getDrawOption("selected");
        bool drawPoints = input.getDrawOption("fillPoints");
        bool drawWireframe = input.getDrawOption("fillWireframe");
        bool drawSmooth = input.getDrawOption("shadingSmooth");
        bool isPicking = input.getDrawOption("isPicking");

        // Clear out override color
        _params.overrideColor[3] = 0.0f;

        // Determine the approrpiate draw mode based on the styling options.
        if ( drawSmooth ) {
            if (_GetProxyOverlayMode() == _tokens->wireframe) {
                _params.drawMode = 
                    UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE;
            } else {
                _params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
            }
        }
        if ( drawWireframe ) {
            _params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
        }
        if ( drawPoints ) { 
            // TODO: support draw points
            _params.drawMode = UsdImagingGLDrawMode::DRAW_POINTS;
        }

        // If this gprim is selected setup drawmode and selection color.
        if ( isSelected ) {
            _params.drawMode = UsdImagingGLDrawMode::DRAW_GEOM_SMOOTH;
            _params.overrideColor = GfVec4f(0.0f, 1.0f, 1.0f, 1.0f);
            glColor4fv(_params.overrideColor.GetArray());
        }
        if (isPicking) {
            if(input.getDrawOption("hasPickColor") == 1) {
                GfVec4f pickColor(0, 0, 0, 1);
                pickColor[0] = input.getDrawOptionFloat("pickColorR");
                pickColor[1] = input.getDrawOptionFloat("pickColorG");
                pickColor[2] = input.getDrawOptionFloat("pickColorB");
                _params.overrideColor = pickColor;
            }
            else {
                // Most horrible hack in the world :(
                // Katana does it's picking by setting a shader
                // that takes a pick id and renders geometry with
                // the color representation of that id.
                // Unfortunately if we are using Hydra, we need to
                // use our own shaders. To get around this, we are
                // using specific knowledge of the katana pick shader
                // to extract the pick id and set our own override color
                // based on this id. This is basically emulating what the
                // katana shader is doing.
                GLint program = -1;
                glGetIntegerv(GL_CURRENT_PROGRAM, &program);
                TF_VERIFY(program != -1);
                GLint kat_PickIdLoc = glGetUniformLocation(program, "kat_PickId");
                if (TF_VERIFY(kat_PickIdLoc != -1)) {
                    GLint kat_PickId;
                    glGetUniformiv(program, kat_PickIdLoc, &kat_PickId);
                    // Simulate pick id with color
                    _params.overrideColor = GfVec4f(
                        ((float)((kat_PickId >> 0  ) & 0xff)) / 255.0f,
                        ((float)((kat_PickId >> 8  ) & 0xff)) / 255.0f,
                        ((float)((kat_PickId >> 16 ) & 0xff)) / 255.0f,
                        1.0f);
                }
            }
            // Using DRAW_GEOM_ONLY will disable lighting and make
            // sure we are rendering a solid color
            _params.drawMode = UsdImagingGLDrawMode::DRAW_GEOM_ONLY;
        }

        // Save and restore shader settings around render call
        // because hydra does not restore shader state.
        GLint oldProgram = -1;
        glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);

        if (TF_VERIFY(_renderer)) {
            // Copy camera from GL state, Katana does not provide it directly
            GfMatrix4d modelViewMatrix, projectionMatrix;
            GfVec4d viewport;
            glGetDoublev(GL_MODELVIEW_MATRIX, modelViewMatrix.GetArray());
            glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix.GetArray());
            glGetDoublev(GL_VIEWPORT, &viewport[0]);

            _renderer->SetCameraState(_viewMatrix, projectionMatrix, viewport);

            GfMatrix4d modelMatrix = modelViewMatrix * (_viewMatrix.GetInverse());
            
            /// XXX suppressing repeated calls to SetRootTransform for very
            ///     similar values. The episilon is big to account for the
            ///     precision lost when computing relative to the _viewMatrix
            ///     Querying the GL state for transformation is currently the
            ///     most reliable way of getting (close to) accurate answers
            ///     for all cases given our use of a VMP within a katana viewer
            ///     proxy sub-scene.
            ///     TODO: find a better way and ensure that katana 3.0's
            ///           viewer can answer this directly.
            double * modelMatrixValues = modelMatrix.GetArray();
            for (int i = 0; i < 16; ++i)
            {
                if (!GfIsClose(modelMatrixValues[i], _lastModelMatrix[i], 0.01))
                {
                    _renderer->SetRootTransform(modelMatrix);
                    memcpy(&_lastModelMatrix[0], modelMatrixValues,
                            sizeof(double) * 16);
                    break;
                }
            }
            

            glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT);

            if (_GetProxyOverlayMode() == _tokens->ghosted) {
                glEnable(GL_LIGHT0);
                float f = 0.1;
                float params[4] = { f, 0.0, f, 1.0 };
                glLightfv(GL_LIGHT0, GL_AMBIENT, params);
            }

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadMatrixd(_viewMatrix.GetArray());
            _renderer->SetLightingStateFromOpenGL();
            glPopMatrix();

            glPopAttrib();

            // The multi-threaded Usd Op may be loading or unloading models on
            // the stage we need, so we grab the global lock in reader mode
            // before rendering.
            boost::shared_lock<boost::upgrade_mutex> 
                                            readerLock(UsdKatanaGetStageLock());
            
            _renderer->Render(_prim, _params);
        }

        // Restore old shader
        glUseProgram(oldProgram);
    }
}

void
USDVMP::cleanup(FnKat::ViewerModifierInput& input)
{
    TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg("%s @ %p : %s\n",
            TF_FUNC_NAME().c_str(), this, input.getFullName().c_str());
}

void
USDVMP::deepCleanup(FnKat::ViewerModifierInput& input)
{
    TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg("%s @ %p : %s\n",
            TF_FUNC_NAME().c_str(), this, input.getFullName().c_str());
}

FnKat::DoubleAttribute
USDVMP::getWorldSpaceBoundingBox(FnKat::ViewerModifierInput& input)
{
    TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg("%s @ %p : %s\n",
            TF_FUNC_NAME().c_str(), this, input.getFullName().c_str());
    return FnKat::DoubleAttribute();
}

void
USDVMP::flush()
{
    // flush all caches
    UsdKatanaCache::GetInstance().Flush();
}

void
USDVMP::_loadSubtreeForCurrentPrim()
{
    if (!_prim) {
        return;
    }

    // Grab an upgrade lock in case we have to write. Only one thread can pass
    // this lock, but it does not block other shared locks.
    boost::upgrade_lock<boost::upgrade_mutex>
                                readerLock(UsdKatanaGetStageLock());

    UsdPrimSiblingRange childrenToLoad = _prim.GetFilteredChildren(
                !UsdPrimIsLoaded && UsdPrimIsActive);

    if(!childrenToLoad.empty()) {
        // We have to compose more of the usd stage, upgrade to a unique lock
        // because loading prims is a write operation.
        boost::upgrade_to_unique_lock<boost::upgrade_mutex>
                                writerLock(readerLock);

        // Queue up all load and unload paths to process at once.
        SdfPathSet toLoad, toUnload;
        TF_FOR_ALL(childIt, childrenToLoad) {
            toLoad.insert(childIt->GetPath());

            TF_DEBUG(KATANA_DEBUG_VMP_USD).Msg(
                        "{USD_VMP} Loading prim: %s\n",
                        childIt->GetPath().GetText());
        }

        _prim.GetStage()->LoadAndUnload(toLoad, toUnload);
    }
}

DEFINE_VMP_PLUGIN(USDVMP)

void registerPlugins()
{
    REGISTER_PLUGIN(USDVMP, "USDVMP", 0, 1);
}

// End
