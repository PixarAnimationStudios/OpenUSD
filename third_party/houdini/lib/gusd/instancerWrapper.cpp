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
#include "instancerWrapper.h"

#include "gusd/context.h"
#include "gusd/GU_PackedUSD.h"
#include "gusd/UT_Gf.h"
#include "gusd/refiner.h"


#include "gusd/GT_PrimCache.h"
#include "gusd/USD_XformCache.h"

#include <GT/GT_DAConstantValue.h>
#include <GT/GT_DAIndexedString.h>
#include <GT/GT_DANumeric.h>
#include <GT/GT_GEODetail.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_RefineCollect.h>
#include <GT/GT_TransformArray.h>
#include <GT/GT_Util.h>
#include <GU/GU_PrimPacked.h>
#include <OBJ/OBJ_Node.h>
#include <OP/OP_Director.h>
#include <SOP/SOP_Node.h>
#include <UT/UT_Assert.h>
#include <UT/UT_Matrix3.h>
#include <UT/UT_Matrix4.h>
#include <UT/UT_Quaternion.h>
#include <UT/UT_StringArray.h>

#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/sdf/cleanupEnabler.h"

#include <pxr/base/gf/quatf.h>
#include <pxr/base/gf/transform.h>
#include <pxr/base/tf/pathUtils.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::map;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((prunable, "pruning:prunable"))
    (ReferencedPath)
    (Xform)
);     

namespace {

const SdfPath kReferenceProtoPath("Prototypes");

// TODO This is duped from OP_gusd/ROP_UsdInstPrototypes.cpp.
// Move to a common location.
SdfPath _rootPrimPath(const SdfPath path)
{
    if( path.IsRootPrimPath()
     || path.IsEmpty()) {
        return path;
    }

    return _rootPrimPath(path.GetParentPath());
}

void setAngularVelocity(GT_DataArrayHandle houWAttr, std::vector<fpreal32>& houWArray) {
    // Houdini stores angular velocity in radians per second.
    // USD is degrees per second
    const GT_Size numVals = houWAttr->entries() * houWAttr->getTupleSize();
    houWArray = std::vector<fpreal32>(numVals);
    houWAttr->fillArray(houWArray.data(), 0, houWAttr->entries(), houWAttr->getTupleSize());
    std::transform(
        houWArray.begin(),
        houWArray.end(),
        houWArray.begin(),
        std::bind1st(std::multiplies<fpreal32>(), 180.0 / M_PI));
}

void setTransformAttrsFromComponents(UsdAttribute& usdPositionAttr,
                                     UsdAttribute& usdRotationAttr,
                                     UsdAttribute& usdScaleAttr,
                                     const GT_AttributeListHandle& gtAttrs,
                                     UsdTimeCode time)
{
    // Build instance transform using houdini attribute conventions.
    // It's assumed that the GT_AttributeListHandle contains point attributes.
    if (!usdRotationAttr.IsValid()
     || !usdScaleAttr.IsValid()
     || !gtAttrs)
        return;

    GT_DataArrayHandle houPosAttr          = gtAttrs->get("P", 0); 
    GT_DataArrayHandle houNormalAttr       = gtAttrs->get("N", 0); 
    GT_DataArrayHandle houVelAttr          = gtAttrs->get("v", 0); 
    GT_DataArrayHandle houUpAttr           = gtAttrs->get("up", 0); 
    GT_DataArrayHandle houTransAttr        = gtAttrs->get("trans", 0); 
    GT_DataArrayHandle houScaleAttr        = gtAttrs->get("scale", 0); 
    GT_DataArrayHandle houUniformScaleAttr = gtAttrs->get("pscale", 0); 
    GT_DataArrayHandle houRotAttr          = gtAttrs->get("rot", 0); 
    GT_DataArrayHandle houOrientAttr       = gtAttrs->get("orient", 0); 
    GT_DataArrayHandle houPivotAttr        = gtAttrs->get("pivot", 0); 

    bool needsScale = ( houScaleAttr || houUniformScaleAttr );
    bool needsFullScale = bool(houScaleAttr);
    bool needsRotation = ( houNormalAttr || houVelAttr || houUpAttr
                        || houRotAttr || houOrientAttr ); 

    const float *houPosArray=NULL, *houNormalArray=NULL, *houVelArray=NULL,
                *houUpArray=NULL,  *houTransArray=NULL,  *houScaleArray=NULL, 
                *houUniformScaleArray=NULL, *houRotArray=NULL,
                *houOrientArray=NULL, *houPivotArray=NULL;

    GT_DataArrayHandle houPosBuffer, houNormalBuffer, houVelBuffer, houUpBuffer,
                       houTransBuffer, houScaleBuffer, houUniformScaleBuffer,
                       houRotBuffer, houOrientBuffer, houPivotBuffer;

    if(houPosAttr && houPosAttr->getTupleSize() == 3) 
        houPosArray = houPosAttr->getF32Array(houPosBuffer);

    // Instance prim isn't valid if we don't have position data
    if(!houPosAttr || !houPosArray) 
        return;

    if(needsScale) {
        if(houScaleAttr && houScaleAttr->getTupleSize() == 3)
            houScaleArray = houScaleAttr->getF32Array(houUniformScaleBuffer);

        if(houUniformScaleAttr && houUniformScaleAttr->getTupleSize() == 1)
            houUniformScaleArray = houUniformScaleAttr
                ->getF32Array(houUniformScaleBuffer);
    }

    if(needsRotation) {
        if(houNormalAttr && houNormalAttr->getTupleSize() == 3)
            houNormalArray = houNormalAttr->getF32Array(houNormalBuffer);

        if(houVelAttr && houVelAttr->getTupleSize() == 3)
            houVelArray = houVelAttr->getF32Array(houVelBuffer);

        if(houUpAttr && houUpAttr->getTupleSize() == 3)
            houUpArray = houUpAttr->getF32Array(houUpBuffer);

        if(houTransAttr && houTransAttr->getTupleSize() == 3)
            houTransArray = houTransAttr->getF32Array(houTransBuffer);

        if(houRotAttr && houRotAttr->getTupleSize() == 4)
            houRotArray = houRotAttr->getF32Array(houRotBuffer);

        if(houOrientAttr && houOrientAttr->getTupleSize() == 4)
            houOrientArray = houOrientAttr->getF32Array(houOrientBuffer);

        if(houPivotAttr && houPivotAttr->getTupleSize() == 3)
            houPivotArray = houPivotAttr->getF32Array(houPivotBuffer);

        // Houdini convention is to substitute v for N if N is missing
        if(!houNormalArray) 
            houNormalArray = houVelArray;
    }

    if(needsScale || needsRotation) {

        UT_Vector3 defaultN(0,0,0);
        UT_Vector3 scale, up, trans, pivot;
        UT_Quaternion rot, orient;
        float defaultScale = 1.0;
        UT_Matrix4F instanceM;

        const int numPoints = houPosAttr->entries();

        GT_DataArrayHandle houRotationsHandle;
        GT_Real32Array* houRotations = NULL;
        if(needsRotation) {
            houRotations = new GT_Real32Array(numPoints, 4);
            houRotationsHandle = houRotations;
        }

        GT_DataArrayHandle houScalesHandle;
        GT_DataArrayHandle houUniformScalesHandle;
        GT_Real32Array* houScales = NULL;
        GT_Real32Array* houUniformScales = NULL;
        if(needsFullScale) {
            houScales = new GT_Real32Array(numPoints, 3);
            houScalesHandle = houScales;
        }
        else if(needsScale) { 
            houUniformScales = new GT_Real32Array(numPoints, 3);
            houUniformScalesHandle = houUniformScales;
        }


        for(std::size_t i=0; i<numPoints; ++i) {

            if(houScaleArray)  scale.assign(&houScaleArray[i*3]);
            if(houUpArray)     up.assign(&houUpArray[i*3]);
            if(houTransArray)  trans.assign(&houTransArray[i*3]);
            if(houPivotArray)  pivot.assign(&houPivotArray[i*3]);
            if(houRotArray)    rot = UT_Quaternion(&houRotArray[i*4]);
            if(houOrientArray) orient = UT_Quaternion(&houOrientArray[i*4]);

            instanceM.instance(
                UT_Vector3(&houPosArray[i*3]),
                houNormalArray       ? UT_Vector3(&houNormalArray[i*3]) : defaultN,
                houUniformScaleArray ? houUniformScaleArray[i] : defaultScale,
                houScaleArray        ? &scale  : NULL,
                houUpArray           ? &up     : NULL,
                houRotArray          ? &rot    : NULL,
                houTransArray        ? &trans  : NULL,
                houOrientArray       ? &orient : NULL,
                houPivotArray        ? &pivot  : NULL
            );

            // reusing rot & scale
            UT_Matrix3F xform(instanceM);
            xform.extractScales(scale);
            if(houScales != NULL) {
                houScales->setTuple(scale.data(), i);
            }
            else if(houUniformScales != NULL) {
                houUniformScales->set(scale.x(), i, 0);
                houUniformScales->set(scale.x(), i, 1);
                houUniformScales->set(scale.x(), i, 2);
            }
            if(houRotations != NULL) {
                // TODO clean this up
                rot.updateFromRotationMatrix(xform);
                GfQuatf gfRot(rot.w(), GfVec3f(rot.x(), rot.y(), rot.z()));
                gfRot.Normalize();
                // Houdini quaternions are i,j,k,w
                rot.assign(gfRot.GetImaginary()[0],
                           gfRot.GetImaginary()[1],
                           gfRot.GetImaginary()[2],
                           gfRot.GetReal());
                houRotations->setTuple(rot.data(), i);
            }
        }

        GusdGT_Utils::setUsdAttribute(usdPositionAttr, houPosBuffer, time);

        if(needsRotation && houRotationsHandle) {
            GusdGT_Utils::setUsdAttribute(usdRotationAttr, houRotationsHandle, time);
        }

        if(needsFullScale && houScalesHandle) {
            GusdGT_Utils::setUsdAttribute(usdScaleAttr, houScalesHandle, time);
        }
        else if(needsScale && houUniformScalesHandle) {
            GusdGT_Utils::setUsdAttribute(usdScaleAttr,
                                        houUniformScalesHandle, time);
        }
    }
}

} // anon namespace

