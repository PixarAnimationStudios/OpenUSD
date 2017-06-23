//
// Copyright 2017 Pixar
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
#include "OBJ_usdcamera.h"

#include <CH/CH_LocalVariable.h>
#include <CH/CH_Manager.h>
#include <DEP/DEP_MicroNode.h>
#include <OP/OP_Director.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Value.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_Parm.h>
#include <UT/UT_ThreadSpecificValue.h>
#include <UT/UT_Version.h>

#include "gusd/PRM_Shared.h"
#include "gusd/USD_StageCache.h"
#include "gusd/UT_Assert.h"
#include "gusd/UT_Gf.h"
#include "gusd/UT_Usd.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdUtils/pipeline.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef enum
{
    _POSTMULTCTM_TRANSFORM,
    _CTM_TRANSFORM,
    _OBJ_TRANSFORM,
    _IGNORE_TRANSFORM
} _TransformMode;


void
GusdOBJ_usdcamera::Register(OP_OperatorTable* table)
{
    OP_Operator* op = new OP_Operator("pixar::usdcamera",
                                      "USD Camera",
                                      creator,
                                      GetTemplates(),
#if UT_MAJOR_VERSION_INT >= 16
                                      SOP_TABLE_NAME,
#endif
                                      /* min inputs*/ 0,
                                      /* max inputs*/ 1,
                                      GetVariables());
    op->setIconName("pxh_gusdIcon.png");
    op->setOpTabSubMenuPath( "Pixar" );
    table->addOperator(op);
    table->setOpFirstName("pixar::usdcamera", "usdcam");
}


OP_VariablePair*
GusdOBJ_usdcamera::GetVariables()
{
    static CH_LocalVariable vars[] = {
        {"SCREENASPECT", VAR_SCREENASPECT, CH_VARIABLE_TIME},
        {"YRES", VAR_YRES, CH_VARIABLE_TIME},
        {"PROJECTION", VAR_PROJECTION, CH_VARIABLE_TIME},
        {"FOCAL", VAR_FOCAL, CH_VARIABLE_TIME},
        {"HAPERTURE", VAR_HAPERTURE, CH_VARIABLE_TIME},
        {"VAPERTURE", VAR_VAPERTURE, CH_VARIABLE_TIME},
        {"NEAR", VAR_NEAR, CH_VARIABLE_TIME},
        {"FAR", VAR_FAR, CH_VARIABLE_TIME},
        {"FOCUS", VAR_FOCUS, CH_VARIABLE_TIME},
        {"FSTOP", VAR_FSTOP, CH_VARIABLE_TIME},
        {"HAPERTUREOFFSET", VAR_HAPERTUREOFFSET, CH_VARIABLE_TIME},

        // for backwards compatibility with old stereo attributes {
        {"ISSTEREO", VAR_ISSTEREO, CH_VARIABLE_TIME},
        {"CONVERGENCEDISTANCE", VAR_CONVERGENCEDISTANCE, CH_VARIABLE_TIME},
        {"INTEROCULARDISTANCE", VAR_INTEROCULARDISTANCE, CH_VARIABLE_TIME},
        {"LEFTEYEBIAS", VAR_LEFTEYEBIAS, CH_VARIABLE_TIME},
        // }

        {0, 0, 0}
    };
    static OP_VariablePair oldVarPair(ourLocalVariables, NULL);
    static OP_VariablePair varPair(vars, &oldVarPair);
    return &varPair;
};


