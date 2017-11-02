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
#ifndef __GUSD_INSTANCERWRAPPER_H__
#define __GUSD_INSTANCERWRAPPER_H__

#include "pxr/pxr.h"
#include "primWrapper.h"

#include <UT/UT_Map.h>
#include <UT/UT_StringHolder.h>

#include <pxr/usd/usdGeom/pointInstancer.h>

PXR_NAMESPACE_OPEN_SCOPE


class GusdInstancerWrapper : public GusdPrimWrapper
{
    typedef UT_Map<TfToken, int>                    RelationshipIndexMap;

public:

    GusdInstancerWrapper( const UsdStagePtr& stage,
                          const SdfPath& path,
                          const GusdContext &ctxt,
                          bool isOverride = false );
    GusdInstancerWrapper( const UsdGeomPointInstancer&  usdInstancer, 
                          UsdTimeCode                   t,
                          GusdPurposeSet                purposes );  
    virtual ~GusdInstancerWrapper();

    virtual const UsdGeomImageable getUsdPrimForWrite() const override { return m_usdPointInstancerForWrite; }

    virtual const UsdGeomImageable getUsdPrimForRead() const override {
        return m_usdPointInstancerForRead;
    }

    virtual bool redefine( 
           const UsdStagePtr& stage,
           const SdfPath& path,
           const GusdContext& ctxt,
           const GT_PrimitiveHandle& sourcePrim ) override;

    virtual bool getUniqueID(int64& id) const override;
    
    virtual const char* className() const override;

    virtual void enlargeBounds(UT_BoundingBox boxes[], int nsegments) const override;

    virtual int getMotionSegments() const override;

    virtual int64 getMemoryUsage() const override;

    virtual GT_PrimitiveHandle doSoftCopy() const override;

    virtual bool
    updateFromGTPrim(const GT_PrimitiveHandle& sourcePrim,
                     const UT_Matrix4D&        houXform,
                     const GusdContext&        ctxt,
                     GusdSimpleXformCache&     xformCache ) override;

    virtual bool isValid() const override;

    virtual bool refine( GT_Refine& refiner,
                         const GT_RefineParms* parms = NULL ) const override;

    virtual bool unpack(
        GU_Detail&              gdr,
        const UT_StringRef&     fileName,
        const SdfPath&          primPath,
        const UT_Matrix4D&      xform,
        fpreal                  frame,
        const char *            viewportLod,
        GusdPurposeSet          purposes ) override;

public:

    static GT_PrimitiveHandle
    defineForWrite( const GT_PrimitiveHandle& sourcePrim,
                    const UsdStagePtr& stage,
                    const SdfPath& path,
                    const GusdContext& ctxt);

    static GT_PrimitiveHandle
    defineForRead( const UsdGeomImageable&  sourcePrim, 
                   UsdTimeCode              time,
                   GusdPurposeSet           purposes );

private:
    bool initUsdPrim(const UsdStagePtr& stage,
                     const SdfPath& path,
                     bool asOverride);

    void writePrototypes( const GusdContext& ctxt, 
                          const UsdStagePtr& stage,
                          const GT_PrimitiveHandle& sourcePrim);

    void storePreOverlayData(bool justProtoIndices, const UsdTimeCode* time);

    void clearPreOverlayData();

    void setTransformAttrsFromMatrices(const UT_Matrix4D &worldToLocal,
                                   const GT_AttributeListHandle gtAttrs,
                                   GusdContext ctxt,
                                   GT_PrimitiveHandle sourcePrim);

private:
    UsdGeomPointInstancer m_usdPointInstancerForRead,
                          m_usdPointInstancerForWrite;

    // A map of tokens to indexes in the point instancer's relationship array.
    // The tokens could be unique ids built from USD packed prims or
    // paths to SOP nodes (as in instancepath attributes).
    RelationshipIndexMap    m_relationshipIndexMap;

    // List of prototype transforms for "subtracting" from final instance
    // transforms.
    std::vector<UT_Matrix4D> m_prototypeTransforms;

    // Scope to write prototypes to (usually ../Prototypes/..).
    SdfPath m_prototypesScope;

    // List of attributes to write for point instancer.
    std::vector<TfToken> m_usdGeomTokens = {UsdGeomTokens->protoIndices,
                                            UsdGeomTokens->positions,
                                            UsdGeomTokens->orientations,
                                            UsdGeomTokens->scales,
                                            UsdGeomTokens->velocities,
                                            UsdGeomTokens->angularVelocities};

    // Struct for helping store original data from a point instancer we are
    // overlaying, so we can partially overlay a subset of points while
    // writing out the original data for the others in an overlay transform.
    template <class T>
    struct PreOverlayDataEntry {
        std::map<UsdTimeCode, VtArray<T>> preOverlayDataMap;
        UsdAttribute usdAttr;
        
        PreOverlayDataEntry(UsdAttribute usdAttr) : usdAttr(usdAttr) {}

        PreOverlayDataEntry() {}
        ~PreOverlayDataEntry() {}
        PreOverlayDataEntry(PreOverlayDataEntry &&other) {
            preOverlayDataMap = std::move(other.preOverlayDataMap);
            usdAttr = other.usdAttr;
        }
        PreOverlayDataEntry& operator=(PreOverlayDataEntry &&other) {
            preOverlayDataMap = std::move(other.preOverlayDataMap);
            usdAttr = other.usdAttr;
            return *this;
        }
        PreOverlayDataEntry(PreOverlayDataEntry const& other){
            preOverlayDataMap = other.preOverlayDataMap;
            usdAttr = other.usdAttr;
        }

        // Store this attribute's data at the given time.
        void storeAtTime(UsdTimeCode time) {
            VtArray<T> dataArray;
            if (usdAttr.Get(&dataArray, time)){
                preOverlayDataMap[time] = dataArray;
            }
        }

        // Get the stored data for this attribute at the given point and time.
        bool getPointValue(UsdTimeCode time, int ptNum, T& value) {
            VtArray<T> dataArray;
            if (preOverlayDataMap.count(time) > 0) {
                dataArray = preOverlayDataMap[time];
                if (dataArray.size() > ptNum) {
                    value = dataArray[ptNum];
                    return true;
                }
            } else {
                usdAttr.Get(&dataArray, time);
                if (dataArray.size() > ptNum) {
                    value = dataArray[ptNum];
                    return true;
                }
            }
            return false;
        }
    };


    // Boost variant for the different types of data stored by attributes.
    typedef boost::variant<PreOverlayDataEntry<int>, 
                           PreOverlayDataEntry<GfVec3f>,
                           PreOverlayDataEntry<GfQuath>> dataEntry;

    // Map from attribute token to original data from base point instancer.
    std::map<TfToken, dataEntry> m_preOverlayDataMap;

    // Visitor for the boost variant to call the store at time function.
    struct StoreAtTime : public boost::static_visitor<>
    {
        StoreAtTime(UsdTimeCode time) : time(time){}
        
        UsdTimeCode time;

        template <typename T>
        void operator()(T& t) const {
            t.storeAtTime(time);
        }
    };

    // Visitor to clear stored data in a data entry.
    struct ClearData : public boost::static_visitor<>
    {
        template <typename T>
        void operator()(T t) const { 
            t.preOverlayDataMap.clear(); 
        }
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_INSTANCERWRAPPER_H__