/////////////////////////////////////////////////////////////////////////////

GusdInstancerWrapper::
GusdInstancerWrapper(
        const UsdStagePtr& stage,
        const SdfPath& path,
        const GusdContext& ctxt,
        bool isOverride )
{
    initUsdPrim( stage, path, isOverride );
}

GusdInstancerWrapper::
GusdInstancerWrapper(
    const UsdGeomPointInstancer& usdInstancer, 
    UsdTimeCode                  time,
    GusdPurposeSet               purposes )
    : GusdPrimWrapper( time, purposes )
    , m_usdPointInstancer( usdInstancer )
{
}

GusdInstancerWrapper::
~GusdInstancerWrapper()
{}

bool GusdInstancerWrapper::
initUsdPrim(const UsdStagePtr& stage,
            const SdfPath& path,
            bool asOverride)
{
    bool newPrim = true;
    if( asOverride ) {
        UsdPrim existing = stage->GetPrimAtPath( path );
        if( existing ) {
            newPrim = false;
            m_usdPointInstancer = UsdGeomPointInstancer(stage->OverridePrim( path ));
        }
        else {
            // When fracturing, we want to override the outside surfaces and create
            // new inside surfaces in one export. So if we don't find an existing prim
            // with the given path, create a new one.
            m_usdPointInstancer = UsdGeomPointInstancer::Define( stage, path );
        }
    }
    else {
        m_usdPointInstancer = UsdGeomPointInstancer::Define( stage, path );  
    }
    if( !m_usdPointInstancer || !m_usdPointInstancer.GetPrim().IsValid() ) {
        TF_WARN( "Unable to create %s instancer '%s'.", newPrim ? "new" : "override", path.GetText() );
    }
    return bool( m_usdPointInstancer );
}

void GusdInstancerWrapper::storePreOverlayData(bool justProtoIndices,
        const UsdTimeCode* time=nullptr) {
    // Store point instancer attribute data so that we can write "partial"
    // overlays during overlay transform. Once a new attribute list is overlaid
    // we lose the original data, so we have to store it somewhere if we want to
    // access it for points where we just want the original data.

    UsdGeomPointInstancer& pI = m_usdPointInstancer;
    UsdAttribute usdProtoIndicesAttr = pI.GetProtoIndicesAttr();
    m_preOverlayDataMap[UsdGeomTokens->protoIndices] = 
        PreOverlayDataEntry<int>(usdProtoIndicesAttr);
    if (!justProtoIndices) {
        m_preOverlayDataMap[UsdGeomTokens->positions] = 
            PreOverlayDataEntry<GfVec3f>(pI.GetPositionsAttr());
        m_preOverlayDataMap[UsdGeomTokens->orientations] = 
            PreOverlayDataEntry<GfQuath>(pI.GetOrientationsAttr());
        m_preOverlayDataMap[UsdGeomTokens->scales] = 
            PreOverlayDataEntry<GfVec3f>(pI.GetScalesAttr());
        m_preOverlayDataMap[UsdGeomTokens->velocities] = 
            PreOverlayDataEntry<GfVec3f>(pI.GetVelocitiesAttr());
        m_preOverlayDataMap[UsdGeomTokens->angularVelocities] = 
            PreOverlayDataEntry<GfVec3f>(pI.GetAngularVelocitiesAttr());
    }

    // Get all the time samples we have for prototype indices, or use the
    // the provided timecode.
    std::vector<double> times;
    if (time == nullptr){
        usdProtoIndicesAttr.GetTimeSamples(&times);
    } else {
        times.push_back(time->GetValue());
    }
    
    // For time samples where we have prototype indices, store data.
    for (int i = 0; i < times.size(); i++) {
        for (TfToken token : m_usdGeomTokens) {
            if (!m_preOverlayDataMap.count(token))
                continue;
            boost::apply_visitor(StoreAtTime(UsdTimeCode(times[i])), m_preOverlayDataMap[token]);
        }
    }

}
void GusdInstancerWrapper::clearPreOverlayData() {
    // Clears original data so we don't have to store unnecessary information.
    for (TfToken token : m_usdGeomTokens){
        if (!m_preOverlayDataMap.count(token))
            continue;
        boost::apply_visitor(ClearData{}, m_preOverlayDataMap[token]);
    }

}

GT_PrimitiveHandle GusdInstancerWrapper::
defineForWrite(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        const GusdContext& ctxt)
{
    GusdInstancerWrapper* instancePrim =
        new GusdInstancerWrapper( stage, path, ctxt, ctxt.writeOverlay );

    if( ctxt.writeOverlay ) {
        // For overlays, store original point instancer data in case of partial
        // overlay.
        bool justProtoIndices = (ctxt.overlayPoints || ctxt.overlayAll);
        if (ctxt.granularity != GusdContext::PER_FRAME){
            instancePrim->storePreOverlayData(justProtoIndices);
        } else {
            instancePrim->storePreOverlayData(justProtoIndices, &ctxt.time);
        }
    }

    if(!ctxt.writeOverlay || ctxt.overlayAll) {
        
        // Set empty defaults for positions, scale, and indices. 
        // This prevents katana errors when expanding per-frame exports
        // with animated visibility.
        if(instancePrim) {
            VtIntArray intArray;
            VtVec3fArray vec3fArray;
            VtQuathArray quathArray;
            instancePrim->m_usdPointInstancer.GetProtoIndicesAttr().Set(intArray, UsdTimeCode::Default());
            instancePrim->m_usdPointInstancer.GetPositionsAttr().Set(vec3fArray, UsdTimeCode::Default());
            instancePrim->m_usdPointInstancer.GetScalesAttr().Set(vec3fArray, UsdTimeCode::Default());
            instancePrim->m_usdPointInstancer.GetOrientationsAttr().Set(quathArray, UsdTimeCode::Default());
        }

    }

    // Write out prototypes in usd and build an index map for prototype
    // relationships.
    instancePrim->writePrototypes( ctxt, stage, sourcePrim );

    if( ctxt.writeOverlay ) {
        // If we are writing an overlay, turn off pruning for this point instancer.
        // We may have shuffled the instance index order. 
        if( UsdAttribute attr = 
                instancePrim->m_usdPointInstancer.GetPrim().CreateAttribute( 
                                        _tokens->prunable, SdfValueTypeNames->Bool, 
                                        false, SdfVariabilityUniform )) {
            attr.Set( false );
        }
    }


    return GT_PrimitiveHandle( instancePrim );
}

