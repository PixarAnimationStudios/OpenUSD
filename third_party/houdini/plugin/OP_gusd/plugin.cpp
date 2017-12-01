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
#include "ROP_usdoutput.h"
#include "SOP_usdimport.h"
#include "SOP_usdunpack.h"
#include "OBJ_usdcamera.h"

#include "pxr/base/arch/export.h"
#include "gusd/gusd.h"

#include <SYS/SYS_Version.h>
#include <UT/UT_DSOVersion.h>

using std::cerr;
using std::endl;

ARCH_EXPORT
void 
newDriverOperator(OP_OperatorTable* operators) 
{
    PXR_NS::GusdInit();
    PXR_NS::GusdROP_usdoutput::Register(operators);
}

ARCH_EXPORT
void 
newSopOperator( OP_OperatorTable* operators) 
{
    PXR_NS::GusdInit();
    PXR_NS::GusdSOP_usdimport::Register(operators);
    PXR_NS::GusdSOP_usdunpack::Register(operators);
}

ARCH_EXPORT
void 
newObjectOperator(OP_OperatorTable *operators)
{
    PXR_NS::GusdInit();
    PXR_NS::GusdOBJ_usdcamera::Register(operators);
}

ARCH_EXPORT
void
newGeometryPrim( GA_PrimitiveFactory *f ) 
{
    PXR_NS::GusdInit();
    PXR_NS::GusdNewGeometryPrim( f );
}

ARCH_EXPORT
void
newGeometryIO( void * )
{
    PXR_NS::GusdInit();
    PXR_NS::GusdNewGeometryIO();
}

