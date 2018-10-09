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
#ifndef PXRUSDMAYA_PROXY_SHAPE_H
#define PXRUSDMAYA_PROXY_SHAPE_H

/// \file usdMaya/proxyShape.h

#include "usdMaya/api.h"
#include "usdMaya/stageNoticeListener.h"
#include "usdMaya/usdPrimProvider.h"

#include "pxr/pxr.h"

#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MDGContext.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MSelectionMask.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <map>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_PROXY_SHAPE_TOKENS \
    ((MayaTypeName, "pxrUsdProxyShape"))

TF_DECLARE_PUBLIC_TOKENS(UsdMayaProxyShapeTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_PROXY_SHAPE_TOKENS);


class UsdMayaProxyShape : public MPxSurfaceShape,
                          public UsdMayaUsdPrimProvider
{
    public:
        PXRUSDMAYA_API
        static const MTypeId typeId;
        PXRUSDMAYA_API
        static const MString typeName;

        PXRUSDMAYA_API
        static const MString displayFilterName;
        PXRUSDMAYA_API
        static const MString displayFilterLabel;

        // Attributes
        PXRUSDMAYA_API
        static MObject filePathAttr;
        PXRUSDMAYA_API
        static MObject primPathAttr;
        PXRUSDMAYA_API
        static MObject excludePrimPathsAttr;
        PXRUSDMAYA_API
        static MObject timeAttr;
        PXRUSDMAYA_API
        static MObject variantKeyAttr;
        PXRUSDMAYA_API
        static MObject complexityAttr;
        PXRUSDMAYA_API
        static MObject inStageDataAttr;
        PXRUSDMAYA_API
        static MObject inStageDataCachedAttr;
        PXRUSDMAYA_API
        static MObject fastPlaybackAttr;
        PXRUSDMAYA_API
        static MObject outStageDataAttr;
        PXRUSDMAYA_API
        static MObject displayGuidesAttr;
        PXRUSDMAYA_API
        static MObject displayRenderGuidesAttr;
        PXRUSDMAYA_API
        static MObject softSelectableAttr;

        /// Delegate function for computing the closest point and surface normal
        /// on the proxy shape to a given ray.
        /// The input ray, output point, and output normal should be in the
        /// proxy shape's local space.
        /// Should return true if a point was found, and false otherwise.
        /// (You could just treat this as a ray intersection and return true
        /// if intersected, false if missed.)
        typedef std::function<bool(const UsdMayaProxyShape&, const GfRay&,
                GfVec3d*, GfVec3d*)> ClosestPointDelegate;

        /// Delegate function for returning whether object soft select mode is
        /// currently on
        typedef std::function<bool(void)> ObjectSoftSelectEnabledDelgate;

        PXRUSDMAYA_API
        static void* creator();

        PXRUSDMAYA_API
        static MStatus initialize();

        PXRUSDMAYA_API
        static UsdMayaProxyShape* GetShapeAtDagPath(const MDagPath& dagPath);

        PXRUSDMAYA_API
        static void SetClosestPointDelegate(ClosestPointDelegate delegate);

        PXRUSDMAYA_API
        static void SetObjectSoftSelectEnabledDelegate(
                ObjectSoftSelectEnabledDelgate delegate);

        PXRUSDMAYA_API
        static bool GetObjectSoftSelectEnabled();

        // Virtual function overrides
        PXRUSDMAYA_API
        void postConstructor() override;
        PXRUSDMAYA_API
        MStatus compute(
                const MPlug& plug,
                MDataBlock& dataBlock) override;
        PXRUSDMAYA_API
        bool isBounded() const override;
        PXRUSDMAYA_API
        MBoundingBox boundingBox() const override;
        PXRUSDMAYA_API
        MSelectionMask getShapeSelectionMask() const override;

        PXRUSDMAYA_API
        bool closestPoint(
                const MPoint& raySource,
                const MVector& rayDirection,
                MPoint& theClosestPoint,
                MVector& theClosestNormal,
                bool findClosestOnMiss,
                double tolerance) override;

        PXRUSDMAYA_API
        bool canMakeLive() const override;

        // UsdMayaUsdPrimProvider overrides:
        /**
         * accessor to get the usdprim
         *
         * This method pulls the usdstage data from outData, and will evaluate
         * the dependencies necessary to do so. It should be called instead of
         * pulling on the data directly.
         */
        PXRUSDMAYA_API
        UsdPrim usdPrim() const override;

        // Public functions
        PXRUSDMAYA_API
        SdfPathVector getExcludePrimPaths() const;

        PXRUSDMAYA_API
        int getComplexity() const;
        PXRUSDMAYA_API
        UsdTimeCode getTime() const;

        PXRUSDMAYA_API
        bool displayGuides() const;
        PXRUSDMAYA_API
        bool displayRenderGuides() const;

        PXRUSDMAYA_API
        bool GetAllRenderAttributes(
                UsdPrim* usdPrimOut,
                SdfPathVector* excludePrimPathsOut,
                int* complexityOut,
                UsdTimeCode* timeOut,
                bool* guidesOut,
                bool* renderGuidesOut);

        PXRUSDMAYA_API
        MStatus setDependentsDirty(
                const MPlug& plug,
                MPlugArray& plugArray) override;

        PXRUSDMAYA_API
        bool setInternalValueInContext(
                const MPlug& plug,
                const MDataHandle& dataHandle,
                MDGContext& ctx) override;

        PXRUSDMAYA_API
        bool getInternalValueInContext(
                const MPlug& plug,
                MDataHandle& dataHandle,
                MDGContext& ctx) override;

    protected:
        PXRUSDMAYA_API
        bool isStageValid() const;

    private:
        UsdMayaProxyShape();

        UsdMayaProxyShape(const UsdMayaProxyShape&);
        ~UsdMayaProxyShape() override;
        UsdMayaProxyShape& operator=(const UsdMayaProxyShape&);

        MStatus computeInStageDataCached(MDataBlock& dataBlock);
        MStatus computeOutStageData(MDataBlock& dataBlock);

        UsdPrim _GetUsdPrim(MDataBlock dataBlock) const;
        SdfPathVector _GetExcludePrimPaths(MDataBlock dataBlock) const;
        int _GetComplexity(MDataBlock dataBlock) const;
        UsdTimeCode _GetTime(MDataBlock dataBlock) const;
        bool _GetDisplayGuides(MDataBlock dataBlock) const;
        bool _GetDisplayRenderGuides(MDataBlock dataBlock) const;

        bool _CanBeSoftSelected() const;

        void _OnStageContentsChanged(
                const UsdNotice::StageContentsChanged& notice);

        UsdMayaStageNoticeListener _stageNoticeListener;

        std::map<UsdTimeCode, MBoundingBox> _boundingBoxCache;

        bool _useFastPlayback;

        static ClosestPointDelegate _sharedClosestPointDelegate;
        static ObjectSoftSelectEnabledDelgate
            _sharedObjectSoftSelectEnabledDelgate;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
