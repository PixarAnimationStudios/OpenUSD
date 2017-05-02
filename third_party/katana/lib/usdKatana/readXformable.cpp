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
#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readPrim.h"
#include "usdKatana/readXformable.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/xform.h"

#include <FnAttribute/FnDataBuilder.h>
#include <FnGeolibServices/FnXFormUtil.h>
#include <FnLogging/FnLogging.h>

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


FnLogSetup("PxrUsdKatanaReadXformable");

void
PxrUsdKatanaReadXformable(
        const UsdGeomXformable& xformable,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    PxrUsdKatanaReadPrim(xformable.GetPrim(), data, attrs);

    //
    // Calculate and set the xform attribute.
    //

    double currentTime = data.GetCurrentTime();

    // Get the ordered xform ops for the prim.
    //
    bool resetsXformStack;
    std::vector<UsdGeomXformOp> orderedXformOps =
        xformable.GetOrderedXformOps(&resetsXformStack);

    FnKat::GroupBuilder gb;

    const bool isMotionBackward = data.IsMotionBackward();

    // For each xform op, construct a matrix containing the 
    // transformation data for each time sample it has.
    //
    int opCount = 0;
    for (std::vector<UsdGeomXformOp>::iterator I = orderedXformOps.begin(); 
            I != orderedXformOps.end(); ++I)
    {
        UsdGeomXformOp& xformOp = (*I);

        const std::vector<double>& motionSampleTimes = 
            data.GetMotionSampleTimes(xformOp.GetAttr());

        FnKat::DoubleBuilder matBuilder(16);
        TF_FOR_ALL(iter, motionSampleTimes)
        {
            double relSampleTime = *iter;
            double time = currentTime + relSampleTime;

            GfMatrix4d mat = xformOp.GetOpTransform(time);

            // Convert to vector.
            const double *matArray = mat.GetArray();
            std::vector<double> &matVec = matBuilder.get(isMotionBackward ?
                PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime) : relSampleTime);

            matVec.resize(16);
            for (int i = 0; i < 16; ++i)
            {
                matVec[i] = matArray[i];
            }
        }

        std::stringstream ss;
        ss << "matrix" << opCount;
        gb.set(ss.str(), matBuilder.build());
        opCount++;
    }

    // Only set an 'xform' attribute if xform ops were found.
    //
    if (!orderedXformOps.empty())
    {
        FnKat::GroupBuilder xformGb;
        xformGb.setGroupInherit(false);

        // Reset the location to the origin if the xform op
        // requires the xform stack to be reset.
        if (resetsXformStack)
        {
            xformGb.set("origin", FnKat::DoubleAttribute(1));
        }

        FnAttribute::DoubleAttribute matrixAttr =
        FnGeolibServices::FnXFormUtil::
            CalcTransformMatrixAtExistingTimes(gb.build()).first;

        xformGb.set("matrix", matrixAttr);

        attrs.set("xform", xformGb.build());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