OP_TemplatePair*
GusdOBJ_usdcamera::GetTemplates()
{
    /* Our common camera params come from the same initialization script
       as the standard camera.*/

    static PRM_Default primPathDef(0, "/World/main_cam");
    static PRM_Name frameName("frame", "Frame");
    static PRM_Default frameDef(0, "$FF");
    
    static PRM_Name xformName("xformmode", "Transform Mode");
    static PRM_Name xformNames[] = {
        PRM_Name("postmultctm", "Object to World"),
        PRM_Name("ctm", "Parent to World"),
        PRM_Name("obj", "Object"),
        PRM_Name("none", "None"),
        PRM_Name()
    };
    static PRM_ChoiceList xformMenu(PRM_CHOICELIST_SINGLE, xformNames);

    GusdPRM_Shared prmShared;

    static PRM_Template camTemplates[] = {
        PRM_Template(PRM_FILE, 1, &prmShared->filePathName, 0,
                     /*choicelist*/ 0, /*range*/ 0,
                     /*callback*/ 0, &prmShared->usdFileROData),
        PRM_Template(PRM_STRING, 1, &prmShared->primPathName,
                     &primPathDef),
        PRM_Template(PRM_FLT, 1, &frameName, &frameDef),
        PRM_Template(PRM_ORD, 1, &xformName, 
                     /* default*/ 0, 
                     /* choice list */ &xformMenu,
                     /* range */0,
                     /* callback */0,
                     /* spare */0,
                     /* group */0,
                     "If this node is included in a OBJ hierarchy this "
                     "should be set to 'Object'. If not, it should "
                     "be set to 'Object to World'"),
        PRM_Template()
    };
    unsigned numCamTemplates = PRM_Template::countTemplates(camTemplates);

    // for backwards compatibility with old stereo attributes {
    static PRM_Name isStereoName("isstereo", "Is Stereo");
    static PRM_Name rightEyeName( "isrighteye", "Is Right Eye" );
    static PRM_Name convergenceDistanceName("convergencedistance",
                                            "Convergence Distance");
    static PRM_Name interocularDistanceName("interoculardistance",
                                            "Interocular Distance");
    static PRM_Name leftEyeBiasName("lefteyebias", "Left Eye Bias");

    static PRM_Template stereoAttrsTemplates[] = {
        PRM_Template(PRM_TOGGLE | PRM_TYPE_INVISIBLE, 1, &isStereoName,
                     /* defaults */ 0,
                     /* choice list */ 0,
                     /* range ptr */0,
                     /* callback */0,
                     /* spare */ 0,
                     /* parmgroup*/ 1,
                     "Show mono view if off. Right or left eye view if on."),
        PRM_Template(PRM_TOGGLE | PRM_TYPE_INVISIBLE, 1, &rightEyeName,
                     /* defaults */ 0,
                     /* choice list */ 0,
                     /* range ptr */0,
                     /* callback */0,
                     /* spare */ 0,
                     /* parmgroup*/ 1,
                     "If checked, show right eye view. "
                     "Otherwise show left eye view."),
        PRM_Template(PRM_FLT | PRM_TYPE_INVISIBLE, 1, &convergenceDistanceName, 0),
        PRM_Template(PRM_FLT | PRM_TYPE_INVISIBLE, 1, &interocularDistanceName, 0),
        PRM_Template(PRM_FLT | PRM_TYPE_INVISIBLE, 1, &leftEyeBiasName,
                     /* defaults */ 0,
                     /* choice list */ 0,
                     /* range ptr */0,
                     /* callback */0,
                     /* spare */ 0,
                     /* parmgroup*/ 1,
                     "If 0, left eye view matches mono view. "
                     "If 1, right eye view matches mono view." ),
        PRM_Template()
    };
    
    unsigned numStereoAttrsTemplates = 
        PRM_Template::countTemplates(stereoAttrsTemplates);
    // }

    static PRM_Name displayFrustumName( "displayFrustum", "Display Frustum");
    static PRM_Template displayFrustum(PRM_TOGGLE, 1, &displayFrustumName, 0);

    const PRM_Template* const objTemplates = getTemplateList(OBJ_PARMS_PLAIN);
    unsigned numObjTemplates = PRM_Template::countTemplates(objTemplates);

    /* First template in common obj parms is a switcher.
       We need to modify the switcher to include our own tab.*/
    
    UT_IntArray numSwitchersOnPages, numNonSwitchersOnPages;
    PRM_Template::getSwitcherStats(objTemplates,
                                   numSwitchersOnPages,
                                   numNonSwitchersOnPages);
    unsigned oldSwitcherSize = numNonSwitchersOnPages.entries();
    
    static std::vector<PRM_Default> tabs(oldSwitcherSize+1);
    
    static PRM_Default renderPaneDefault;
    
    for(int i = 0; i < oldSwitcherSize; ++i)
    {
        tabs[i] = objTemplates->getFactoryDefaults()[i];
        
        // We want to add an item to the Render pane, displayFrustum.
        // We need to increase the item count in the switcher default.
        if(UT_String(tabs[i].getString()) == "Render")
        {
            renderPaneDefault.setString(tabs[i].getString());
            renderPaneDefault.setFloat(tabs[i].getFloat() + 1);
            tabs[i] = renderPaneDefault;
        }
    }
    tabs[oldSwitcherSize] = 
        PRM_Default(numCamTemplates + numStereoAttrsTemplates, "USD");

    static PRM_Name switcherName = *objTemplates->getNamePtr();

    static std::vector<PRM_Template> templates;
    templates.push_back(
        PRM_Template(PRM_SWITCHER, tabs.size(), &switcherName, &tabs[0]));
    
    for(std::size_t i = 1; i < numObjTemplates; ++i) 
    {
        templates.push_back( objTemplates[i] );
        if(UT_String(objTemplates[i].getNamePtr()->getToken()) == "display")
            templates.push_back(displayFrustum);
    }
    templates.insert(templates.end(), camTemplates,
                     camTemplates + numCamTemplates);
    templates.insert(templates.end(), stereoAttrsTemplates,
                     stereoAttrsTemplates + numStereoAttrsTemplates);
    templates.push_back(PRM_Template());

    static OP_TemplatePair templatePair(&templates[0], NULL);
    return &templatePair;
}