GT_PrimitiveHandle GusdInstancerWrapper::
defineForRead( const UsdGeomImageable&  sourcePrim, 
               UsdTimeCode              time,
               GusdPurposeSet           purposes )
{
    return new GusdInstancerWrapper( 
                    UsdGeomPointInstancer( sourcePrim.GetPrim() ),
                    time,
                    purposes );
}

bool GusdInstancerWrapper::
redefine( const UsdStagePtr& stage,
          const SdfPath& path,
          const GusdContext& ctxt,
          const GT_PrimitiveHandle& sourcePrim )
{
    initUsdPrim( stage, path, ctxt.writeOverlay );

    if( ctxt.writeOverlay ) {
        // If per frame and redefining, we don't need to store all data, so
        // clear previous frames and store this frame's.
        bool justProtoIndices = (ctxt.overlayPoints || ctxt.overlayAll);
        if (ctxt.granularity == GusdContext::PER_FRAME){
            clearPreOverlayData();
            storePreOverlayData(justProtoIndices);
        } else {
            storePreOverlayData(justProtoIndices, &ctxt.time);
        }
    }

    if(!ctxt.writeOverlay || ctxt.overlayAll) {
        
        // Set empty defaults for positions, scale, and indices. 
        // This prevents katana errors when expanding per-frame exports
        // with animated visibility.
        if( m_usdPointInstancer ) {
            VtIntArray intArray;
            VtVec3fArray vec3fArray;
            VtQuathArray quathArray;
            m_usdPointInstancer.GetProtoIndicesAttr().Set(
                intArray, UsdTimeCode::Default());
            m_usdPointInstancer.GetPositionsAttr().Set(
                    vec3fArray, UsdTimeCode::Default());
            m_usdPointInstancer.GetScalesAttr().Set(
                    vec3fArray, UsdTimeCode::Default());
            m_usdPointInstancer.GetOrientationsAttr().Set(
                quathArray, UsdTimeCode::Default());
       
        }
    }

    stage->OverridePrim( path.AppendPath(
        !m_prototypesScope.IsEmpty() ? m_prototypesScope : kReferenceProtoPath));

    m_relationshipIndexMap.clear();
    clearCaches();

    // Write out prototypes in usd and build an index map for prototype
    // relationships.
    writePrototypes( ctxt, stage, sourcePrim );

    return true;
}

bool GusdInstancerWrapper::
getUniqueID(int64& id) const
{
    static const int s_id = GT_Primitive::createPrimitiveTypeId();
    id = s_id;
    return true;
}


const char* GusdInstancerWrapper::
className() const
{
    return "GusdInstancerWrapper";
}


void GusdInstancerWrapper::
enlargeBounds(UT_BoundingBox boxes[], int nsegments) const
{
    // TODO
}


int GusdInstancerWrapper::
getMotionSegments() const
{
    // TODO
    return 1;
}


int64 GusdInstancerWrapper::
getMemoryUsage() const
{
    // TODO
    return 0;
}


GT_PrimitiveHandle GusdInstancerWrapper::
doSoftCopy() const
{
    // TODO
    return GT_PrimitiveHandle(new GusdInstancerWrapper( *this ));
}


bool GusdInstancerWrapper::isValid() const
{
    return m_usdPointInstancer;
}

