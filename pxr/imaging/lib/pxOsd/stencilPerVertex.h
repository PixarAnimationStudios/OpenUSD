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
///
/// \file pxOsd/stencilPerVertex.h

#ifndef PXOSD_STENCILPERVERTEX_H
#define PXOSD_STENCILPERVERTEX_H

#include "pxr/imaging/pxOsd/meshTopology.h"

#include <boost/shared_ptr.hpp>
#include <opensubdiv3/far/stencilTable.h>

class PxOsdStencilPerVertex {
public:    

    // Compute a limit stencil table holding a limit stencil for each
    // of the control vertices.  Similar to ProjectPoints except directly
    // constructs surface locations corresponding to the control vertices,
    // instead of performing numerical closest-point projection.
    //
    static boost::shared_ptr<const OpenSubdiv::Far::LimitStencilTable>
    GetStencilPerVertex(const PxOsdMeshTopology &topology,
                        const int level);
};

#endif 

