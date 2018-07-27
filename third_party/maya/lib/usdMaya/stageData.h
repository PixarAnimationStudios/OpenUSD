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
#ifndef PXRUSDMAYA_STAGE_DATA_H
#define PXRUSDMAYA_STAGE_DATA_H

/// \file usdMaya/stageData.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MPxData.h>
#include <maya/MPxGeometryData.h>
#include <maya/MMessage.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_STAGE_DATA_TOKENS \
    ((MayaTypeName, "pxrUsdStageData"))

TF_DECLARE_PUBLIC_TOKENS(UsdMayaStageDataTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_STAGE_DATA_TOKENS);


class UsdMayaStageData : public MPxGeometryData
{
    public:
        /// Unlike other Maya node types, MPxData/MPxGeometryData declare
        /// typeId() as a pure virtual method that must be overridden in
        /// derived classes, so we have to call this static member "mayaTypeId"
        /// instead of just "typeId" as we usually would.
        PXRUSDMAYA_API
        static const MTypeId mayaTypeId;
        PXRUSDMAYA_API
        static const MString typeName;

        PXRUSDMAYA_API
        static void* creator();

        /**
         * \name MPxGeometryData overrides
         */
        //@{

        PXRUSDMAYA_API
        void copy(const MPxData& src) override;

        PXRUSDMAYA_API
        MTypeId typeId() const override;

        PXRUSDMAYA_API
        MString name() const override;
        //@}

        PXRUSDMAYA_API
        void registerExitCallback();

        PXRUSDMAYA_API
        void unregisterExitCallback();

        /**
         * \name data
         */
        //@{

        UsdStageRefPtr stage;
        SdfPath primPath;

        //@}

    private:
        UsdMayaStageData();
        ~UsdMayaStageData() override;

        UsdMayaStageData(const UsdMayaStageData&);
        UsdMayaStageData& operator=(const UsdMayaStageData&);

        MCallbackId _exitCallbackId;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