void GusdInstancerWrapper::
writePrototypes(const GusdContext& ctxt, const UsdStagePtr& stage,
                const GT_PrimitiveHandle& sourcePrim)
{
    // Write out all the prototypes in usd, populate the prototype relationship
    // array and build a map from prototype name to an index into the prototype
    // array.
    //
    // When we write the point instancer, we will write all our prototypes 
    // into a group called "Prototypes" that is a child of the point instancer.
    // Each prototype will be given the name "prototype_*" numbered for each
    // primitive at the marked "usdprototypespath" node.
    //
    // This map is used to compute the index that is stored in each enty of the
    // point array that is used to pick which prototype to use for that entry.

    if( !m_usdPointInstancer) {
        TF_WARN( "No usd point instancer found to write prototypes to." );
        return;
    }

    // Check to make sure we have a valid source prim (NULL when 0 points)
    if (sourcePrim == NULL) {
        return;
    }

    // Get prorotypes path from context in case it was set as a parameter.
    string usdPrototypesPath = ctxt.usdPrototypesPath;
    GT_Owner owner;
    GT_DataArrayHandle prototypesPathAttr;
    // If an attribute exists for usdprototypespath, set that as our path.
    prototypesPathAttr = sourcePrim->findAttribute("usdprototypespath", owner, 0);
    if(prototypesPathAttr != NULL && prototypesPathAttr->entries() > 0) {
        usdPrototypesPath = prototypesPathAttr->getS(0);
    }

    if (usdPrototypesPath.empty() && !ctxt.writeOverlay) {
        TF_WARN("No usdprototypespath attribute found. Specify where all the packed prototypes are to build a point instancer.");
        return;
    }
    
    // Check if we have an attribute for a custom prototypes scope.
    string usdPrototypesScope;
    GT_DataArrayHandle prototypesScopeAttr;
    prototypesScopeAttr = sourcePrim->findAttribute("usdprototypesscope", owner, 0);
    if(prototypesScopeAttr != NULL && prototypesScopeAttr->entries() > 0) {
        usdPrototypesScope = prototypesScopeAttr->getS(0);
    }

    m_prototypesScope = kReferenceProtoPath;
    if (!usdPrototypesScope.empty()) {
        std::string errMsg;
        if (!SdfPath::IsValidPathString(usdPrototypesScope, &errMsg)) {
            TF_WARN("Prototype scope '%s' is an invalid Usd scope, "
                "using standard prototype scope instead.",
                usdPrototypesScope.c_str());
        } else {
            m_prototypesScope = SdfPath(usdPrototypesScope);
        }
    }

    // Get the prim path for the root (point instancer) to use as the parent
    // scope for prototypes
    SdfPath protoPath = m_usdPointInstancer.GetPath().AppendPath(m_prototypesScope);

    // Collect sops containing prototypes into a list. If usdPrototypesPath
    // references a sop, it will be a length of one. If it references a subnet,
    // it will contain all the renderable sops within the net.
    std::vector<SOP_Node*> protoNodes;
    OBJ_Node* objNode;
    UT_Matrix4D localToWorldMatrix;
    double time = CHgetTimeFromFrame(GusdUSD_Utils::GetNumericTime(ctxt.time));
    OP_Context houdiniContext(time);
    if (!usdPrototypesPath.empty()) {
        objNode = OPgetDirector()->findOBJNode(usdPrototypesPath.c_str());
        if( objNode ) {
            if(objNode->getObjectType()== OBJ_SUBNET) {
                for(int child = 0; child < objNode->getNchildren(); child++) {
                    if(auto childObjNode = CAST_OBJNODE(objNode->getChild(child))){
                        if (auto protoNode = childObjNode->getRenderSopPtr()) {
                            protoNodes.push_back(protoNode);
                        }
                    }
                }
            } else if (auto protoNode = objNode->getRenderSopPtr()) {
                protoNodes.push_back(protoNode);
            }
        } else if (auto protoNode = OPgetDirector()->findSOPNode(usdPrototypesPath.c_str())) {
            protoNodes.push_back(protoNode);
            objNode = CAST_OBJNODE(protoNode->getCreator());
        }

        if( (protoNodes.empty() || !objNode) && !ctxt.writeOverlay ) {
            TF_WARN("No node found at usdPrototypesPath '%s'", usdPrototypesPath.c_str());
            return;
        }

        // Get the obj node transform
        objNode->getLocalToWorldTransform( houdiniContext, localToWorldMatrix);
    }

    // Populate a map from instance path to detail hanlde. Each detail will be
    // refined then written as a prototype, and the instance path will be used
    // to create a mapping to the usd path for indexing in the point instancer.
    std::map<std::tuple<string, bool>, GU_DetailHandle> protoDetailMap;

    // Iterate through sops containing prototypes, and create a detail handle
    // for each packed primitive.
    for (int i=0; i < protoNodes.size(); i++) {
        if(auto protoNode = protoNodes[i]){
            // If the sop is a subnetwork get the renderable node
            if (protoNode->isSubNetwork(true)) {
                protoNode = CAST_SOPNODE(protoNode->getRenderNodePtr());
            }
            GU_DetailHandle cookedGeoHdl = protoNode->getCookedGeoHandle(houdiniContext);
            GU_DetailHandleAutoReadLock detailLock( cookedGeoHdl );

            // usdinstancepath can be any kind of attribute (may use detail if
            // one proto)
            const GA_AttributeOwner searchOrder[4] = {GA_ATTRIB_VERTEX,
                                                      GA_ATTRIB_POINT,
                                                      GA_ATTRIB_PRIMITIVE,
                                                      GA_ATTRIB_GLOBAL};
            GA_ROHandleS instancePathAttr( detailLock->findAttribute(
                                                    "usdinstancepath",
                                                    searchOrder,
                                                    4 ));

            // If the usdinstancepath is a valid Sdf path, we use it as the
            // names of each prototype.
            bool generateProtoNames = false;
            // We also support instancepath instead of usdinstancepath
            if (!instancePathAttr.isValid()) {
                instancePathAttr = GA_ROHandleS( detailLock->findAttribute(
                                                        "instancepath",
                                                        searchOrder,
                                                        4 ));
                // These are generall full paths to nodes and should not be
                // used as prototype scopes.
                generateProtoNames = true;
            }

            // Iteratve over each primitive and create a detail handle
            GA_Range primRange = detailLock->getPrimitiveRange();
            for(GA_Iterator offsetIt(primRange); !offsetIt.atEnd(); ++offsetIt) {
                // Use the context's usdinstancepath as default if no attributes
                std::tuple<string, bool> usdInstancePath(ctxt.usdInstancePath, generateProtoNames);
                if (instancePathAttr.isValid()) {
#if HDK_API_VERSION < 16050000
                    string instancePathAttrVal = instancePathAttr.get(offsetIt.getOffset());
#else
                    string instancePathAttrVal = instancePathAttr.get(offsetIt.getOffset()).toStdString();
#endif
                    if (!instancePathAttrVal.empty()) {
                        std::get<0>(usdInstancePath) = instancePathAttrVal;
                        std::string errMsg;
                        if (!generateProtoNames &&
                            !SdfPath::IsValidPathString(instancePathAttrVal, &errMsg)) {
                            TF_WARN("Instance name '%s' is an invalid Usd scope, "
                                "using standard prototype naming instead.",
                                instancePathAttrVal.c_str());
                            std::get<1>(usdInstancePath) = true;
                        }
                    }
                }
                if (!std::get<0>(usdInstancePath).empty()) {
                    const GU_Detail *srcDetail = detailLock.getGdp();
                    GA_PrimitiveGroup primGroup(*srcDetail);
                    primGroup.addOffset(offsetIt.getOffset());
                    // Create a detail based on a prim group for each primitive
                    GU_Detail *detail = new GU_Detail(srcDetail, &primGroup);
                    GU_DetailHandle detailHandle;
                    // Handle owns the detail so it will free the memory
                    detailHandle.allocateAndSet(detail, true);
                    if(detailHandle) {
                        if ( protoDetailMap.count(usdInstancePath) ) {
                            TF_WARN ("Multiple prototypes found with instance "
                                "path '%s', may result in loss of prototypes.",
                                std::get<0>(usdInstancePath).c_str());
                        }
                        protoDetailMap[usdInstancePath] = detailHandle;
                    }
                } else {
                    TF_WARN("No instance path found for primitive in node %s",protoNode->getName().c_str());
                }
            }
        }
    }

    // Map to store instance path and usd path, for generating mapping for index
    // array
    std::map<string, SdfPath> protoPathsMap;

    // Index for naming prototypes
    int protoIdx = 0;

    for ( const auto &pair : protoDetailMap ) {
        GT_RefineParms refineParms;
        // Tell the collectors (in particular the f3d stuff) that we are 
        // writing a USD file rather than doing interactive visualization.
        // an interactive visualization
        refineParms.set( "refineToUSD", true );

        GusdContext newContext = GusdContext( ctxt );

        // We don't want to force overlays of prototypes. If it has the same
        // scope in an overlay all it will still overlay, but this way new
        // prototypes won't be pure overs.
        newContext.writeOverlay = false;

        // If a prototype is selecting a variant, make sure to set it
        newContext.authorVariantSelections = true;

        GusdRefinerCollector refinerCollector;

        string protoUsdName;
        if ( !std::get<1>(pair.first) ) {
            protoUsdName = std::get<0>(pair.first);
        } else {
            protoUsdName = "prototype_";
            protoUsdName += std::to_string(protoIdx);
            protoIdx++;
        }
        SdfPath protoUsdPath = protoPath.AppendPath(SdfPath(protoUsdName));
        
        GusdRefiner refiner( 
            refinerCollector,
            protoUsdPath,
            "", 
            localToWorldMatrix );

        refiner.m_refinePackedPrims = true;

        // Set the refiner to build prototypes (so we don't recurse and create
        // another point instancer)
        refiner.m_buildPrototypes = true;

        // Refine the detail handle
        refiner.refineDetail( pair.second, refineParms );

        // Build the instancer prims.
        const GusdRefiner::GprimArray& gprimArray = refiner.finish();

        // Sort the refined prim array by primitive paths. This ensures parents
        // will be written before their children.
        GusdRefinerCollector::GprimArray gPrims = gprimArray;
        std::sort( gPrims.begin(), gPrims.end(),
                []( const GusdRefinerCollector::GprimArrayEntry& a,
                    const GusdRefinerCollector::GprimArrayEntry& b ) -> bool
                { return a.path < b.path; } );

        GusdSimpleXformCache xformCache;

        // Iterate over the prims we need to write
        for( auto& gtPrim : gPrims ) {
            // Create a new USD prim
            const SdfPath& primPath = gtPrim.path;
            GT_PrimitiveHandle usdPrim;
            usdPrim = GusdPrimWrapper::defineForWrite(
                        gtPrim.prim, stage, primPath, newContext );

            if( !usdPrim ) {
                TF_WARN( "prim did not convert. %s", gtPrim.prim->className() );
            }
            else {
                GusdPrimWrapper* primPtr
                        = UTverify_cast<GusdPrimWrapper*>(usdPrim.get());
                primPtr->markVisible( true );
            }

            if(usdPrim) {
                GusdPrimWrapper* primPtr
                        = UTverify_cast<GusdPrimWrapper*>(usdPrim.get());

                newContext.purpose = gtPrim.purpose;
                
                // Copy attributes from gt prim to USD prim.
                primPtr->updateFromGTPrim(gtPrim.prim,
                                          gtPrim.xform,
                                          newContext,
                                          xformCache );

                // Create an array of prototype transforms for subtracting from
                // instance transforms later
                m_prototypeTransforms.push_back(gtPrim.xform);
            }
        }

        // Add the mapping from instance path (pair.first) to usd path
        protoPathsMap[std::get<0>(pair.first)] = protoUsdPath;
    }

    if(ctxt.writeOverlay && (!ctxt.overlayAll || usdPrototypesPath.empty())) {

        // If we are doing an overlay, build the map from the existing relationships
        UsdRelationship prototypesRel = m_usdPointInstancer.GetPrototypesRel();
        SdfPathVector targets;
        prototypesRel.GetForwardedTargets(&targets);
        for(size_t i=0; i<targets.size(); ++i) {
            m_relationshipIndexMap[targets[i].GetNameToken()] = i;
        }
    }
    UsdRelationship prototypesRel = m_usdPointInstancer.GetPrototypesRel();
    
    // Always clear the prortypes relationship array as we either don't touch it
    // or write it from scrath (rather than trying to add on top of old protos).
    prototypesRel.ClearTargets(true);
    int relIdx = 0;

    // When overlaying all, we want to set the prototypes relationship array
    // rather then add targets to it, so we collect all paths into a vector.
    SdfPathVector relationshipPaths;
    for ( const auto &pair : protoPathsMap ) {
        string mapKey = pair.first;
        SdfPath relationshipPath = pair.second;

        // USD doesn't allow references to non-root prims. If we want to build
        // a point instancer with non-root prims, we reference the root prim
        // but point the relationship to point to a descendant. When we wrote
        // the prototype file, we added an attribute to tell us what descendant
        // to use.
        UsdPrim protoRootPrim( stage->DefinePrim( pair.second, _tokens->Xform ));
        protoRootPrim.Load();
        // TODO Enable cleanup to remove empty tokens left after moving
        // transforms in the case of prototype transforms where the prototype
        // is not the referenced root.
        //SdfCleanupEnabler();
        for(auto protoPrim : protoRootPrim.GetAllChildren()) {
            UsdAttribute pathAttr
                = protoPrim.GetAttribute(_tokens->ReferencedPath);
            if( pathAttr ) {
                string subPath;
                pathAttr.Get(&subPath);
                relationshipPath = protoPrim.GetPath().AppendPath(SdfPath(subPath));

                // Get the prototype scope referenced by the relationship array
                UsdPrim protoTarget = stage->GetPrimAtPath(relationshipPath);
                if (!protoTarget.IsValid()) {
                    TF_WARN( "Prototype does not exist at '%s'",
                                relationshipPath.GetString().c_str() );
                    continue;
                }

                // Get the Xformables at the prototype scope and the referenced
                // prototype scope (where we actually retrieve geometry)
                UsdGeomXformable protoXformable( protoPrim );
                UsdGeomXformable protoTargetXformable( protoTarget );

                // Get the xforms we wrote out on the prototype scope
                bool resetXformStack = false;
                std::vector<UsdGeomXformOp> xformOps = protoXformable.GetOrderedXformOps(&resetXformStack);
                if (xformOps.size()==0)
                    continue;

                // Set the transform on the referenced scope to be the same
                // we wrote onto the prototype scope. First clear previous
                // xformOps.
                protoTargetXformable.SetXformOpOrder(std::vector<UsdGeomXformOp>());
                for (auto xformOp : xformOps) {
                    // Add an equivalent xformOp to the target prototype scope
                    // that was in the original protoype scope.
                    const UsdGeomXformOp xformOpTarget =
                        protoTargetXformable.AddXformOp(xformOp.GetOpType(),
                                                        xformOp.GetPrecision());
                    xformOpTarget.Set( xformOp.GetOpTransform(ctxt.time),
                                       ctxt.time);

                    // Clear each xformOp from the original scope.
                    xformOp.GetAttr().Clear();
                }
                protoXformable.GetXformOpOrderAttr().Clear();
            }
        }
        if(ctxt.overlayAll) {
            relationshipPaths.push_back(relationshipPath);
        } else {
            prototypesRel.AddTarget(relationshipPath);
        }
        m_relationshipIndexMap[TfToken(mapKey)] = relIdx++;
    }
    if (ctxt.overlayAll && relationshipPaths.size() > 0) {
        // Set the targets as this forces the point instancer to explicitly
        // only uses the new prototypes and not the ones from the file
        // we are overlaying.
        prototypesRel.SetTargets(relationshipPaths);
    }

}

