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
#ifndef PXRUSDMAYA_PROXYSHAPE_H
#define PXRUSDMAYA_PROXYSHAPE_H

#include "usdMaya/usdPrimProvider.h"

#include <maya/MPxSurfaceShape.h>
#include <maya/MSceneMessage.h>
#include <maya/MBoundingBox.h>
#include <maya/MDataBlock.h>

#include "pxr/base/tf/envSetting.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/path.h"


extern TfEnvSetting<bool> PIXMAYA_ENABLE_BOUNDING_BOX_MODE;


class UsdMayaProxyShape : public MPxSurfaceShape,
    public PxrUsdMayaUsdPrimProvider
{
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
        struct PluginStaticData
        {
            // these get set in initialize()
            MObject filePath;
            MObject primPath;
            MObject excludePrimPaths;
            MObject time;
            MObject variantKey;
            MObject complexity;
            MObject inStageData;
            MObject inStageDataCached;
            MObject fastPlayback;
            MObject tint;
            MObject tintColor;
            MObject outStageData;
            MObject displayGuides;
            MObject displayRenderGuides;

            // this will not change once constructed.
            const MTypeId typeId;
            const MString typeName;
            const MTypeId stageDataTypeId;

            PluginStaticData(
                    const MTypeId& typeId,
                    const MString& typeName,
                    const MTypeId& stageDataTypeId) :
                typeId(typeId),
                typeName(typeName),
                stageDataTypeId(stageDataTypeId)
            { }
        };

        static void* creator(
                const PluginStaticData& psData);

        static MStatus initialize(
                PluginStaticData* psData);

        // Virtual function overrides
        virtual void postConstructor();
        virtual MStatus compute(
            const MPlug& plug,
            MDataBlock& dataBlock);
        virtual bool isBounded() const;
        virtual MBoundingBox boundingBox() const;

        // PxrUsdMayaUsdPrimProvider overrides:
        UsdPrim usdPrim() const override;

        // Public functions
        /**
         * accessor to get the usdprim
         *
         * This method pulls the usdstage data from outData, and will evaluate
         * the dependencies necessary to do so. It should be called instead of
         * pulling on the data directly.
         */
        SdfPathVector getExcludePrimPaths() const;

        int getComplexity() const;
        UsdTimeCode getTime() const;

	bool displayGuides() const;
	bool displayRenderGuides() const;
        
	bool getTint(GfVec4f *outTintColor) const;
        
        bool GetAllRenderAttributes(
            UsdPrim* usdPrimOut,
            SdfPathVector* excludePrimPathsOut,
            int* complexityOut,
            UsdTimeCode* timeOut,
            bool* guidesOut,
            bool* renderGuidesOut,
            bool* tint,
            GfVec4f* tintColor );

        virtual bool setInternalValueInContext(
            const MPlug& plug,
            const MDataHandle& dataHandle,
            MDGContext& ctx);
        
        virtual bool getInternalValueInContext(
            const MPlug& plug,
            MDataHandle& dataHandle,
            MDGContext& ctx);

    protected:
        bool isStageValid() const;

    private:
        const PluginStaticData& _psData;

        UsdMayaProxyShape(const PluginStaticData& psData);

        UsdMayaProxyShape(const UsdMayaProxyShape&);
        ~UsdMayaProxyShape();
        UsdMayaProxyShape& operator=(const UsdMayaProxyShape&);
    
        MStatus computeInStageDataCached(MDataBlock& dataBlock);
        MStatus computeOutStageData(MDataBlock& dataBlock);

        UsdPrim _GetUsdPrim( MDataBlock dataBlock ) const;
        SdfPathVector _GetExcludePrimPaths( MDataBlock dataBlock ) const;
        int _GetComplexity( MDataBlock dataBlock ) const;
        UsdTimeCode _GetTime( MDataBlock dataBlock ) const;
	bool _GetDisplayGuides( MDataBlock dataBlock ) const;
	bool _GetDisplayRenderGuides( MDataBlock dataBlock ) const;
        bool _GetTint( MDataBlock dataBlock, GfVec4f *outTintColor ) const;

        std::map<UsdTimeCode, MBoundingBox> _boundingBoxCache;
        
	bool _useFastPlayback;
};
#endif // PXRUSDMAYA_PROXYSHAPE_H
