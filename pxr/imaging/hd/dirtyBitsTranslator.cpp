//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/dirtyBitsTranslator.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/dataSourceLegacyPrim.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/coordSys.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/field.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/renderBuffer.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/categoriesSchema.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/geomSubsetSchema.h"
#include "pxr/imaging/hd/geomSubsetsSchema.h"
#include "pxr/imaging/hd/instanceCategoriesSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/renderBufferSchema.h"
#include "pxr/imaging/hd/subdivisionTagsSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"
#include "pxr/imaging/hd/volumeFieldSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

/*static*/
void
HdDirtyBitsTranslator::RprimDirtyBitsToLocatorSet(TfToken const& primType,
    const HdDirtyBits bits, HdDataSourceLocatorSet *set)
{
    if (ARCH_UNLIKELY(set == nullptr)) {
        return;
    }

    if (bits == HdChangeTracker::AllDirty) {
        set->append(HdDataSourceLocator::EmptyLocator());
        return;
    }

    // To minimize the cost of building the locator set, we append to the set
    // in the locator-defined order. If you add to this function, make sure
    // you sort the addition by locator name, so as not to slow down append.
    // Also note, this should match RprimLocatorSetToDirtyBits.

    if (primType == HdPrimTypeTokens->basisCurves) {
        if (bits & HdChangeTracker::DirtyTopology) {
            // could either be topology or geomsubsets
            set->append(HdBasisCurvesSchema::GetDefaultLocator());
        }
    }

    if (bits & HdChangeTracker::DirtyDisplayStyle) {
        set->append(HdLegacyDisplayStyleSchema::GetDefaultLocator());
    } else {
        if (bits & HdChangeTracker::DirtyCullStyle) {
            set->append(HdLegacyDisplayStyleSchema::GetCullStyleLocator());
        }
        if (bits & HdChangeTracker::DirtyRepr) {
            set->append(HdLegacyDisplayStyleSchema::GetReprSelectorLocator());
        }
    }

    if (bits & HdChangeTracker::DirtyExtent) {
        set->append(HdExtentSchema::GetDefaultLocator());
    }

    if (bits & HdChangeTracker::DirtyPrimvar) {
        set->append(HdExtComputationPrimvarsSchema::GetDefaultLocator());
    }

    if (bits & HdChangeTracker::DirtyInstancer) {
        set->append(HdInstancedBySchema::GetDefaultLocator());
    }

    if (bits & HdChangeTracker::DirtyInstanceIndex) {
        set->append(HdInstancerTopologySchema::GetDefaultLocator());
    }

    if (bits & HdChangeTracker::DirtyMaterialId) {
        set->append(HdMaterialBindingSchema::GetDefaultLocator());
    }

    if (primType == HdPrimTypeTokens->mesh) {
        if (bits & HdChangeTracker::DirtyDoubleSided) {
            set->append(HdMeshSchema::GetDoubleSidedLocator());
        }

        if (bits & HdChangeTracker::DirtyTopology) {
            set->append(HdMeshSchema::GetGeomSubsetsLocator());
            set->append(HdMeshSchema::GetSubdivisionSchemeLocator());
        }

        if (bits & HdChangeTracker::DirtySubdivTags) {
            set->append(HdSubdivisionTagsSchema::GetDefaultLocator());
        }

        if (bits & HdChangeTracker::DirtyTopology) {
            set->append(HdMeshTopologySchema::GetDefaultLocator());
        }
    }

    if (bits & HdChangeTracker::DirtyPrimvar) {
        set->append(HdPrimvarsSchema::GetDefaultLocator());
    } else {
        if (bits & HdChangeTracker::DirtyNormals) {
            set->append(HdPrimvarsSchema::GetNormalsLocator());
        }
        if (bits & HdChangeTracker::DirtyPoints) {
            set->append(HdPrimvarsSchema::GetPointsLocator());
        }
        if (bits & HdChangeTracker::DirtyWidths) {
            set->append(HdPrimvarsSchema::GetWidthsLocator());
        }
    }

    if (bits & HdChangeTracker::DirtyRenderTag) {
        set->append(HdPurposeSchema::GetDefaultLocator());
    }

    if (bits & HdChangeTracker::DirtyVisibility) {
        set->append(HdVisibilitySchema::GetDefaultLocator());
    }

    if (bits & HdChangeTracker::DirtyVolumeField) {
        set->append(HdVolumeFieldBindingSchema::GetDefaultLocator());
    }

    if (bits & HdChangeTracker::DirtyTransform) {
        set->append(HdXformSchema::GetDefaultLocator());
    }
}