bool
GusdOBJ_usdcamera::updateParmsFlags()
{
    int changed = 0;

    return changed;
}


void
GusdOBJ_usdcamera::loadStart()
{
    _isLoading = true;
    OBJ_Camera::loadStart();
}


void
GusdOBJ_usdcamera::loadFinished()
{
    OBJ_Camera::loadFinished();
    _isLoading = false;
}


OP_Node*
GusdOBJ_usdcamera::creator(OP_Network *net, const char* name, OP_Operator* op)
{
    return new GusdOBJ_usdcamera(net, name, op);
}


GusdOBJ_usdcamera::GusdOBJ_usdcamera(OP_Network* net, const char* name, OP_Operator* op)
    : OBJ_Camera(net, name, op), _isLoading(false), _camParmsMicroNode(*this)
{
    GusdPRM_Shared prmShared;

    const PRM_ParmList* parms = GusdUTverify_ptr(getParmList());

    _camParmsMicroNode.addParm(
        parms->getParmIndex(prmShared->filePathName.getToken()));
    _camParmsMicroNode.addParm(
        parms->getParmIndex(prmShared->primPathName.getToken()));

    _frameIdx = parms->getParmIndex("frame");
    UT_ASSERT(_frameIdx >= 0);
}


struct _NameDefaultPair
{
    const char* name;
    int         vi;
    PRM_Default def;
};


bool
GusdOBJ_usdcamera::runCreateScript()
{
    static _NameDefaultPair pairs[] = {
        {"iconscale", 0, PRM_Default(500,"")},
        {"res", 0, PRM_Default(1920, "")},
        {"res", 1, PRM_Default(803, "$YRES")},
        {"projection", 0, PRM_Default(0, "$PROJECTION")},
        {"focal", 0, PRM_Default(50, "$FOCAL")},
        {"orthowidth", 0, PRM_Default(1000, "$HAPERTURE * .1")},
        {"aperture", 0, PRM_Default(41.2136, "$HAPERTURE")},
        {"near", 0, PRM_Default(0.001, "$NEAR")},
        {"far", 0, PRM_Default(10000, "$FAR")},
        {"focus", 0, PRM_Default(5, "$FOCUS")},
        {"fstop", 0, PRM_Default(5.6, "$FSTOP")},
        {"win", 0, PRM_Default(0, "$HAPERTUREOFFSET/$HAPERTURE")},

        // for backwards compatibility with old stereo attributes {
        {"isstereo", 0, PRM_Default(0, "$ISSTEREO")},
        {"convergencedistance", 0, PRM_Default(1000, "$CONVERGENCEDISTANCE")},
        {"interoculardistance", 0, PRM_Default(50, "$INTEROCULARDISTANCE")},
        {"lefteyebias", 0, PRM_Default(0.0, "$LEFTEYEBIAS")},
        // }

        {NULL, 0, PRM_Default()}
    };

    if(OBJ_Camera::runCreateScript()) /* run the obj/GusdOBJ_usdcamera.cmd setup
                                         script. This gives us our common
                                         camera and usd properties. */
    {
        /* Loop over relevant camera properties and setup new defaults.
           We want cam parms to use expressions by default. We have variable
           expansions reference the relevant usd properties.
           The point of this design is that it allows users the ability to
           override any of the properties coming from usd, wrap expressions
           around them, etc.
        */
        PRM_ParmList* parms = GusdUTverify_ptr(getParmList());
        for(_NameDefaultPair* pair = pairs; pair->name; pair++)
        {
            if(PRM_Parm* parm = parms->getParmPtr(pair->name))
            {
                if(PRM_Template* tmpl = parm->getTemplatePtr())
                {
                    tmpl->setDefault(pair->vi, pair->def);
                    parm->revertToDefaults(0);
                }
            }
        }
        
#if 0
        // Replace the choices for the resolution menu with ones that 
        // we read from our configuration script.
        if(PRM_Parm *parm = parms->getParmPtr("resMenu"))
        {
            if(PRM_Template *tmpl = parm->getTemplatePtr())
            {
                PRM_ChoiceList *myChoices = 
                    new PRM_ChoiceList( 
                        PRM_CHOICELIST_SINGLE, 
                        "__import__('pxh_config')."
                        "GetResolutionChoiceList(False)",
                        CH_PYTHON_SCRIPT);
                tmpl->setChoiceListPtr(myChoices);
            }

        }
#endif
        return true;
    }
    return false;
}