bool GusdInstancerWrapper::
updateFromGTPrim(const GT_PrimitiveHandle& sourcePrim,
                 const UT_Matrix4D&        houXform,
                 const GusdContext&        ctxt,
                 GusdSimpleXformCache&     xformCache )
{     
    if( !m_usdPointInstancer ) {
        TF_WARN( "Can't update USD point instancer from GT prim '%s'", m_usdPointInstancer.GetPrim().GetPath().GetText() );
        return false;
    }

    DBG(cerr << "GusdInstanceWrapper::updateFromGTPrim, " << 
            m_usdPointInstancer.GetPath().GetString() << endl);

    bool writeTransforms = !ctxt.writeOverlay || ctxt.overlayAll || 
                                    ctxt.overlayPoints || ctxt.overlayTransforms;

    //--------------------------------------------------------------------------

    GfMatrix4d xform = computeTransform (
                            m_usdPointInstancer.GetPrim().GetParent(),
                            ctxt.time,
                            houXform,
                            xformCache);
 
    // intrinsic attributes ----------------------------------------------------
    
    GT_Owner attrOwner = GT_OWNER_INVALID;
    GT_DataArrayHandle houAttr;
    UsdAttribute usdAttr;
    std::vector<bool> maskAtTime;

    // If this is not an overlay, write out the transform.
    // (If it is an overlay, the instances themselves will be
    // set to correct locations via their position attribute.)
    if( !ctxt.writeOverlay ) {
        
        // transform
        updateTransformFromGTPrim( xform, ctxt.time, 
                                   ctxt.granularity == GusdContext::PER_FRAME );
    }
    
    int numPoints = 0;
    if( writeTransforms ) {

        UT_Matrix4D localToWorld;
        GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
            m_usdPointInstancer.GetPrim(),
            ctxt.time,
            localToWorld );
        UT_Matrix4D worldToLocal = localToWorld;
        worldToLocal.invert();
 
        // visibility
        updateVisibilityFromGTPrim(sourcePrim, ctxt.time, 
                               ctxt.granularity == GusdContext::PER_FRAME );

        // P
        houAttr = sourcePrim->findAttribute("P", attrOwner, 0);
        if(houAttr) {
            numPoints = houAttr->entries();
        }

        // Mask
        // XXX We currently don't support the mask attribute, but it could
        // be authored on a prim we're overlaying, in which case we write
        // a constant value of true.
        maskAtTime = m_usdPointInstancer.ComputeMaskAtTime(ctxt.time);
        if( !maskAtTime.empty() ) {
            m_usdPointInstancer.ActivateAllIds();
            m_usdPointInstancer.VisAllIds(ctxt.time);
        }

        // Indices
        usdAttr = m_usdPointInstancer.GetProtoIndicesAttr();

        bool gotValidIndices = false;

        GT_Int32Array* idxArray = new GT_Int32Array(numPoints, 1); 
        GT_DataArrayHandle houAttr(idxArray);

        // define & update prototypes ----------------------------------------------

        // Get the instance paths from attributes on the points.
        GT_DataArrayHandle instancePathAttr;
        if( auto pntAttrs = sourcePrim->getPointAttributes() ) {
            instancePathAttr = pntAttrs->get( "usdinstancepath" );
            if (instancePathAttr == NULL) {
                instancePathAttr = pntAttrs->get( "instancepath" );
            }
        }

        // The instance paths for the points should match the mapping we created
        // when we wrote the prototypes.
        if(instancePathAttr != NULL ) {
            if(instancePathAttr->entries() >= numPoints) {
                for(int i=0; i<numPoints; ++i) {

                    GT_String usdInstancePath = instancePathAttr->getS(i);

                    auto idxIt = m_relationshipIndexMap.find( TfToken(usdInstancePath) );
                    if( idxIt != m_relationshipIndexMap.end()) {

                        idxArray->set(idxIt->second, i);
                    }
                    else {
                        TF_WARN("Couldn't resolve prototype index for %s.",
                                             usdInstancePath);
                        idxArray->set(0, i);
                    }
                }
            } 
            gotValidIndices = true;
        } else if (!ctxt.usdInstancePath.empty()){
            // If the instancepath was set as a paramater and no attribute
            // overwrote it, check the context for a usd instance path.
            for(int i=0; i<numPoints; ++i) {
                auto idxIt = m_relationshipIndexMap.find( TfToken(ctxt.usdInstancePath) );
                if( idxIt != m_relationshipIndexMap.end()) {

                    idxArray->set(idxIt->second, i);
                }
                else {
                    TF_WARN("Couldn't resolve prototype index for %s.",
                                         ctxt.usdInstancePath.c_str());
                    idxArray->set(0, i);
                }
            }
            gotValidIndices = true;
        } else if (ctxt.writeOverlay && (ctxt.overlayPoints || ctxt.overlayAll)) {
            // If we are writing an overlay points or all, but didn't construct
            // a new prototypes relationship array, we can still find the
            // instances' appropriate proto indices from the original instancer.
            // This will be the new indices array, and does not have to be the
            // the same length as the original.

            GT_DataArrayHandle indexAttr;
            std::vector<exint> instanceIndexArray;

            // Get the houdini attribute defining the index of its position
            // in the original point instancer.
            if ( auto pntAttrs = sourcePrim->getPointAttributes() )
                indexAttr = pntAttrs->get( "__instanceindex" );

            // Get the original proto indices array.
            std::map<UsdTimeCode, VtIntArray> preOverlayProtoIndices = boost::get<PreOverlayDataEntry<int>>(
                m_preOverlayDataMap[UsdGeomTokens->protoIndices]).preOverlayDataMap;

            // If we stored indices in the point instancer we are overlaying,
            // get those. otherwise get them from the attribute.
            VtIntArray usdProtoIndicies;
            if (preOverlayProtoIndices.count(ctxt.time) > 0) {
                usdProtoIndicies = preOverlayProtoIndices[ctxt.time];
            } else if (ctxt.granularity==GusdContext::PER_FRAME) {
                m_usdPointInstancer.GetProtoIndicesAttr().Get(&usdProtoIndicies, ctxt.time);
            }

            if (usdProtoIndicies.size() > 0) {
                if (indexAttr) {
                    int numIndicies = indexAttr->entries();

                    // If we have an index for each instance:
                    if (numIndicies == numPoints) {
                        const GT_Size numVals = indexAttr->entries() * indexAttr->getTupleSize();
                        instanceIndexArray = std::vector<exint>(numVals);
                        indexAttr->fillArray(instanceIndexArray.data(), 0, indexAttr->entries(), indexAttr->getTupleSize());
                        
                        // For each instance, grab it's prototype index by
                        // accessing the original proto indices array at the
                        // index of it's place in the instancer.
                        for(int i=0; i<numPoints; ++i) {
                            int index = usdProtoIndicies[instanceIndexArray[i]];
                            idxArray->set(index, i);
                        }
                        gotValidIndices = true;
                    }
                }
            }
        } else if (!ctxt.writeOverlay || ctxt.overlayAll) {
            TF_WARN( "Instance prototypes not specified as instance path or packed prim" );
        }

        // Set indicies array
        if(gotValidIndices && usdAttr) {
            GusdGT_Utils::setUsdAttribute(usdAttr, houAttr, ctxt.time);
        }

        // When the instance has a transform, set everything here.
        const GT_AttributeListHandle gtPointAttrs = sourcePrim->getPointAttributes();
        if( gtPointAttrs->hasName("__instancetransform") ) {
            setTransformAttrsFromMatrices(
                              worldToLocal,
                              gtPointAttrs,
                              ctxt,
                              sourcePrim);
        }
        else {
            // For nativ houdini instancing with just attributes on a point.
            // v
            houAttr = sourcePrim->findAttribute("v", attrOwner, 0);
            usdAttr = m_usdPointInstancer.GetVelocitiesAttr();
            if(houAttr && usdAttr) {
                GusdGT_Utils::setUsdAttribute(usdAttr, houAttr, ctxt.time);
            }

            //w
            houAttr = sourcePrim->findAttribute("w", attrOwner, 0);
            usdAttr = m_usdPointInstancer.GetAngularVelocitiesAttr();
            if(houAttr && usdAttr) {
                std::vector<fpreal32> wArray;
                setAngularVelocity(houAttr, wArray);
                houAttr.reset(new GT_Real32Array(wArray.data(), houAttr->entries() , houAttr->getTupleSize()));
                GusdGT_Utils::setUsdAttribute(usdAttr, houAttr, ctxt.time);
            }
            UsdAttribute usdPositionAttr = m_usdPointInstancer.GetPositionsAttr();
            UsdAttribute usdRotationAttr = m_usdPointInstancer.GetOrientationsAttr();
            UsdAttribute usdScalesAttr = m_usdPointInstancer.GetScalesAttr();
            
            if (usdRotationAttr 
             && usdScalesAttr
             && gtPointAttrs) {

                setTransformAttrsFromComponents(
                                  usdPositionAttr,
                                  usdRotationAttr,
                                  usdScalesAttr,
                                  gtPointAttrs,
                                  ctxt.time);
            }
        }
    
        // extent ------------------------------------------------------------------

        VtVec3fArray extent(2);
        // Using utility function from UsdGeomPointInstancer
        if(m_usdPointInstancer.ComputeExtentAtTime(&extent, ctxt.time, ctxt.time)) {
            m_usdPointInstancer.GetExtentAttr().Set(extent, ctxt.time);
        }
    }

    // primvars ----------------------------------------------------------------
    
    if( !ctxt.writeOverlay || ctxt.overlayAll || ctxt.overlayPrimvars ) {

        GusdGT_AttrFilter filter = ctxt.attributeFilter;
        // Filter attributes which were used to construct the instance transform
        // and prototype relationships.
        filter.appendPattern(GT_OWNER_POINT,
                "^__* ^orient ^rot ^scale ^instancepath ^usdinstancepath \
                 ^usdprototypespath ^usdprototypesscope ^trans ^up");
        filter.appendPattern(GT_OWNER_POINT, "^P ^N ^v");
        filter.appendPattern(GT_OWNER_CONSTANT, "^usdprimpath ^instancepath \
                ^usdinstancepath ^usdprototypespath ^usdprototypesscope");
        if(const GT_AttributeListHandle pointAttrs = sourcePrim->getPointAttributes()) {

            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_POINT;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( pointAttrs, filter, UsdGeomTokens->uniform, ctxt.time );
        }
        if(const GT_AttributeListHandle constAttrs = sourcePrim->getDetailAttributes()) {

            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_CONSTANT;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( constAttrs, filter, UsdGeomTokens->constant, ctxt.time );
        }
    }
    // -------------------------------------------------------------------------

    return GusdPrimWrapper::updateFromGTPrim(sourcePrim, houXform, ctxt, xformCache);
}
void GusdInstancerWrapper::setTransformAttrsFromMatrices(const UT_Matrix4D &worldToLocal,
                                   const GT_AttributeListHandle gtAttrs,
                                   GusdContext ctxt,
                                   GT_PrimitiveHandle sourcePrim)
{
    // Create a map from TfToken to UsdAttribute for each one we want to set.
    std::map<TfToken, UsdAttribute> usdAttrMap;
    for (auto token : m_usdGeomTokens) {
        if (token != UsdGeomTokens->protoIndices) {
            usdAttrMap[token] =
                m_usdPointInstancer.GetPrim().GetAttribute(token);
            if(!usdAttrMap[token].IsValid()) {
                TF_WARN( "Missing '%s' attribute from point instancer. "
                    "Failed to update attributes.", token.GetString().c_str());
                return;
            }
        }
    }

    if (!gtAttrs)
        return;

    UsdTimeCode time = ctxt.time;
    
    GT_DataArrayHandle houXformAttr = gtAttrs->get("__instancetransform", 0); 
    if(!houXformAttr || !(houXformAttr->getTupleSize() ==16))
        return;

    GT_DataArrayHandle houXformBuffer;
    const fpreal64* houXformArray = houXformAttr->getF64Array(houXformBuffer);

    if(!houXformArray)
        return;

    GT_Size numXforms = houXformAttr->entries();

    int numPoints = 0;

    // If writing an overlay, get the indices of each instance into the original
    // point instancer, and see if we are writing a partial overlay of some sort.
    GT_DataArrayHandle indexAttr;

    // Map from index into point instancer to point number in houdini.
    std::map<exint, int> instanceIndexMap;

    indexAttr = gtAttrs->get( "__instanceindex" );
    if (ctxt.writeOverlay && ctxt.overlayTransforms && 
            !(ctxt.overlayAll || ctxt.overlayPoints) && indexAttr) {
        const GT_Size numVals = indexAttr->entries() * indexAttr->getTupleSize();
        std::vector<exint> instanceIndexArray = std::vector<exint>(numVals);
        indexAttr->fillArray(instanceIndexArray.data(), 0, indexAttr->entries(), indexAttr->getTupleSize());
        for (int i =0; i < instanceIndexArray.size(); i++) {
            instanceIndexMap[instanceIndexArray[i]] = i;
        }

        // Get the number of points in the original point instancer.
        std::map<UsdTimeCode, VtIntArray> preOverlayProtoIndices = boost::get<PreOverlayDataEntry<int>>(
            m_preOverlayDataMap[UsdGeomTokens->protoIndices]).preOverlayDataMap;
        if ( preOverlayProtoIndices.count(time) > 0 ) {    
            numPoints = preOverlayProtoIndices[time].size();
        } else {
            VtIntArray indices;
            m_usdPointInstancer.GetProtoIndicesAttr().Get( &indices, time);
            numPoints = indices.size();
        }
    } else {
        // Just write out a point instancer with the data provided by houdini.
        numPoints = numXforms;
    }

    // Create a handle for data from houdini
    std::map<TfToken, GT_DataArrayHandle> houHandlesMap;
    GT_Real32Array* houPositions = new GT_Real32Array(numPoints, 3);
    houHandlesMap[UsdGeomTokens->positions] = GT_DataArrayHandle(houPositions);

    GT_Real32Array* houRotations = new GT_Real32Array(numPoints, 4);
    houHandlesMap[UsdGeomTokens->orientations] = GT_DataArrayHandle(houRotations);

    GT_Real32Array* houScales = new GT_Real32Array(numPoints, 3);
    houHandlesMap[UsdGeomTokens->scales] = GT_DataArrayHandle(houScales);

    GT_Owner attrOwner = GT_OWNER_INVALID;
    GT_DataArrayHandle houVAttr = sourcePrim->findAttribute("v", attrOwner, 0);
    const float *houVArray=NULL;
    GT_Real32Array* houVelocities=NULL;
    if(houVAttr && houVAttr->getTupleSize() == 3) {
        if (numPoints == numXforms) {
            // We can set it directly with no further calculations
            houHandlesMap[UsdGeomTokens->velocities] = houVAttr;
        } else {
            // We have to construct the array point by point, in order to get
            // some data from the original point instancer.
            GT_DataArrayHandle houVBuffer;
            houVArray = houVAttr->getF32Array(houVBuffer);
            houVelocities = new GT_Real32Array(numPoints, 3);
            houHandlesMap[UsdGeomTokens->velocities] = GT_DataArrayHandle(houVelocities);
        }
    }

    GT_DataArrayHandle houWAttr = sourcePrim->findAttribute("w", attrOwner, 0);
    GT_Real32Array* houAngularVelocities = NULL;
    std::vector<fpreal32> houWArray;
    if (houWAttr && houWAttr->getTupleSize() == 3) {
        setAngularVelocity(houWAttr, houWArray);
        if (numPoints == numXforms) {
            // We can set it directly with no further calculations
            houWAttr.reset(new GT_Real32Array(houWArray.data(), houWAttr->entries() , houWAttr->getTupleSize()));
            houHandlesMap[UsdGeomTokens->angularVelocities] = houWAttr;
        } else {
            // We have to construct the array point by point, in order to get
            // some data from the original point instancer.
            houAngularVelocities = new GT_Real32Array(numPoints, 3);
            houHandlesMap[UsdGeomTokens->angularVelocities] = GT_DataArrayHandle(houAngularVelocities);
        }
    }



    // If we have transforms on prototypes, we have to remove them from our
    // final instance transformation, as the point instancer schema accounts
    // for prototype transforms. Will only be the case when writing out new
    // prototypes (new geom or overlay all).
    VtIntArray indices;
    m_usdPointInstancer.GetProtoIndicesAttr().Get( &indices, time);
    bool removeProtoTransforms = (indices.size() == numXforms) &&
                                 (m_prototypeTransforms.size() > 0);

    for(int pt=0; pt<numPoints; ++pt) {
        UT_Vector3 position;
        UT_Vector3 scale;
        UT_Quaternion q;
        UT_Vector3 velocity;
        UT_Vector3 angularVelocity;

        if (numPoints == numXforms || instanceIndexMap.count((exint)pt)) {
            // If all the data is from houdini, or this point is being overlaid,
            // get the data from houdini.

            int i = (numPoints == numXforms) ? pt : instanceIndexMap[pt];
            // Build a 4x4 that represents this instance transformation.
            // Bring this into local space and then take the 3x3 from the upper left.
            UT_Matrix4D instXform( 
                houXformArray[i*16],    houXformArray[i*16+1],  houXformArray[i*16+2],  houXformArray[i*16+3],
                houXformArray[i*16+4],  houXformArray[i*16+5],  houXformArray[i*16+6],  houXformArray[i*16+7],
                houXformArray[i*16+8],  houXformArray[i*16+9],  houXformArray[i*16+10], houXformArray[i*16+11],
                houXformArray[i*16+12], houXformArray[i*16+13], houXformArray[i*16+14], houXformArray[i*16+15] );

            UT_Matrix4D localInstXform = instXform * worldToLocal;
            // Multiply by the prototype inverse to "subtract" its transformation
            if (removeProtoTransforms) {
                int protoIdx = indices[i];
                UT_Matrix4D protoXform = m_prototypeTransforms[protoIdx];
                protoXform.invert();
                localInstXform =  protoXform * localInstXform;
            }
            localInstXform.getTranslates( position );
            UT_Matrix3D localInstXform3( localInstXform );
            localInstXform3.extractScales(scale);
            q.updateFromRotationMatrix(localInstXform3);
            q.normalize();
            // if partial mode
            if (numPoints != numXforms ) {
                if (houVArray) {
                    velocity = UT_Vector3(houVArray[i*3],
                                          houVArray[i*3+1],
                                          houVArray[i*3+2]);
                }
                if (houWArray.size() >= numXforms * 3) {
                    angularVelocity = UT_Vector3(houWArray[i*3],
                                                 houWArray[i*3+1],
                                                 houWArray[i*3+2]);
                }
            }
        } else {
            // This point was in the original point instancer but not being
            // overlaid, so get original values. Only in an overlay transform.

            TfToken token = UsdGeomTokens->positions;
            GfVec3f ptPosition;
            if (boost::get<PreOverlayDataEntry<GfVec3f>>(
                m_preOverlayDataMap[token]).getPointValue(time, pt, ptPosition))
                position = GusdUT_Gf::Cast(ptPosition);

            token = UsdGeomTokens->scales;
            GfVec3f ptScale;
            if (boost::get<PreOverlayDataEntry<GfVec3f>>(
                m_preOverlayDataMap[token]).getPointValue(time, pt, ptScale))
                scale = GusdUT_Gf::Cast(ptScale);

            token = UsdGeomTokens->orientations;
            GfQuath ptOrientation;
            if (boost::get<PreOverlayDataEntry<GfQuath>>(
                m_preOverlayDataMap[token]).getPointValue(time, pt, ptOrientation))
                GusdUT_Gf::Convert((GfQuatf)ptOrientation, q);

            token = UsdGeomTokens->velocities;
            GfVec3f ptVelocity;
            if (boost::get<PreOverlayDataEntry<GfVec3f>>(
                m_preOverlayDataMap[token]).getPointValue(time, pt, ptVelocity))
                velocity = GusdUT_Gf::Cast(ptVelocity);

            token = UsdGeomTokens->angularVelocities;
            GfVec3f ptAngularVelocity;
            if (boost::get<PreOverlayDataEntry<GfVec3f>>(
                m_preOverlayDataMap[token]).getPointValue(time, pt, ptAngularVelocity))
                angularVelocity = GusdUT_Gf::Cast(ptAngularVelocity);

        }
        houPositions->setTuple( position.data(), pt );

        houScales->setTuple(scale.data(), pt);
        // Houdini quaternions are i,j,k,w
        houRotations->setTuple(
                UT_Vector4(q.x(), q.y(), q.z(), q.w()).data(), pt);

        // We only reconstruct the data if we are doing a partial overlay.
        if (numPoints != numXforms ) {
            if (houVelocities != NULL)
                houVelocities->setTuple(velocity.data(), pt);
            if (houAngularVelocities != NULL)
                houAngularVelocities->setTuple(angularVelocity.data(), pt);
        }
    }

    // Set all the attributes' data.
    for (auto pair : houHandlesMap)
        GusdGT_Utils::setUsdAttribute(usdAttrMap[pair.first], pair.second, time);
}

