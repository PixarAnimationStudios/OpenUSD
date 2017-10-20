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
/**
   \file
   \brief USD Camera.
*/
#ifndef _GUSD_OBJ_USDCAMERA_H_
#define _GUSD_OBJ_USDCAMERA_H_

#include <OBJ/OBJ_Camera.h>
#include <OP/OP_ParmCache.h>
#include <UT/UT_RWLock.h>

#include "gusd/OP_ParmChangeMicroNode.h"

#include <pxr/pxr.h>
#include "pxr/usd/usdGeom/camera.h"

PXR_NAMESPACE_OPEN_SCOPE

/** USD camera object node.
    
    Houdini cameras are evaluated based on a combination of node parameters
    and the object's computed transform. This implementation works by using
    custom local variables in expressions to pull in the corresponding data
    from USD. The reason for doing it this way is that, by default,
    we're able to have everything come from USD, but at any point,
    users are free to delete or modify the default epxressions to change 
    behavior (eg., maybe we want 2x the authored near/far range).
    
    An additional oddity is that the camera parameters come from creation
    scripts. Specifically, see obj/pixar-usdcamera.py, which simply calls
    out to the standard camera startup script. We do this because it means
    we don't have to replicate the parm interface ourselves. It also means
    we're guaranteed to get a default setup that looks just like a regular
    cam. That should be the goal here: a plain old cam that's driven by USD.
    The use of startup scripts is not without precedent; it's exactly
    how the standard camera node works.*/
class GusdOBJ_usdcamera : public OBJ_Camera
{
public:

    enum
    {
        VAR_SCREENASPECT,
        VAR_YRES,
        VAR_PROJECTION,
        VAR_FOCAL,
        VAR_HAPERTURE,
        VAR_VAPERTURE,
        VAR_NEAR,
        VAR_FAR,
        VAR_FOCUS,
        VAR_FSTOP,
        VAR_HAPERTUREOFFSET,
        
        // for backwards compatibility with old stereo attributes {
        VAR_ISSTEREO,
        VAR_CONVERGENCEDISTANCE,
        VAR_INTEROCULARDISTANCE,
        VAR_LEFTEYEBIAS,
        // }
        
        NUM_VARS,
    };

    static OP_TemplatePair* GetTemplates();
    static OP_VariablePair* GetVariables();

    static OP_Node*         creator(OP_Network* net, const char* name,
                                    OP_Operator* op);

    /** Overridden to modify defaults of scripted properties. */
    virtual bool            runCreateScript();

    /** Evaluate the float value of a variable.
        This is where we hook in most of our USD queries. */
    virtual bool            evalVariableValue(fpreal& val,
				    int idx, int thread) override;
    virtual bool            evalVariableValue(UT_String& val,
				    int idx, int thread) override
			    {
				return OP_Network::evalVariableValue(
						   val, idx, thread);
			    }

protected:
    GusdOBJ_usdcamera(OP_Network* net, const char* name, OP_Operator* op);

    virtual ~GusdOBJ_usdcamera() {}

    virtual int             applyInputIndependentTransform(OP_Context& ctx,
                                                           UT_DMatrix4& mx);

    virtual bool            updateParmsFlags();

    virtual void            loadStart();
    virtual void            loadFinished();

    virtual OP_ERROR        cookMyObj(OP_Context& ctx);

private:
    OP_ERROR                _Cook(OP_Context& ctx);

    UsdGeomCamera           _LoadCamera(fpreal t, int thread);

    bool                    _EvalCamVariable(fpreal& val, int idx, int thread);
    
private:
    UT_ErrorManager _errors;
    UsdGeomCamera   _cam;

    int             _frameIdx;  /*! Cached index of the frame parm.*/

    UT_RWLock       _lock;
    bool            _isLoading;

    /** Micro node for tracking changes to the parms that affect our
        camera selection.
        The camera is queried within variable evaluation, so the
        lookup needs to be fast.*/
    GusdOP_ParmChangeMicroNode  _camParmsMicroNode;

public:
    static void             Register(OP_OperatorTable* table);
};

PXR_NAMESPACE_CLOSE_SCOPE


#endif /* _GUSD_OBJ_USDCAMERA_H_ */