namespace {


/** Simple index stack.*/
struct _VarEvalStack
{
    _VarEvalStack()
        : _stack(4) /* very unlikely that a larger stack is needed! */
        {}
    
    int     Last() const    { return _stack.isEmpty() ? -1 : _stack.last(); }
    
    void    Push(int idx)   { _stack.append(idx); }
    
    void    Pop()           {
                                UT_ASSERT_P(_stack.size() > 0);
                                _stack.setSize(_stack.size()-1);
                            }
    
private:
    UT_Array<int>   _stack;
};

UT_ThreadSpecificValue<_VarEvalStack*> _varEvalStack;


} /*namespace*/


bool
GusdOBJ_usdcamera::evalVariableValue(fpreal& val, int idx, int thread)
{
    if(idx >= NUM_VARS)
        return OBJ_Node::evalVariableValue(val, idx, thread);

    /* Protect against cyclic variable referencing when eval'ing cam vars.
       Most vars eval the 'frame' parm in order to pull animated data
       from USD. If someone were to put, say, $YRES on the frame number,
       then evaluating the frame would require evaluation of the YRES
       variable, which requires evaluation of the frame parm...and we're
       stuck in a loop.*/
    _VarEvalStack*& stack = _varEvalStack.getValueForThread(thread);
    if(BOOST_UNLIKELY(!stack)) stack = new _VarEvalStack;
    
    if(stack->Last() != idx) {
        stack->Push(idx);
        bool res = _EvalCamVariable(val, idx, thread);
        stack->Pop();
        return res;
    }
    return false;
}