bool GusdInstancerWrapper::
refine( GT_Refine& refiner,
        const GT_RefineParms* parms ) const
{

    UsdStagePtr stage = m_usdPointInstancer.GetPrim().GetStage();

    DBG(cerr << "GusdInstancerWrapper::refine, " << m_usdPointInstancer.GetPrim().GetPath() << endl);
    
    UsdRelationship relationship = m_usdPointInstancer.GetPrototypesRel();
    SdfPathVector targets;
    relationship.GetForwardedTargets( &targets );

    // Build prototype prims on demand
    vector<GT_PrimitiveHandle> protoPrims( targets.size() );

    VtArray<int> indices;
    if( !m_usdPointInstancer.GetProtoIndicesAttr().Get( &indices, m_time ) )
    {
        TF_WARN("error getting indices attribute");
        return false;
    }

    VtArray<GfMatrix4d> frames;
    if( !m_usdPointInstancer.ComputeInstanceTransformsAtTime( &frames,
                                                          m_time,
                                                          m_time,
                                                          UsdGeomPointInstancer::ProtoXformInclusion::IncludeProtoXform,
                                                          UsdGeomPointInstancer::MaskApplication::IgnoreMask  ) )
    {
        TF_WARN("ComputeFrames failed");
        return false;
    }

    if( indices.size() != frames.size() )
    {
        TF_WARN("Indices and frames arrays are not the same size");
        return false;
    }

    for( size_t targetIndex = 0; targetIndex < targets.size(); ++targetIndex ) {

        UsdPrim p = stage->GetPrimAtPath( targets[targetIndex] );

        if( !p ) {
            TF_WARN( "getting proto prim failed '%s'", targets[targetIndex].GetText() );
            continue;
        }

        GT_PrimitiveHandle gtPrim = GusdGT_PrimCache::GetInstance().GetPrim( 
                                        p, m_time, m_purposes );


        auto transforms = new GT_TransformArray();
        for( size_t i = 0; i < indices.size(); ++i ) {

            int idx = indices[i];
            if( idx != targetIndex )
                continue;

            if( idx < 0 || idx >= targets.size() )
            {
                TF_WARN("Invalid prototype index: %d", idx);;
                continue;
            }

            UT_Matrix4D m = GusdUT_Gf::Cast( frames[i] );
            transforms->append( new GT_Transform( &m, 1 ) );
        }
        if( transforms->entries() > 0 ) {

            refiner.addPrimitive( new GT_PrimInstance( gtPrim, transforms ));
        }
    }
    return true;
}