/* static */
void
HdDirtyBitsTranslator::SprimDirtyBitsToLocatorSet(TfToken const& primType,
    const HdDirtyBits bits, HdDataSourceLocatorSet *set)
{
    if (ARCH_UNLIKELY(set == nullptr)) {
        return;
    }

    // To minimize the cost of building the locator set, we append to the set
    // in the locator-defined order. If you add to this function, make sure
    // you sort the addition by locator name, so as not to slow down append.
    // Also note, this should match SprimLocatorSetToDirtyBits.

    if (primType == HdPrimTypeTokens->material) {
        if (bits & HdMaterial::AllDirty) {
            set->append(HdMaterialSchema::GetDefaultLocator());
        }
    } else if (primType == HdPrimTypeTokens->coordSys) {
        if (bits & HdCoordSys::DirtyTransform) {
            set->append(HdXformSchema::GetDefaultLocator());
        }
    } else if (primType == HdPrimTypeTokens->camera) {
        if (bits & (HdCamera::DirtyProjMatrix |
                    HdCamera::DirtyWindowPolicy |
                    HdCamera::DirtyClipPlanes |
                    HdCamera::DirtyParams)) {
            set->append(HdCameraSchema::GetDefaultLocator());
        }
        if (bits & HdCamera::DirtyTransform) {
            set->append(HdXformSchema::GetDefaultLocator());
        }
    } else if (HdPrimTypeIsLight(primType)) {
        if (bits & (HdLight::DirtyParams |
                    HdLight::DirtyShadowParams |
                    HdLight::DirtyCollection)) {
            set->append(HdLightSchema::GetDefaultLocator());
        }
        if (bits & HdLight::DirtyResource) {
            set->append(HdMaterialSchema::GetDefaultLocator());
        }
        // XXX: Right now, primvars don't seem to have a dirty bit, so
        //      group them with params...
        if (bits & HdLight::DirtyParams) {
            set->append(HdPrimvarsSchema::GetDefaultLocator());
        }
        // XXX: Some delegates seems to be sending light visibility
        //      changes on the child guide mesh instead of here. So,
        //      for now, let's consider dirty light params to include
        //      light visibility
        if (bits & (HdChangeTracker::DirtyVisibility | HdLight::DirtyParams)) {
            set->append(HdVisibilitySchema::GetDefaultLocator());
        }
        if (bits & HdLight::DirtyTransform) {
            set->append(HdXformSchema::GetDefaultLocator());
        }
    } else if (primType == HdPrimTypeTokens->drawTarget) {
        const static HdDataSourceLocator locator(
                HdPrimTypeTokens->drawTarget);
        if (bits) {
            set->append(locator);
        }
    } else if (primType == HdPrimTypeTokens->extComputation) {
        if (bits & HdExtComputation::DirtyDispatchCount) {
            set->append(HdExtComputationSchema::GetDispatchCountLocator());
        }
        if (bits & HdExtComputation::DirtyElementCount) {
            set->append(HdExtComputationSchema::GetElementCountLocator());
        }
        if (bits & HdExtComputation::DirtyKernel) {
            set->append(HdExtComputationSchema::GetGlslKernelLocator());
        }
        if (bits & (HdExtComputation::DirtyInputDesc |
                    HdExtComputation::DirtySceneInput)) {
            set->append(HdExtComputationSchema::GetInputComputationsLocator());
            set->append(HdExtComputationSchema::GetInputValuesLocator());
        }
        if (bits & HdExtComputation::DirtyOutputDesc) {
            set->append(HdExtComputationSchema::GetOutputsLocator());
        }
    }
}