bool
GusdOBJ_usdcamera::_EvalCamVariable(fpreal& val, int idx, int thread)
{
    UT_ASSERT_P(idx >= 0 && idx < NUM_VARS);
    
    const fpreal t = CHgetEvalTime(thread);
    
    _CamHolder::ScopedLock lock;
    if(UsdGeomCamera cam = _LoadCamera(lock, t, thread))
    {
        float frame = evalFloatT(_frameIdx, 0, t, thread);
        switch(idx)
        {
        case VAR_SCREENASPECT:
            {
                val = cam.GetCamera(frame).GetAspectRatio();
                return true;
            }
        case VAR_YRES:
            {
                // XXX This is redundant since resy can be set to
                //     "ch(\"resx\")/$SCREENASPECT" in runCreateScript,
                //     however it's needed to get around a Houdini bug
                //     (see bug 94389)
                const float screenAspect =
                    cam.GetCamera(frame).GetAspectRatio();
                float xRes = evalFloatT("res", 0, t, thread);
                val = xRes / screenAspect;
                return true;
            }
        case VAR_PROJECTION:
            {
                TfToken proj;
                if(cam.GetProjectionAttr().Get(&proj) &&
                   proj == UsdGeomTokens->orthographic)
                    val = OBJ_PROJ_ORTHO;
                else
                    val = OBJ_PROJ_PERSPECTIVE;
                return true;
            }
        case VAR_FOCAL:
            {
                float focal = 50;
                cam.GetFocalLengthAttr().Get(&focal, frame);
                val = focal;
                return true;
            }
        case VAR_HAPERTURE:
            {
                float aperture = 41.2136;
                cam.GetHorizontalApertureAttr().Get(&aperture, frame);
                val = aperture;
                return true;
            }
        case VAR_VAPERTURE:
            {
                float aperture = 41.2136;
                cam.GetVerticalApertureAttr().Get(&aperture, frame);
                val = aperture;
                return true;
            }
        case VAR_NEAR:
            {
                GfVec2f clipping;
                val = cam.GetClippingRangeAttr().Get(&clipping, frame) ?
                    clipping[0] : 0.001;
                return true;
            }
        case VAR_FAR:
            {
                GfVec2f clipping;
                val = cam.GetClippingRangeAttr().Get(&clipping, frame) ?
                    clipping[1] : 10000;
                return true;
            }
        case VAR_FOCUS:
            {
                float focus = 5;
                cam.GetFocusDistanceAttr().Get(&focus, frame);
                val = focus;
                return true;
            }
        case VAR_FSTOP:
            {
                float fstop = 5.6;
                cam.GetFStopAttr().Get(&fstop, frame);
                val = fstop;
                return true;
            }
        case VAR_HAPERTUREOFFSET:
            {
                float apertureOffset = 41.2136;
                cam.GetHorizontalApertureOffsetAttr().Get(&apertureOffset, frame);
                val = apertureOffset;
                return true;
            }
        // for backwards compatibility with old stereo attributes {

        // XXX:-matthias
        // We are just writing out dummy values so that old assets do not
        // break.
        // The following code together with the definitions of
        // VAR_ISSTEREO, ... should eventually be removed.

        case VAR_ISSTEREO:
            {
                bool isStereo = false;
                val = isStereo;
                return true;
            }
        case VAR_CONVERGENCEDISTANCE:
            {
                float convergenceDistance = 1000;
                val = convergenceDistance;
                return true;
            }
        case VAR_INTEROCULARDISTANCE:
            {
                float interocularDistance = 50;
                val = interocularDistance;
                return true;
            }
        case VAR_LEFTEYEBIAS:
            {
                float leftEyeBias = 0.0;
                val = leftEyeBias;
                return true;
            }
        // }
        };
    }

    /* Couldn't load a camera. Just return defaults.*/
    switch(idx)
    {
    case VAR_SCREENASPECT:          val = 1.0; break;
    case VAR_YRES:                  val = 1080; break;
    case VAR_PROJECTION:            val = OBJ_PROJ_PERSPECTIVE; break;
    case VAR_FOCAL:                 val = 50; break;
    case VAR_HAPERTURE:             val = 41.2136; break;
    case VAR_VAPERTURE:             val = 41.2136; break;
    case VAR_NEAR:                  val = 0.001; break;
    case VAR_FAR:                   val = 10000; break;
    case VAR_FOCUS:                 val = 5; break;
    case VAR_FSTOP:                 val = 5.6; break;
    case VAR_HAPERTUREOFFSET:       val = 41.2136;
    // for backwards compatibility with old stereo attributes
    case VAR_ISSTEREO:              val = 0; break;
    case VAR_CONVERGENCEDISTANCE:   val = 1000; break;
    case VAR_INTEROCULARDISTANCE:   val = 50; break;
    case VAR_LEFTEYEBIAS:           val = 0.0; break;
    };
    /* Return true so that the variables are still considered valid,
       even in the absence of the camera. This is done to prevent
       evaluation errors during saves.*/
    return true;
}


int
GusdOBJ_usdcamera::applyInputIndependentTransform(OP_Context& ctx, UT_DMatrix4& mx)
{
    mx.identity();
    fpreal t = ctx.getTime();

    _CamHolder::ScopedLock lock;
    if(UsdGeomCamera cam = _LoadCamera(lock, t, ctx.getThread()))
    {
        float frame = evalFloat(_frameIdx, 0, t);

        GfMatrix4d ctm(1.);
        bool stat = true;
        bool resetsXformStack = false;

        switch(evalInt("xformmode", 0, t))
        {
        case _POSTMULTCTM_TRANSFORM:
            stat = true;
            ctm = cam.ComputeLocalToWorldTransform(frame);
            break;
        case _CTM_TRANSFORM:
            stat = true;
            ctm = cam.ComputeParentToWorldTransform(frame);
            break;
        case _OBJ_TRANSFORM:
            // XXX: how do we reset xformStack here?
            // Is that (or should that
            // be) handled by the Compute calls above?
            stat = cam.GetLocalTransformation(&ctm, &resetsXformStack, frame);
            break;
        default: // _IGNORE_TRANSFORM:
            stat = true;
            ctm.SetIdentity();
            break;
        }
        if(!stat)
        {
            stealErrors(_errors, /*borrow*/ true);
            return 0;
        }

        mx = GusdUT_Gf::Cast(ctm);
    }
    return OBJ_Camera::applyInputIndependentTransform(ctx, mx);
}