bool
GusdInstancerWrapper::unpack( 
    GU_Detail&              gdr,
    const UT_StringRef&     fileName,
    const SdfPath&          primPath,
    const UT_Matrix4D&      xform,
    fpreal                  frame,
    const char*             viewportLod,
    GusdPurposeSet          purposes )
{

    UsdPrim usdPrim = m_usdPointInstancer.GetPrim();

    UsdRelationship relationship = m_usdPointInstancer.GetPrototypesRel();
    SdfPathVector targets;
    relationship.GetForwardedTargets( &targets );

    VtArray<int> indices;
    if( !m_usdPointInstancer.GetProtoIndicesAttr().Get( &indices, UsdTimeCode( frame )))
    {
        TF_WARN( "error getting indicies" );
        return false;
    }

    UT_Matrix4D instancerXform;
    GusdUSD_XformCache::GetInstance().
        GetLocalToWorldTransform( usdPrim,
                                  UsdTimeCode( frame ),
                                  instancerXform );

    VtArray<GfMatrix4d> frames;
    if( !m_usdPointInstancer.ComputeInstanceTransformsAtTime( &frames,
                                                        UsdTimeCode(frame),
                                                        UsdTimeCode(frame),
                                                        UsdGeomPointInstancer::ProtoXformInclusion::IncludeProtoXform,
                                                        UsdGeomPointInstancer::MaskApplication::IgnoreMask  ) )
    {
        TF_WARN( "ComputeFrames failed" );
        return false;
    }

    if( indices.size() != frames.size() )
    {
        TF_WARN( "Indices and frames arrays are not the same size" );
        return false;
    }

    // If the primPath of the instancer contains a variant selection,
    // copy the variant selection to the prototype paths.
    if( primPath.ContainsPrimVariantSelection() ) {

        SdfPath strippedPathHead(primPath.StripAllVariantSelections());
        for( size_t i = 0; i < targets.size(); ++ i ) {
            targets[i] = targets[i].ReplacePrefix( strippedPathHead, primPath );
        }
    }


    GA_Offset start = -1;

    for( size_t i = 0; i < indices.size(); ++i )
    {
        const int idx = indices[i];
        if( idx < 0 || idx >= targets.size() )
        {
            TF_WARN( "Invalid prototype index: %d", idx );
            continue;
        }

        GU_PrimPacked *guPrim = 
            GusdGU_PackedUSD::Build( gdr, fileName,
                                     targets[idx], 
                                     primPath,
                                     i,
                                     frame, 
                                     viewportLod,
                                     purposes );

        UT_Matrix4D m = GusdUT_Gf::Cast( frames[i] ) * xform;
        UT_Vector3D p;
        m.getTranslates( p );
        
        guPrim->setLocalTransform( UT_Matrix3D( m ) );
        guPrim->setPos3(0,p);

        if( i == 0 ) {
            start = guPrim->getPointOffset( 0 );
        }
    }


    // unpack any per-instance primvars to point attributes

    vector<UsdGeomPrimvar> authoredPrimvars = m_usdPointInstancer.GetAuthoredPrimvars();
    for( const UsdGeomPrimvar &primvar : authoredPrimvars ) {

        if( primvar.GetInterpolation() == UsdGeomTokens->constant || 
            primvar.GetInterpolation() == UsdGeomTokens->uniform ) {

            // TODO: Constant and uniform primvars need to be replicated for each 
            // instance
            TF_WARN( "%s:%s has %s interpolation. These are not supported yet.",
                     m_usdPointInstancer.GetPrim().GetPath().GetText(), 
                     primvar.GetPrimvarName().GetText(),
                     primvar.GetInterpolation().GetText() ); 
        }
        else {

            GT_DataArrayHandle pvData = GusdPrimWrapper::convertPrimvarData( primvar, UsdTimeCode( frame ) );
            GT_Storage storage = pvData->getStorage();

            if( pvData->entries() < indices.size() ) {
                TF_WARN( "Invalid primvar found: '%s:%s'. It has %zd values. It should have at least %zd.", 
                         m_usdPointInstancer.GetPrim().GetPath().GetText(),
                         primvar.GetPrimvarName().GetText(), 
                         pvData->entries(), indices.size() );
                continue;
            }

            if( storage == GT_STORE_REAL16 ||
                storage == GT_STORE_REAL32 ||
                storage == GT_STORE_REAL64 ) {

                GA_RWAttributeRef attr = 
                    gdr.addFloatTuple( GA_ATTRIB_POINT, 
                                       primvar.GetBaseName().GetString().c_str(), 
                                       pvData->getTupleSize(),
                                       GA_Defaults(0.0),
                                       /* creation_args */0,
                                       /* attribute_options */0,
                                       GT_Util::getGAStorage(storage) );

                if( attr.isValid() ) {
                    attr->setTypeInfo( GT_Util::getGAType( pvData->getTypeInfo() ));
                    
                    // AIFTuples don't support half floats. Promote them to 32 bits.
                    GT_DataArrayHandle tmp;
                    if( storage == GT_STORE_REAL16 ||
                        storage == GT_STORE_REAL32 ) {
                        attr->getAIFTuple()->setRange( 
                                attr.getAttribute(),
                                GA_Range( attr->getIndexMap(), 
                                          start, start + pvData->entries() ),
                                pvData->getF32Array( tmp ) );
                    }
                    if( storage == GT_STORE_REAL64 ) {
                        attr->getAIFTuple()->setRange( 
                                attr.getAttribute(),
                                GA_Range( attr->getIndexMap(), 
                                          start, start + pvData->entries() ),
                                pvData->getF64Array( tmp ) );
                    }
                }
            }
            else if( storage == GT_STORE_UINT8 ||
                     storage == GT_STORE_INT32 ||
                     storage == GT_STORE_INT64 ) {

                GA_RWAttributeRef attr = 
                    gdr.addIntTuple( GA_ATTRIB_POINT, 
                                     primvar.GetBaseName().GetString().c_str(), 
                                     pvData->getTupleSize(),
                                     GA_Defaults(0.0),
                                     /* creation_args */0,
                                     /* attribute_options */0,
                                     GT_Util::getGAStorage(storage) );

                if( attr.isValid() ) {
                    attr->setTypeInfo( GT_Util::getGAType( pvData->getTypeInfo() ));

                    // AIFTuples don't support 8 bit ints. promote to 32 bits.
                    GT_DataArrayHandle tmp;
                    if( storage == GT_STORE_UINT8 ||
                        storage == GT_STORE_INT32 ) {
                        attr->getAIFTuple()->setRange( 
                                attr.getAttribute(),
                                GA_Range( attr->getIndexMap(), 
                                          start, start + pvData->entries() ),
                                pvData->getI32Array( tmp ) );
                    }
                    else {
                        attr->getAIFTuple()->setRange( 
                                attr.getAttribute(),
                                GA_Range( attr->getIndexMap(), 
                                          start, start + pvData->entries() ),
                                pvData->getI64Array( tmp ) );
                    }
                }
            }
            else {

                // TODO: String primvars need to be implements.

                TF_WARN( "Found primvar with unsupported data type. %s:%s type = %s",
                         m_usdPointInstancer.GetPrim().GetPath().GetText(),
                         primvar.GetPrimvarName().GetText(),
                         primvar.GetTypeName().GetAsToken().GetText());
            }
        }
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