/*static*/
void
HdDirtyBitsTranslator::InstancerDirtyBitsToLocatorSet(TfToken const& primType,
    const HdDirtyBits bits, HdDataSourceLocatorSet *set)
{
    if (ARCH_UNLIKELY(set == nullptr)) {
        return;
    }

    // To minimize the cost of building the locator set, we append to the set
    // in the locator-defined order. If you add to this function, make sure
    // you sort the addition by locator name, so as not to slow down append.
    // Also note, this should match InstancerLocatorSetToDirtyBits.

    if (bits == HdChangeTracker::AllDirty) {
        set->append(HdDataSourceLocator::EmptyLocator());
        return;
    }

    if (bits & HdChangeTracker::DirtyInstancer) {
        set->append(HdInstancedBySchema::GetDefaultLocator());
    }
    if (bits & HdChangeTracker::DirtyInstanceIndex) {
        set->append(HdInstancerTopologySchema::GetDefaultLocator());
    }
    if (bits & HdChangeTracker::DirtyPrimvar) {
        set->append(HdPrimvarsSchema::GetDefaultLocator());
    }
    if (bits & HdChangeTracker::DirtyTransform) {
        set->append(HdXformSchema::GetDefaultLocator());
    }
}

/*static*/
void
HdDirtyBitsTranslator::BprimDirtyBitsToLocatorSet(TfToken const& primType,
    const HdDirtyBits bits, HdDataSourceLocatorSet *set)
{
    if (ARCH_UNLIKELY(set == nullptr)) {
        return;
    }

    // To minimize the cost of building the locator set, we append to the set
    // in the locator-defined order. If you add to this function, make sure
    // you sort the addition by locator name, so as not to slow down append.
    // Also note, this should match BprimLocatorSetToDirtyBits.

    if (primType == HdPrimTypeTokens->renderBuffer) {
        if (bits & HdRenderBuffer::DirtyDescription) {
            set->append(HdRenderBufferSchema::GetDefaultLocator());
        }
    } else if (HdLegacyPrimTypeIsVolumeField(primType)) {
        if (bits & HdField::DirtyParams) {
            set->append(HdVolumeFieldSchema::GetDefaultLocator());
        }
        // XXX: DirtyTransform seems unused...
    }
}

// ----------------------------------------------------------------------------

static bool
_FindLocator(HdDataSourceLocator const& locator,
             HdDataSourceLocatorSet::const_iterator const& end,
             HdDataSourceLocatorSet::const_iterator *it,
             const bool advanceToNext = true)
{
    if (*it == end) {
        return false;
    }

    // The range between *it and end can be divided into:
    // 1.) items < locator and not a prefix.
    // 2.) items < locator and a prefix.
    // 3.) locator
    // 4.) items > locator and a suffix.
    // 5.) items > locator and not a suffix.

    // We want to return true if sets [2-4] are nonempty.
    // If (advanceToNext) is true, we leave it pointing at the first element
    // of 5; otherwise, we leave it pointing at the first element of [2-4].
    bool found = false;
    for (; (*it) != end; ++(*it)) {
        if ((*it)->Intersects(locator)) {
            found = true;
            if (!advanceToNext) {
                break;
            }
        } else if (locator < (**it)) {
            break;
        }
    }
    return found;
}

