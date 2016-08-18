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
#ifndef PXRUSDMAYA_STAGEDATA_H
#define PXRUSDMAYA_STAGEDATA_H

#include "usdMaya/api.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MPxGeometryData.h>
#include <maya/MTypeId.h>
#include <maya/MSceneMessage.h>
#include <maya/MString.h>

class UsdMayaStageData : public MPxGeometryData {

    public:

        /// \brief Helper struct to hold MObjects for this class.
        ///
        /// These would normally be static members but since we have this class
        /// registered in multiple plugins, we have the actual data stored
        /// statically in the plugin.cpp.  
        /// 
        /// A reference to this is setup by creator().
        ///
        /// \sa PxrUsdMayaPluginStaticData
        struct PluginStaticData {
            const MTypeId typeId;
            const MString typeName;

            PluginStaticData(
                    const MTypeId& typeId, 
                    const MString& typeName) :
                typeId(typeId),
                typeName(typeName)
            { }
        };

        /**
         * creator method
         */
        USDMAYA_API
        static void* creator(
                const PluginStaticData& _psData);

        /**
         * \name virtual overrides
         */
        //@{

        void copy(
            const MPxData& aDatum);

        MTypeId typeId() const;

        MString name() const;
        //@}

        void registerExitCallback();

        void unregisterExitCallback();

        /**
         * \name data
         */
        //@{

        UsdStageRefPtr stage;
        SdfPath primPath;

        //@}

    protected:
    private:

        UsdMayaStageData(const PluginStaticData& psData);
        ~UsdMayaStageData();

        UsdMayaStageData(
            const UsdMayaStageData&);

        UsdMayaStageData& operator=(const UsdMayaStageData&);

        const PluginStaticData& _psData;

        MCallbackId _exitCallbackId;
};
#endif // PXRUSDMAYA_STAGEDATA_H
