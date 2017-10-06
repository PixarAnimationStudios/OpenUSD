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

void setTransformAttrsFromMatrices(const UT_Matrix4D &worldToLocal,
                                   UsdAttribute& usdPositionAttr,
                                   UsdAttribute& usdRotationAttr,
                                   UsdAttribute& usdScaleAttr,
                                   const GT_AttributeListHandle& gtAttrs,
                                   UsdTimeCode time,
                                   const UsdGeomPointInstancer& pointInstancer,
                                   std::vector<UT_Matrix4D> prototypeTransforms)
{
    if (!usdPositionAttr.IsValid() 
     || !usdRotationAttr.IsValid()
     || !usdScaleAttr.IsValid()
     || !gtAttrs)
        return;

    GT_DataArrayHandle houXformAttr = gtAttrs->get("__instancetransform", 0); 
    if(!houXformAttr || !(houXformAttr->getTupleSize() ==16))
        return;

    GT_DataArrayHandle houXformBuffer;
    const fpreal64* houXformArray = houXformAttr->getF64Array(houXformBuffer);

    if(!houXformArray)
        return;

    GT_Size numXforms = houXformAttr->entries();

    GT_Real32Array* houPositions = new GT_Real32Array(numXforms, 3);
    GT_DataArrayHandle houPositionsHandle(houPositions);

    GT_Real32Array* houRotations = new GT_Real32Array(numXforms, 4);
    GT_DataArrayHandle houRotationsHandle(houRotations);

    GT_Real32Array* houScales = new GT_Real32Array(numXforms, 3);
    GT_DataArrayHandle houScalesHandle(houScales);


    VtArray<int> indices;
    pointInstancer.GetProtoIndicesAttr().Get( &indices, time);

    // If we have transforms on prototypes, we have to remove them from our
    // final instance transformation, as the potin instancer schema accounts
    // for prototype transforms.
    bool removeProtoTransforms = (indices.size() == numXforms) &&
                                 (prototypeTransforms.size() > 0);

    for(GA_Size i=0; i<numXforms; ++i) {
        
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
            UT_Matrix4D protoXform = prototypeTransforms[protoIdx];
            protoXform.invert();
            localInstXform =  protoXform * localInstXform;
        }

        UT_Vector3 position;
        localInstXform.getTranslates( position );
        houPositions->setTuple( position.data(), i );

        UT_Matrix3D localInstXform3( localInstXform );

        UT_Vector3 scales;
        localInstXform3.extractScales(scales);
        UT_Quaternion q;
        q.updateFromRotationMatrix(localInstXform3);
        q.normalize();

        houScales->setTuple(scales.data(), i);
        // Houdini quaternions are i,j,k,w
        houRotations->setTuple(
                UT_Vector4(q.x(), q.y(), q.z(), q.w()).data(), i);
    }

    GusdGT_Utils::setUsdAttribute(usdPositionAttr, houPositionsHandle, time);
    GusdGT_Utils::setUsdAttribute(usdRotationAttr, houRotationsHandle, time);
    GusdGT_Utils::setUsdAttribute(usdScaleAttr, houScalesHandle, time);
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
    , m_usdPointInstancerForRead( usdInstancer )
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
            m_usdPointInstancerForWrite = UsdGeomPointInstancer(stage->OverridePrim( path ));
        }
        else {
            // When fracturing, we want to override the outside surfaces and create
            // new inside surfaces in one export. So if we don't find an existing prim
            // with the given path, create a new one.
            m_usdPointInstancerForWrite = UsdGeomPointInstancer::Define( stage, path );
        }
    }
    else {
        m_usdPointInstancerForWrite = UsdGeomPointInstancer::Define( stage, path );  
    }
    if( !m_usdPointInstancerForWrite || !m_usdPointInstancerForWrite.GetPrim().IsValid() ) {
        TF_WARN( "Unable to create %s instancer '%s'.", newPrim ? "new" : "override", path.GetText() );
    }
    return bool( m_usdPointInstancerForWrite );
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

    if(!ctxt.writeOverlay || ctxt.overlayAll) {
        
        // Set empty defaults for positions, scale, and indices. 
        // This prevents katana errors when expanding per-frame exports
        // with animated visibility.
        if(instancePrim) {
            VtIntArray intArray;
            instancePrim->m_usdPointInstancerForWrite.GetProtoIndicesAttr().Set(intArray, UsdTimeCode::Default());
            VtVec3fArray vec3fArray;
            VtQuathArray quathArray;
            instancePrim->m_usdPointInstancerForWrite.GetPositionsAttr().Set(vec3fArray, UsdTimeCode::Default());
            instancePrim->m_usdPointInstancerForWrite.GetScalesAttr().Set(vec3fArray, UsdTimeCode::Default());
            instancePrim->m_usdPointInstancerForWrite.GetOrientationsAttr().Set(quathArray, UsdTimeCode::Default());
        }

    }

    // Write out prototypes in usd and build an index map for prototype
    // relationships.
    instancePrim->writePrototypes( ctxt, stage, sourcePrim );

    if( ctxt.writeOverlay ) {

        // If we are writing an overlay, turn off pruning for this point instancer.
        // We may have shuffled the instance index order. 
        if( UsdAttribute attr = 
                instancePrim->m_usdPointInstancerForWrite.GetPrim().CreateAttribute( 
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

    if(!ctxt.writeOverlay || ctxt.overlayAll) {
        
        // Set empty defaults for positions, scale, and indices. 
        // This prevents katana errors when expanding per-frame exports
        // with animated visibility.
        if( m_usdPointInstancerForWrite ) {
            VtIntArray intArray;
            m_usdPointInstancerForWrite.GetProtoIndicesAttr().Set(
                    intArray, UsdTimeCode::Default());
            VtVec3fArray vec3fArray;
            VtQuathArray quathArray;
            m_usdPointInstancerForWrite.GetPositionsAttr().Set(
                    vec3fArray, UsdTimeCode::Default());
            m_usdPointInstancerForWrite.GetScalesAttr().Set(
                    vec3fArray, UsdTimeCode::Default());
            m_usdPointInstancerForWrite.GetOrientationsAttr().Set(
                quathArray, UsdTimeCode::Default());
       
        }
    }
    stage->OverridePrim( path.AppendPath( kReferenceProtoPath ) );

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
    return m_usdPointInstancerForWrite || m_usdPointInstancerForRead;
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

    if( !m_usdPointInstancerForWrite) {
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

    // Get the prim path for the root (point instancer) to use as the parent
    // scope for prototypes
    SdfPath protoPath = m_usdPointInstancerForWrite.GetPath().AppendPath(kReferenceProtoPath);

    // Collect sops containing prototypes into a list. If usdPrototypesPath
    // references a sop, it will be a length of one. If it references a subnet,
    // it will contain all the renderable sops within the net.
    std::vector<SOP_Node*> protoNodes;
    OBJ_Node* objNode;
    UT_Matrix4D localToWorldMatrix;
    double time = GusdUSD_Utils::GetNumericTime(ctxt.time);
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
    std::map<string, GU_DetailHandle> protoDetailMap;

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
            // We also support instancepath instead of usdinstancepath
            if (!instancePathAttr.isValid()) {
                instancePathAttr = GA_ROHandleS( detailLock->findAttribute(
                                                        "instancepath",
                                                        searchOrder,
                                                        4 ));
            }

            // Iteratve over each primitive and create a detail handle
            GA_Range primRange = detailLock->getPrimitiveRange();
            for(GA_Iterator offsetIt(primRange); !offsetIt.atEnd(); ++offsetIt) {
                // Use the context's usdinstancepath as default if no attributes
                string usdInstancePath = ctxt.usdInstancePath;
                if (instancePathAttr.isValid()) {
                    string instancePathAttrVal = instancePathAttr.get(offsetIt.getOffset());
                    if (!instancePathAttrVal.empty()) {
                        usdInstancePath = instancePathAttrVal;
                    }
                }
                if (!usdInstancePath.empty()) {
                    const GU_Detail *srcDetail = detailLock.getGdp();
                    GA_PrimitiveGroup primGroup(*srcDetail);
                    primGroup.addOffset(offsetIt.getOffset());
                    // Create a detail based on a prim group for each primitive
                    GU_Detail *detail = new GU_Detail(srcDetail, &primGroup);
                    GU_DetailHandle detailHandle;
                    // Handle owns the detail so it will free the memory
                    detailHandle.allocateAndSet(detail, true);
                    if(detailHandle) {
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
        // TODO: add custom scoping for prototypes
        string protoUsdName("prototype_");
        protoUsdName += std::to_string(protoIdx);
        protoIdx++;
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
        protoPathsMap[pair.first] = protoUsdPath;
    }

    if(ctxt.writeOverlay && (!ctxt.overlayAll || usdPrototypesPath.empty())) {

        // If we are doing an overlay, build the map from the existing relationships
        UsdRelationship prototypesRel = m_usdPointInstancerForWrite.GetPrototypesRel();
        SdfPathVector targets;
        prototypesRel.GetForwardedTargets(&targets);
        for(size_t i=0; i<targets.size(); ++i) {
            m_relationshipIndexMap[targets[i].GetNameToken()] = i;
        }
    }
    UsdRelationship prototypesRel = m_usdPointInstancerForWrite.GetPrototypesRel();
    
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
    if (ctxt.overlayAll) {
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
    if( !m_usdPointInstancerForWrite ) {
        TF_WARN( "Can't update USD point instancer from GT prim '%s'", m_usdPointInstancerForWrite.GetPrim().GetPath().GetText() );
        return false;
    }

    DBG(cerr << "GusdInstanceWrapper::updateFromGTPrim, " << 
            m_usdPointInstancerForWrite.GetPath().GetString() << endl);

    bool writeTransforms = !ctxt.writeOverlay || ctxt.overlayAll || 
                                    ctxt.overlayPoints || ctxt.overlayTransforms;

    //--------------------------------------------------------------------------

    GfMatrix4d xform = computeTransform (
                            m_usdPointInstancerForWrite.GetPrim().GetParent(),
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

        // TODO If overlayTransforms, should ensure points array is the same
        // length.

        UT_Matrix4D localToWorld;
        GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
            m_usdPointInstancerForWrite.GetPrim(),
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
        maskAtTime = m_usdPointInstancerForWrite.ComputeMaskAtTime(ctxt.time);
        if( !maskAtTime.empty() ) {
            m_usdPointInstancerForWrite.ActivateAllIds();
            m_usdPointInstancerForWrite.VisAllIds(ctxt.time);
        }

        // Indices
        usdAttr = m_usdPointInstancerForWrite.GetProtoIndicesAttr();

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
        } else if (!ctxt.writeOverlay || ctxt.overlayAll) {
            TF_WARN( "Instance prototypes not specified as instance path or packed prim" );
        }

        // Set indicies array
        if(gotValidIndices && usdAttr) {
            GusdGT_Utils::setUsdAttribute(usdAttr, houAttr, ctxt.time);
        }

        // v
        houAttr = sourcePrim->findAttribute("v", attrOwner, 0);
        usdAttr = m_usdPointInstancerForWrite.GetVelocitiesAttr();
        if(houAttr && usdAttr) {
            GusdGT_Utils::setUsdAttribute(usdAttr, houAttr, ctxt.time);
        }

        // w
        houAttr = sourcePrim->findAttribute("w", attrOwner, 0);
        usdAttr = m_usdPointInstancerForWrite.GetAngularVelocitiesAttr();
        if(houAttr && usdAttr) {

            // Houdini stores angular velocity in radians per second.
            // USD is degrees per second
            const GT_Size numVals = houAttr->entries() * houAttr->getTupleSize();
            std::vector<fpreal32> wArray(numVals);
            houAttr->fillArray(wArray.data(), 0, numVals, houAttr->getTupleSize()); 

            std::transform(
                wArray.begin(),
                wArray.end(),
                wArray.begin(),
                std::bind1st(std::multiplies<fpreal32>(), 180.0 / M_PI));

            houAttr.reset(new GT_Real32Array(wArray.data(), houAttr->entries() , houAttr->getTupleSize()));
            GusdGT_Utils::setUsdAttribute(usdAttr, houAttr, ctxt.time);
        }

        // instance rotation and scale
        UsdAttribute usdPositionAttr = m_usdPointInstancerForWrite.GetPositionsAttr();
        UsdAttribute usdRotationAttr = m_usdPointInstancerForWrite.GetOrientationsAttr();
        UsdAttribute usdScalesAttr = m_usdPointInstancerForWrite.GetScalesAttr();
        const GT_AttributeListHandle gtPointAttrs = sourcePrim->getPointAttributes();

        if (usdRotationAttr 
         && usdScalesAttr
         && gtPointAttrs) {

            if( gtPointAttrs->hasName("__instancetransform") ) {
                setTransformAttrsFromMatrices(
                                  worldToLocal,
                                  usdPositionAttr,
                                  usdRotationAttr,
                                  usdScalesAttr,
                                  gtPointAttrs,
                                  ctxt.time,
                                  m_usdPointInstancerForWrite,
                                  m_prototypeTransforms);
            }
            else {
                setTransformAttrsFromComponents(
                                  usdPositionAttr,
                                  usdRotationAttr,
                                  usdScalesAttr,
                                  gtPointAttrs, ctxt.time);
            }
        }
    
        // extent ------------------------------------------------------------------

        VtVec3fArray extent(2);
        // Using utility function from UsdGeomPointInstancer
        if(m_usdPointInstancerForWrite.ComputeExtentAtTime(&extent, ctxt.time, ctxt.time)) {
            m_usdPointInstancerForWrite.GetExtentAttr().Set(extent, ctxt.time);
        }
    }

    // primvars ----------------------------------------------------------------
    
    if( !ctxt.writeOverlay || ctxt.overlayAll || ctxt.overlayPrimvars ) {

        GusdGT_AttrFilter filter = ctxt.attributeFilter;
        // Filter attributes which were used to construct the instance transform
        // and prototype relationships.
        filter.appendPattern(GT_OWNER_POINT,
                "^__* ^orient ^rot ^scale ^instancepath ^usdinstancepath \
                 ^usdprototypespath ^trans ^up");
        filter.appendPattern(GT_OWNER_POINT, "^P ^N ^v");
        filter.appendPattern(GT_OWNER_CONSTANT, "^usdprimpath \
                 ^instancepath ^usdinstancepath ^usdprototypespath");
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

bool GusdInstancerWrapper::
refine( GT_Refine& refiner,
        const GT_RefineParms* parms ) const
{
    const UsdGeomPointInstancer& pointInstancer = m_usdPointInstancerForRead;

    UsdStagePtr stage = pointInstancer.GetPrim().GetStage();

    DBG(cerr << "GusdInstancerWrapper::refine, " << pointInstancer.GetPrim().GetPath() << endl);
    
    UsdRelationship relationship = pointInstancer.GetPrototypesRel();
    SdfPathVector targets;
    relationship.GetForwardedTargets( &targets );

    // Build prototype prims on demand
    vector<GT_PrimitiveHandle> protoPrims( targets.size() );

    VtArray<int> indices;
    if( !pointInstancer.GetProtoIndicesAttr().Get( &indices, m_time ) )
    {
        TF_WARN("error getting indices attribute");
        return false;
    }

    VtArray<GfMatrix4d> frames;
    if( !pointInstancer.ComputeInstanceTransformsAtTime( &frames,
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

    const UsdGeomPointInstancer& instancerPrim = m_usdPointInstancerForRead;
    UsdPrim usdPrim = instancerPrim.GetPrim();

    UsdRelationship relationship = instancerPrim.GetPrototypesRel();
    SdfPathVector targets;
    relationship.GetForwardedTargets( &targets );

    VtArray<int> indices;
    if( !instancerPrim.GetProtoIndicesAttr().Get( &indices, UsdTimeCode( frame )))
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
    if( !instancerPrim.ComputeInstanceTransformsAtTime( &frames,
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
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