/*static*/
HdDirtyBits
HdDirtyBitsTranslator::RprimLocatorSetToDirtyBits(
    TfToken const& primType, HdDataSourceLocatorSet const& set)
{
    HdDataSourceLocatorSet::const_iterator it = set.begin();

    if (it == set.end()) {
        return HdChangeTracker::Clean;
    }

    // If the empty locator is in the set, there shouldn't be any other elements
    // in the set...
    if (*it == HdDataSourceLocator::EmptyLocator()) {
        return HdChangeTracker::AllDirty;
    }

    HdDataSourceLocatorSet::const_iterator end = set.end();
    HdDirtyBits bits = HdChangeTracker::Clean;

    // Note, for efficiency we search for locators in the set in order, so that
    // we only end up making one trip through the set. If you add to this
    // function, make sure you sort the addition by locator name, or
    // _FindLocator won't work.
    // Also note, this should match RprimDirtyBitsToLocatorSet

    // _FindLocator here is called with advanceToNext = true. It will advance
    // "it" from the current position to the first element where it > locator
    // and !it.HasPrefix(locator).
    // If any of the iterated elements intersect locator, it returns true.
    // Here: search for the locator "basisCurvesTopology" in the set; if a child
    // or parent (such as "" or "basisCurvesTopology/curveType") is present,
    // mark DirtyToplogy.  it points to the next element after
    // "basisCurvesTopology", setting us up to check for displayStyle.
    if (primType == HdPrimTypeTokens->basisCurves) {
        if (_FindLocator(HdBasisCurvesSchema::GetGeomSubsetsLocator(),
                         end, &it)) {
            bits |= HdChangeTracker::DirtyTopology;
        }

        if (_FindLocator(HdBasisCurvesTopologySchema::GetDefaultLocator(),
                         end, &it)) {
            bits |= HdChangeTracker::DirtyTopology;
        }
    }

    // _FindLocator here is called with advanceToNext = false. It will advance
    // "it" from the current position to the first element where either
    // it.Intersects(locator) OR (it > locator and !it.HasPrefix), returning
    // true and false respectively.
    // Here: we look for "displayStyle". If the return value is false, there
    // are no parents or children of displayStyle and we start the test for
    // the next item on the line below, with it > "displayStyle".
    // If the return value is true, we either have a prefix of "displayStyle"
    // (such as "" or "displayStyle"), in which case we mark a bunch of bits;
    // or we have a strict suffix such as "displayStyle/cullStyle". If we have
    // a suffix, we can match it to a dirty bit (such as DirtyCullStyle); we
    // iterate through other suffixes, such as "displayStyle/reprSelector",
    // until it no longer intersects "displayStyle", at which point it's
    // also guaranteed to be > "displayStyle" as well.
    if (_FindLocator(HdLegacyDisplayStyleSchema::GetDefaultLocator(), end, &it,
                     false)) {
        if (HdLegacyDisplayStyleSchema::GetDefaultLocator().HasPrefix(*it)) {
            bits |= HdChangeTracker::DirtyDisplayStyle |
                    HdChangeTracker::DirtyCullStyle |
                    HdChangeTracker::DirtyRepr;
        } else {
            do {
                if (it->HasPrefix(
                        HdLegacyDisplayStyleSchema::GetCullStyleLocator())) {
                    bits |= HdChangeTracker::DirtyCullStyle;
                } else if (it->HasPrefix(
                        HdLegacyDisplayStyleSchema::GetReprSelectorLocator())) {
                    bits |= HdChangeTracker::DirtyRepr;
                } else {
                    bits |= HdChangeTracker::DirtyDisplayStyle;
                }
                ++it;
            } while(it != end && it->Intersects(
                        HdLegacyDisplayStyleSchema::GetDefaultLocator()));
        }
    }

    if (_FindLocator(HdExtentSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyExtent;
    }

    if (_FindLocator(HdExtComputationPrimvarsSchema::GetDefaultLocator(),
                end, &it)) {
        bits |= HdChangeTracker::DirtyPrimvar;
    }

    if (_FindLocator(HdInstancedBySchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyInstancer;
    }

    if (_FindLocator(HdInstancerTopologySchema::GetDefaultLocator(), end, &it)){
        bits |= HdChangeTracker::DirtyInstanceIndex;
    }

    if (_FindLocator(HdMaterialBindingSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyMaterialId;
    }

    if (primType == HdPrimTypeTokens->mesh) {

        if (_FindLocator(HdMeshSchema::GetDoubleSidedLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyDoubleSided;
        }

        if (_FindLocator(HdMeshSchema::GetGeomSubsetsLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyTopology;
        }

        if (_FindLocator(HdMeshSchema::GetSubdivisionSchemeLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyTopology;
        }

        if (_FindLocator(HdMeshSchema::GetSubdivisionTagsLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtySubdivTags;
        }

        if (_FindLocator(HdMeshTopologySchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyTopology;
        }
    }

    if (_FindLocator(HdPrimvarsSchema::GetDefaultLocator(), end, &it, false)) {
        // NOTE: this potentially over-invalidates; "primvars" will map to
        // DirtyPrimvar | DirtyPoints.  Importantly, we make sure that
        // "primvars/points" only maps to DirtyPoints, rather than DirtyPrimvar.
        if (HdPrimvarsSchema::GetDefaultLocator().HasPrefix(*it)) {
            bits |= HdChangeTracker::DirtyPrimvar |
                HdChangeTracker::DirtyNormals |
                HdChangeTracker::DirtyPoints |
                HdChangeTracker::DirtyWidths;
        } else {
            do {
                if (it->HasPrefix(
                        HdPrimvarsSchema::GetNormalsLocator())) {
                    bits |= HdChangeTracker::DirtyNormals;
                } else if (it->HasPrefix(
                        HdPrimvarsSchema::GetPointsLocator())) {
                    bits |= HdChangeTracker::DirtyPoints;
                } else if (it->HasPrefix(
                        HdPrimvarsSchema::GetWidthsLocator())) {
                    bits |= HdChangeTracker::DirtyWidths;
                } else {
                    bits |= HdChangeTracker::DirtyPrimvar;
                }
                ++it;
            } while (it != end && it->Intersects(
                        HdPrimvarsSchema::GetDefaultLocator()));
        }
    }

    if (_FindLocator(HdPurposeSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyRenderTag;
    }

    if (_FindLocator(HdVisibilitySchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyVisibility;
    }

    if (_FindLocator(HdVolumeFieldBindingSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyVolumeField;
    }

    if (_FindLocator(HdXformSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyTransform;
    }

    return bits;
}

/*static*/
HdDirtyBits
HdDirtyBitsTranslator::SprimLocatorSetToDirtyBits(
    TfToken const& primType, HdDataSourceLocatorSet const& set)
{
    HdDataSourceLocatorSet::const_iterator it = set.begin();

    if (it == set.end()) {
        return HdChangeTracker::Clean;
    }

    HdDataSourceLocatorSet::const_iterator end = set.end();
    HdDirtyBits bits = HdChangeTracker::Clean;

    // Note, for efficiency we search for locators in the set in order, so that
    // we only end up making one trip through the set. If you add to this
    // function, make sure you sort the addition by locator name, or
    // _FindLocator won't work.
    // Also note, this should match SprimDirtyBitsToLocatorSet

    if (primType == HdPrimTypeTokens->material) {
        if (_FindLocator(HdMaterialSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdMaterial::AllDirty;
        }
    } else if (primType == HdPrimTypeTokens->coordSys) {
        if (_FindLocator(HdXformSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdCoordSys::DirtyTransform;
        }
    } else if (primType == HdPrimTypeTokens->camera) {
        if (_FindLocator(HdCameraSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdCamera::DirtyProjMatrix |
                HdCamera::DirtyWindowPolicy |
                HdCamera::DirtyClipPlanes |
                HdCamera::DirtyParams;
        }
        if (_FindLocator(HdXformSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdCamera::DirtyTransform;
        }
    } else if (HdPrimTypeIsLight(primType)) {
        if (_FindLocator(HdLightSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdLight::DirtyParams |
                HdLight::DirtyShadowParams |
                HdLight::DirtyCollection;
        }
        if (_FindLocator(HdMaterialSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdLight::DirtyResource;
        }
        if (_FindLocator(HdPrimvarsSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdLight::DirtyParams;
        }
        if (_FindLocator(HdVisibilitySchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyVisibility | HdLight::DirtyParams;
        }
        if (_FindLocator(HdXformSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdLight::DirtyTransform;
        }
    } else if (primType == HdPrimTypeTokens->drawTarget) {
        const static HdDataSourceLocator locator(
                HdPrimTypeTokens->drawTarget);
        if (_FindLocator(locator, end, &it)) {
            bits |= HdChangeTracker::AllDirty;
        }
    } else if (primType == HdPrimTypeTokens->extComputation) {
        if (_FindLocator(HdExtComputationSchema::GetDefaultLocator(),
                    end, &it, false)) {
            if (HdExtComputationSchema::GetDefaultLocator().HasPrefix(*it)) {
                bits |= HdExtComputation::DirtyDispatchCount |
                    HdExtComputation::DirtyElementCount |
                    HdExtComputation::DirtyKernel |
                    HdExtComputation::DirtyInputDesc |
                    HdExtComputation::DirtySceneInput |
                    HdExtComputation::DirtyOutputDesc;
            } else {
                do {
                    if (it->HasPrefix(
                        HdExtComputationSchema::GetDispatchCountLocator())) {
                        bits |= HdExtComputation::DirtyDispatchCount;
                    }
                    if (it->HasPrefix(
                        HdExtComputationSchema::GetElementCountLocator())) {
                        bits |= HdExtComputation::DirtyElementCount;
                    }
                    if (it->HasPrefix(
                        HdExtComputationSchema::GetGlslKernelLocator())) {
                        bits |= HdExtComputation::DirtyKernel;
                    }
                    if (it->HasPrefix(
                        HdExtComputationSchema::GetInputValuesLocator()) ||
                        it->HasPrefix(
                        HdExtComputationSchema::GetInputComputationsLocator())){
                        bits |= HdExtComputation::DirtyInputDesc |
                            HdExtComputation::DirtySceneInput;
                    }
                    if (it->HasPrefix(
                        HdExtComputationSchema::GetOutputsLocator())) {
                        bits |= HdExtComputation::DirtyOutputDesc;
                    }
                    ++it;
                } while(it != end && it->Intersects(
                            HdExtComputationSchema::GetDefaultLocator()));
            }
        }
    }

    return bits;
}

/*static*/
HdDirtyBits
HdDirtyBitsTranslator::InstancerLocatorSetToDirtyBits(
    TfToken const& primType, HdDataSourceLocatorSet const& set)
{
    HdDataSourceLocatorSet::const_iterator it = set.begin();

    if (it == set.end()) {
        return HdChangeTracker::Clean;
    }

    // Note, for efficiency we search for locators in the set in order, so that
    // we only end up making one trip through the set. If you add to this
    // function, make sure you sort the addition by locator name, or
    // _FindLocator won't work.
    // Also note, this should match InstancerDirtyBitsToLocatorSet

    if (*it == HdDataSourceLocator::EmptyLocator()) {
        return HdChangeTracker::AllDirty;
    }

    HdDataSourceLocatorSet::const_iterator end = set.end();
    HdDirtyBits bits = HdChangeTracker::Clean;

    if (_FindLocator(HdInstancedBySchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyInstancer;
    }
    if (_FindLocator(HdInstancerTopologySchema::GetDefaultLocator(), end, &it)){
        bits |= HdChangeTracker::DirtyInstanceIndex;
    }
    if (_FindLocator(HdPrimvarsSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyPrimvar;
    }
    if (_FindLocator(HdXformSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyTransform;
    }

    return bits;
}

/*static*/
HdDirtyBits
HdDirtyBitsTranslator::BprimLocatorSetToDirtyBits(
    TfToken const& primType, HdDataSourceLocatorSet const& set)
{
    HdDataSourceLocatorSet::const_iterator it = set.begin();

    if (it == set.end()) {
        return HdChangeTracker::Clean;
    }

    HdDataSourceLocatorSet::const_iterator end = set.end();
    HdDirtyBits bits = HdChangeTracker::Clean;

    // Note, for efficiency we search for locators in the set in order, so that
    // we only end up making one trip through the set. If you add to this
    // function, make sure you sort the addition by locator name, or
    // _FindLocator won't work.
    // Also note, this should match BprimDirtyBitsToLocatorSet

    if (primType == HdPrimTypeTokens->renderBuffer) {
        if (_FindLocator(HdRenderBufferSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdRenderBuffer::DirtyDescription;
        }
    } else if (HdLegacyPrimTypeIsVolumeField(primType)) {
        if (_FindLocator(HdVolumeFieldSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdField::DirtyParams;
        }
    }

    return bits;
}

PXR_NAMESPACE_CLOSE_SCOPE