OP_ERROR
GusdOBJ_usdcamera::_Cook(OP_Context& ctx)
{
    _CamHolder::ScopedLock lock;
    if(_LoadCamera(lock, ctx.getTime(), ctx.getThread())) {
        // Don't need to keep a lock to the prim to report errors.
        lock.Release();
    }
    
    /* XXX: There's a potential race condition here, between
            loading and stealing cached errors.
            Would be better to keep the camera cache locked
            until the error stealing is done.*/
    
    UT_AutoReadLock readLock(_lock);
    stealErrors(_errors, /*borrow*/ true);
    return error();
}


OP_ERROR
GusdOBJ_usdcamera::cookMyObj(OP_Context& ctx)
{
    if(_Cook(ctx) <= UT_ERROR_ABORT)
        OBJ_Camera::cookMyObj(ctx);
    return error();
}


UsdGeomCamera
GusdOBJ_usdcamera::_LoadCamera(_CamHolder::ScopedLock& lock,
                           fpreal t, int thread)
{
    /* XXX: Disallow camera loading until the scene has finished loading.
       What happens otherwise is that some parm values are pulled on
       during loading, causing a _LoadCamera request. If this happens before
       the node's parm values have been loaded, then we'll end up loading
       the camera using defaults (which reference the shot conversion).
       So if we don't block this, we end up always loading the shot conversion,
       even if we don't need it! */

    if(_isLoading)
        return UsdGeomCamera();

    /* Always return a null camera while saving.
       This is to prevent load errors from prematurely interrupting saves,
       which can lead to corrupt files.*/
    if(GusdUTverify_ptr(OPgetDirector())->getIsDoingExplicitSave())
        return UsdGeomCamera();

    {
        UT_AutoReadLock readLock(_lock);
        if(!_camParmsMicroNode.requiresUpdate(t)) {
            if(_cam) {
                lock.Acquire(_cam, /*write*/ false);
                return *lock;
            }
            return UsdGeomCamera();
        }
    }

    UT_AutoWriteLock writeLock(_lock);

    /* Other thread may already have loaded the cam, so only update if needed.*/
    if(_camParmsMicroNode.updateIfNeeded(t, thread))
    {
        _errors.clearAndDestroyErrors();
        _cam.Clear();

        GusdPRM_Shared prmShared;
        UT_String usdPath, primPath;

        evalStringT(usdPath, prmShared->filePathName.getToken(), 0, t, thread);
        evalStringT(primPath, prmShared->primPathName.getToken(), 0, t, thread);

        GusdUT_ErrorManager errMgr(_errors);
        GusdUT_ErrorContext err(errMgr);

        std::string errStr;

        GusdUSD_StageCacheContext cache;
        if(auto proxy = cache.FindOrCreateProxy(TfToken(usdPath.toStdString())))
        {
            // Add dependency on the proxy (I.e., listen for reloads)
            _camParmsMicroNode.addExplicitInput(proxy->GetMicroNode());
            
            GusdUSD_StageProxy::Accessor accessor;
            GusdUSD_Utils::PrimIdentifier primIdentifier;
            if(primIdentifier.SetFromVariantPath(primPath, &err) &&
               cache.Bind(accessor, proxy, primIdentifier, &err)) {
                _cam = accessor.GetPrimSchemaHolderAtPath<UsdGeomCamera>(
                    *primIdentifier, &err);
                if(_cam) {
                    /* Acquire a read-lock on the lock passed in as input.
                       This ensures the caller maintains a lock beyond  
                       the scope of the load.*/
                    lock.Acquire(_cam, /*write*/ false);
                    return *lock;
                }
            }
        }
    }
    return UsdGeomCamera();
}

PXR_NAMESPACE_CLOSE_SCOPE

