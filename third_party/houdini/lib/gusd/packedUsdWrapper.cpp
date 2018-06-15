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
#include "packedUsdWrapper.h"

#include "pxr/usd/usd/variantSets.h"
#include "pxr/base/tf/staticTokens.h"

#include "gusd/context.h"

#include "GT_PackedUSD.h"
#include "GU_PackedUSD.h"

PXR_NAMESPACE_OPEN_SCOPE

using std::cout;
using std::cerr;
using std::endl;
using std::string;

GusdPackedUsdWrapper::GusdPackedUsdWrapper(
        const UsdStagePtr& stage,
        const SdfPath& primPath,
        bool isOverride )
{ 

    initUsdPrim( stage, primPath, isOverride );
}

GusdPackedUsdWrapper::GusdPackedUsdWrapper(const GusdPackedUsdWrapper& other )
    : GusdPrimWrapper(other)
    , m_primRef(other.m_primRef)
{
}

GusdPackedUsdWrapper::
~GusdPackedUsdWrapper()
{}

bool GusdPackedUsdWrapper::
initUsdPrim(const UsdStagePtr& stage,
            const SdfPath& path,
            bool asOverride)
{
    if( asOverride ) {
        m_primRef = stage->OverridePrim( path );
        if( !m_primRef || !m_primRef.IsValid() ) {
            TF_WARN( "Unable to create override prim '%s'.", path.GetText() );
        }
    }
    else {

        SdfPath parentPath = path.GetParentPath();
        UsdPrim parent = stage->GetPrimAtPath( parentPath );
        if( !parent )
            UsdGeomXform::Define( stage, parentPath );

        m_primRef = stage->DefinePrim( path );
    }
    return bool( m_primRef );
}

GT_PrimitiveHandle GusdPackedUsdWrapper::
defineForWrite(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        const GusdContext& ctxt)
{
    return new GusdPackedUsdWrapper( stage, path, ctxt.writeOverlay );
}

bool GusdPackedUsdWrapper::
redefine(const UsdStagePtr& stage,
         const SdfPath& path,
         const GusdContext& ctxt,
         const GT_PrimitiveHandle& sourcePrim)
{
    SdfPath parentPath = path.GetParentPath();
    UsdPrim parent = stage->GetPrimAtPath( parentPath );
    if( !parent )
        UsdGeomXform::Define( stage, parentPath );

    m_primRef = stage->DefinePrim( path );
    clearCaches();
    return true;
}


const char* GusdPackedUsdWrapper::
className() const
{
    return "GusdPackedUsdWrapper";
}


void GusdPackedUsdWrapper::
enlargeBounds(UT_BoundingBox boxes[], int nsegments) const
{
    // TODO
}


int GusdPackedUsdWrapper::
getMotionSegments() const
{
    // TODO
    return 1;
}


int64 GusdPackedUsdWrapper::
getMemoryUsage() const
{
    // TODO
    return 0;
}


GT_PrimitiveHandle GusdPackedUsdWrapper::
doSoftCopy() const
{
    return GT_PrimitiveHandle(new GusdPackedUsdWrapper(*this));
}


bool 
GusdPackedUsdWrapper::isValid() const
{
    return m_primRef;
}


bool 
GusdPackedUsdWrapper::updateFromGTPrim(
    const GT_PrimitiveHandle& sourcePrim,
    const UT_Matrix4D&        houXform,
    const GusdContext&        ctxt,
    GusdSimpleXformCache&     xformCache)
{
    if( !m_primRef )
        return false;

    GusdGT_PackedUSD* gtPackedUSD
        = dynamic_cast<GusdGT_PackedUSD*>(sourcePrim.get());
    if(!gtPackedUSD)  {
        TF_WARN( "source prim is not a packed USD prim. '%s'", m_primRef.GetPrim().GetPath().GetText() );   
        return false;
    }

    if( !ctxt.writeOverlay ) {

        string fileName = gtPackedUSD->getAuxFileName().toStdString();
        if( fileName.empty() )
            fileName = gtPackedUSD->getFileName().toStdString();

        SdfPath variantPrimPath = gtPackedUSD->getPrimPath();
        SdfPath primPath = variantPrimPath.StripAllVariantSelections();

        // Get Layer Offset values from context in case set as node paramaters.
        fpreal usdTimeOffset = ctxt.usdTimeOffset;
        fpreal usdTimeScale = ctxt.usdTimeScale;
        GT_Owner owner;
        GT_DataArrayHandle usdTimeOffsetAttr;
        GT_DataArrayHandle usdTimeScaleAttr;
        // If attributes exists for Layer Offset values, override the context values.
        usdTimeOffsetAttr = sourcePrim->findAttribute("usdtimeoffset", owner, 0);
        if(usdTimeOffsetAttr != NULL ) {
            usdTimeOffset = usdTimeOffsetAttr->getF64(0);
        }
        usdTimeScaleAttr = sourcePrim->findAttribute("usdtimescale", owner, 0);
        if(usdTimeScaleAttr != NULL ) {
            usdTimeScale = usdTimeScaleAttr->getF64(0);
        }
        // Create a layer offset for retiming references.
        SdfLayerOffset layerOffset = SdfLayerOffset(usdTimeOffset, usdTimeScale);

        // Add the reference. Layer offset will only appear if not default values.
        m_primRef.GetReferences().AddReference(fileName, primPath, layerOffset );

        // Set variant selections.
        if(ctxt.authorVariantSelections &&
                        variantPrimPath.ContainsPrimVariantSelection()) {
            SdfPath p(variantPrimPath);

            while(p != SdfPath::EmptyPath()) {

                if(p.IsPrimVariantSelectionPath()) {
                    auto vPair = p.GetVariantSelection();
                    if(p.StripAllVariantSelections().IsRootPrimPath()) {
                        m_primRef.GetVariantSet( vPair.first )
                                        .SetVariantSelection( vPair.second );
                    }
                    else {
                        // FIXME I don't think this is working.
                        UsdPrim prim = m_primRef.GetStage()->OverridePrim(p.GetPrimPath());
                        prim.GetVariantSet(vPair.first)
                            .SetVariantSelection(vPair.second);
                    }
                }
                
                p = p.GetParentPath();
            }
        }

        if( ctxt.purpose != UsdGeomTokens->default_ ) {
            UsdGeomImageable( m_primRef ).GetPurposeAttr().Set( ctxt.purpose );
        }

        // Make instanceable
        if( ctxt.makeRefsInstanceable ) {
            m_primRef.SetInstanceable( true );
        }
    }

    updateVisibilityFromGTPrim(sourcePrim, ctxt.time, 
                               (!ctxt.writeOverlay || ctxt.overlayAll) && 
                                ctxt.granularity == GusdContext::PER_FRAME );

    // transform ---------------------------------------------------------------

    if( !ctxt.writeOverlay || ctxt.overlayAll || 
        ctxt.overlayPoints || ctxt.overlayTransforms ) {

        GfMatrix4d xform = computeTransform( 
                                m_primRef.GetPrim().GetParent(),
                                ctxt.time,
                                houXform,
                                xformCache );

        updateTransformFromGTPrim( xform, ctxt.time, 
                                   ctxt.granularity == GusdContext::PER_FRAME );
    }
    return GusdPrimWrapper::updateFromGTPrim(sourcePrim, houXform, ctxt, xformCache);
}

PXR_NAMESPACE_CLOSE_SCOPE

