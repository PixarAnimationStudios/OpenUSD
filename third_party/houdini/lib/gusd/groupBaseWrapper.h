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
#ifndef __GUSD_GROUPBASEWRAPPER_H__
#define __GUSD_GROUPBASEWRAPPER_H__

#include "primWrapper.h"

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class GusdGroupBaseWrapper : public GusdPrimWrapper
{
public:
    GusdGroupBaseWrapper();
    GusdGroupBaseWrapper( 
            const UsdTimeCode &time, 
            const GusdPurposeSet& purposes );
    GusdGroupBaseWrapper( const GusdGroupBaseWrapper& in );
    virtual ~GusdGroupBaseWrapper();

    virtual bool unpack(
        GU_Detail&              gdr,
        const TfToken&          fileName,
        const SdfPath&          primPath,
        const UT_Matrix4D&      xform,
        fpreal                  frame,
        const char *            viewportLod,
        const GusdPurposeSet&   purposes ) override;

protected:
    bool refineGroup( 
            const GusdUSD_StageProxyHandle& stage,
            const UsdPrim&                  prim,
            GT_Refine&                      refiner,
            const GT_RefineParms*           parms=NULL) const;

    bool updateGroupFromGTPrim(
            const UsdGeomImageable&   destPrim,
            const GT_PrimitiveHandle& sourcePrim,
            const UT_Matrix4D&        houXform,
            const GusdContext&        ctxt,
            GusdSimpleXformCache&     xformCache );
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
