//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/dirtyBitsTranslator.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/dataSourceLegacyPrim.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/coordSys.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/field.h"
#include "pxr/imaging/hd/imageShader.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderSettings.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/categoriesSchema.h"
#include "pxr/imaging/hd/capsuleSchema.h"
#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/coneSchema.h"
#include "pxr/imaging/hd/coordSysSchema.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/cubeSchema.h"
#include "pxr/imaging/hd/cylinderSchema.h"
#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/geomSubsetSchema.h"
#include "pxr/imaging/hd/imageShaderSchema.h"
#include "pxr/imaging/hd/instanceCategoriesSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/integratorSchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/renderBufferSchema.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"
#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/imaging/hd/subdivisionTagsSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"
#include "pxr/imaging/hd/volumeFieldSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/base/tf/staticData.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

using _SToBMap = std::unordered_map<TfToken,
    HdDirtyBitsTranslator::LocatorSetToDirtyBitsFnc, TfHash>;

using _BToSMap = std::unordered_map<TfToken,
    HdDirtyBitsTranslator::DirtyBitsToLocatorSetFnc, TfHash>;

static TfStaticData<_SToBMap> Hd_SPrimSToBFncs;
static TfStaticData<_BToSMap> Hd_SPrimBToSFncs;

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
            set->append(HdBasisCurvesTopologySchema::GetDefaultLocator());
        }
    }

    if (primType == HdPrimTypeTokens->capsule) {
        if (bits & HdChangeTracker::DirtyPrimvar) {
            set->append(HdCapsuleSchema::GetDefaultLocator());
        }
    }

    if (bits & HdChangeTracker::DirtyCategories) {
        set->append(HdCategoriesSchema::GetDefaultLocator());
    }

    if (primType == HdPrimTypeTokens->cone) {
        if (bits & HdChangeTracker::DirtyPrimvar) {
            set->append(HdConeSchema::GetDefaultLocator());
        }
    }

    if (primType == HdPrimTypeTokens->cube) {
        if (bits & HdChangeTracker::DirtyPrimvar) {
            set->append(HdCubeSchema::GetDefaultLocator());
        }
    }

    if (primType == HdPrimTypeTokens->cylinder) {
        if (bits & HdChangeTracker::DirtyPrimvar) {
            set->append(HdCylinderSchema::GetDefaultLocator());
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
        set->append(HdMaterialBindingsSchema::GetDefaultLocator());
    }

    if (primType == HdPrimTypeTokens->mesh) {
        if (bits & HdChangeTracker::DirtyDoubleSided) {
            set->append(HdMeshSchema::GetDoubleSidedLocator());
        }

        if (bits & HdChangeTracker::DirtyTopology) {
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

    if (primType == HdPrimTypeTokens->sphere) {
        if (bits & HdChangeTracker::DirtyPrimvar) {
            set->append(HdSphereSchema::GetDefaultLocator());
        }
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
        if (bits & HdCoordSys::DirtyName) {
            static const HdDataSourceLocator locator =
                HdCoordSysSchema::GetDefaultLocator()
                    .Append(HdCoordSysSchemaTokens->name);
            set->append(locator);
        }
        if (bits & HdCoordSys::DirtyTransform) {
            set->append(HdXformSchema::GetDefaultLocator());
        }
    } else if (primType == HdPrimTypeTokens->camera) {
        if (bits & (HdCamera::DirtyParams |
                    HdCamera::DirtyClipPlanes |
                    HdCamera::DirtyWindowPolicy)) {
            set->append(HdCameraSchema::GetDefaultLocator());
        }
        if (bits & HdCamera::DirtyTransform) {
            set->append(HdXformSchema::GetDefaultLocator());
        }
    } else if (HdPrimTypeIsLight(primType)
            // Lights and light filters are handled similarly in emulation.
            || primType == HdPrimTypeTokens->lightFilter
            // special case for mesh lights coming from emulated scene
            // for which the type will be mesh even though we are receiving
            // sprim-specific dirty bits.
            // NOTE: The absence of this would still work but would
            //       over-invalidate since the fallback value is "".
             || primType == HdPrimTypeTokens->mesh
            ) {
        if (bits & (HdLight::DirtyParams |
                    HdLight::DirtyShadowParams |
                    HdLight::DirtyCollection)) {
            set->append(HdLightSchema::GetDefaultLocator());
        }
        if (bits & HdLight::DirtyResource) {
            set->append(HdMaterialSchema::GetDefaultLocator());
        }
        if (bits & HdLight::DirtyParams) {
            // for mesh lights, don't want changing light parameters to trigger
            // mesh primvar updates.
            if (primType != HdPrimTypeTokens->mesh) {
                set->append(HdPrimvarsSchema::GetDefaultLocator());
            }
            set->append(HdVisibilitySchema::GetDefaultLocator());

            // Invalidate collections manufactured for light linking in
            // emulation.
            set->append(HdCollectionsSchema::GetDefaultLocator());
        }
        if (bits & HdLight::DirtyTransform) {
            set->append(HdXformSchema::GetDefaultLocator());
        }
        if (bits & HdLight::DirtyInstancer) {
            set->append(HdInstancedBySchema::GetDefaultLocator());
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
    } else if (primType == HdPrimTypeTokens->integrator) {
        if (bits & HdChangeTracker::DirtyParams) {
            set->append(HdIntegratorSchema::GetDefaultLocator());
        }
    } else if (primType == HdPrimTypeTokens->sampleFilter) {
        if (bits & HdChangeTracker::DirtyParams) {
            set->append(HdSampleFilterSchema::GetDefaultLocator());
        }
        if (bits & HdChangeTracker::DirtyVisibility) {
            set->append(HdVisibilitySchema::GetDefaultLocator());
        }
    } else if (primType == HdPrimTypeTokens->displayFilter) {
        if (bits & HdChangeTracker::DirtyParams) {
            set->append(HdDisplayFilterSchema::GetDefaultLocator());
        }
        if (bits & HdChangeTracker::DirtyVisibility) {
            set->append(HdVisibilitySchema::GetDefaultLocator());
        }
    } else if (primType == HdPrimTypeTokens->imageShader) {
        if (bits & HdImageShader::DirtyEnabled) {
            set->append(HdImageShaderSchema::GetEnabledLocator());
        }
        if (bits & HdImageShader::DirtyPriority) {
            set->append(HdImageShaderSchema::GetPriorityLocator());
        }
        if (bits & HdImageShader::DirtyFilePath) {
            set->append(HdImageShaderSchema::GetFilePathLocator());
        }
        if (bits & HdImageShader::DirtyConstants) {
            set->append(HdImageShaderSchema::GetConstantsLocator());
        }
        if (bits & HdImageShader::DirtyMaterialNetwork) {
            set->append(HdImageShaderSchema::GetMaterialNetworkLocator());
        }
    } else {
        const auto fncIt = Hd_SPrimBToSFncs->find(primType);
        if (fncIt == Hd_SPrimBToSFncs->end()) {
            // unknown prim type, use AllDirty for anything
            if (bits) {
                set->append(HdDataSourceLocator());
            }
        } else {
            // call custom handler registered for this type
            fncIt->second(bits, set);
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
    } else if (primType == HdPrimTypeTokens->renderSettings) {
        if (bits & HdRenderSettings::DirtyActive) {
            set->append(HdRenderSettingsSchema::GetActiveLocator());
        }
        if (bits & HdRenderSettings::DirtyFrameNumber) {
            set->append(HdRenderSettingsSchema::GetFrameLocator());
        }
        if (bits & HdRenderSettings::DirtyNamespacedSettings) {
            set->append(HdRenderSettingsSchema::GetNamespacedSettingsLocator());
        }
        if (bits & HdRenderSettings::DirtyRenderProducts) {
            set->append(HdRenderSettingsSchema::GetRenderProductsLocator());
        }
        if (bits & HdRenderSettings::DirtyIncludedPurposes) {
            set->append(HdRenderSettingsSchema::GetIncludedPurposesLocator());
        }
        if (bits & HdRenderSettings::DirtyMaterialBindingPurposes) {
            set->append(HdRenderSettingsSchema::GetMaterialBindingPurposesLocator());
        }
        if (bits & HdRenderSettings::DirtyRenderingColorSpace) {
            set->append(HdRenderSettingsSchema::GetRenderingColorSpaceLocator());
        }
        if (bits & HdRenderSettings::DirtyShutterInterval) {
            set->append(HdRenderSettingsSchema::GetShutterIntervalLocator());
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

    // (*) Attention:
    // If you add to this function, make sure you insert the addition so that
    // the _FindLocator calls are sorted by locator name, or _FindLocator won't
    // work.
    // For efficiency we search for locators in the set in order, so
    // that we only end up making one trip through the set.
    // Also note, this should match RprimDirtyBitsToLocatorSet.

    // _FindLocator here is called with advanceToNext = true. It will advance
    // "it" from the current position to the first element where it > locator
    // and !it.HasPrefix(locator).
    // If any of the iterated elements intersect locator, it returns true.
    // Here: search for the locator "basisCurvesTopology" in the set; if a child
    // or parent (such as "" or "basisCurvesTopology/curveType") is present,
    // mark DirtyToplogy.  it points to the next element after
    // "basisCurvesTopology", setting us up to check for displayStyle.
    if (primType == HdPrimTypeTokens->basisCurves) {

        // Locator (*): basisCurves > topology
        if (_FindLocator(HdBasisCurvesTopologySchema::GetDefaultLocator(),
                         end, &it)) {
            bits |= HdChangeTracker::DirtyTopology;
        }
    }

    if (primType == HdPrimTypeTokens->capsule) {
        // Locator (*): capsule
        if (_FindLocator(HdCapsuleSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyPrimvar;
        }
    }

    // Locator (*): categories

    if (_FindLocator(HdCategoriesSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyCategories;
    }

    if (primType == HdPrimTypeTokens->cone) {
        // Locator (*): cone
        if (_FindLocator(HdConeSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyPrimvar;
        }
    }

    if (primType == HdPrimTypeTokens->cube) {
        // Locator (*): cube
        if (_FindLocator(HdCubeSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyPrimvar;
        }
    }

    if (primType == HdPrimTypeTokens->cylinder) {
        // Locator (*): cylinder
        if (_FindLocator(HdCylinderSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyPrimvar;
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

    // Locator (*): displayStyle

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

    // Locator (*): extent

    if (_FindLocator(HdExtentSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyExtent;
    }

    // Locator (*): extComputationPrimvars

    if (_FindLocator(HdExtComputationPrimvarsSchema::GetDefaultLocator(),
                end, &it)) {
        bits |= HdChangeTracker::DirtyPrimvar;
    }

    // Locator (*): instancedBySchema

    if (_FindLocator(HdInstancedBySchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyInstancer;
    }

    // Locator (*): instancerToplogySchema

    if (_FindLocator(HdInstancerTopologySchema::GetDefaultLocator(), end, &it)){
        bits |= HdChangeTracker::DirtyInstanceIndex;
    }

    // Locator (*): materialBindingSchema

    if (_FindLocator(HdMaterialBindingsSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyMaterialId;
    }

    if (primType == HdPrimTypeTokens->mesh) {

        // Locator (*): mesh > doubleSided

        if (_FindLocator(HdMeshSchema::GetDoubleSidedLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyDoubleSided;
        }

        // Locator (*): mesh > subdivisionScheme

        if (_FindLocator(HdMeshSchema::GetSubdivisionSchemeLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyTopology;
        }

        // Locator (*): mesh > subdivisionTags

        if (_FindLocator(HdMeshSchema::GetSubdivisionTagsLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtySubdivTags;
        }

        // Locator (*): mesh > topology

        if (_FindLocator(HdMeshTopologySchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyTopology;
        }
    }

    // Locator (*): primvars

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

    // Locator (*): purpose

    if (_FindLocator(HdPurposeSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyRenderTag;
    }

    if (primType == HdPrimTypeTokens->sphere) {
        // Locator (*): sphere
        if (_FindLocator(HdSphereSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyPrimvar;
        }
    }

    // Locator (*): visibility

    if (_FindLocator(HdVisibilitySchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyVisibility;
    }

    // Locator (*): volumeFieldBinding

    if (_FindLocator(HdVolumeFieldBindingSchema::GetDefaultLocator(), end, &it)) {
        bits |= HdChangeTracker::DirtyVolumeField;
    }

    // Locator (*): xform

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
        static const HdDataSourceLocator nameLocator =
            HdCoordSysSchema::GetDefaultLocator()
            .Append(HdCoordSysSchemaTokens->name);
        if (_FindLocator(nameLocator, end, &it)) {
            bits |= HdCoordSys::DirtyName;
        }
        if (_FindLocator(HdXformSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdCoordSys::DirtyTransform;
        }
    } else if (primType == HdPrimTypeTokens->camera) {
        if (_FindLocator(HdCameraSchema::GetDefaultLocator(), end, &it)) {
            bits |=
                HdCamera::DirtyWindowPolicy |
                HdCamera::DirtyClipPlanes |
                HdCamera::DirtyParams;
        }
        if (_FindLocator(HdXformSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdCamera::DirtyTransform;
        }
    } else if (HdPrimTypeIsLight(primType)
        // Lights and light filters are handled similarly in emulation.
        || primType == HdPrimTypeTokens->lightFilter) {

        if (_FindLocator(HdInstancedBySchema::GetDefaultLocator(), end, &it)) {
            bits |= HdLight::DirtyInstancer;
        }
        if (_FindLocator(HdLightSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdLight::DirtyParams |
                HdLight::DirtyResource |
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
            bits |= HdLight::DirtyParams;
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
    } else if (primType == HdPrimTypeTokens->integrator) {
        if (_FindLocator(HdIntegratorSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyParams;
        }
    } else if (primType == HdPrimTypeTokens->sampleFilter) {
        if (_FindLocator(HdSampleFilterSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyParams;
        }
        if (_FindLocator(HdVisibilitySchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyVisibility;
        }
    } else if (primType == HdPrimTypeTokens->displayFilter) {
        if (_FindLocator(HdDisplayFilterSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyParams;
        }
        if (_FindLocator(HdVisibilitySchema::GetDefaultLocator(), end, &it)) {
            bits |= HdChangeTracker::DirtyVisibility;
        }
    } else if (primType == HdPrimTypeTokens->imageShader) {
        if (_FindLocator(HdImageShaderSchema::GetDefaultLocator(),
                end, &it, false)) {
            if (HdImageShaderSchema::GetDefaultLocator().HasPrefix(*it)) {
                bits |= HdImageShader::AllDirty;
            } else {
                do {
                    if (it->HasPrefix(
                        HdImageShaderSchema::GetEnabledLocator())) {
                        bits |= HdImageShader::DirtyEnabled;
                    }
                    if (it->HasPrefix(
                        HdImageShaderSchema::GetPriorityLocator())) {
                        bits |= HdImageShader::DirtyPriority;
                    }
                    if (it->HasPrefix(
                        HdImageShaderSchema::GetFilePathLocator())) {
                        bits |= HdImageShader::DirtyFilePath;
                    }
                    if (it->HasPrefix(
                        HdImageShaderSchema::GetConstantsLocator())) {
                        bits |= HdImageShader::DirtyConstants;
                    }
                    if (it->HasPrefix(
                        HdImageShaderSchema::GetMaterialNetworkLocator())) {
                        bits |= HdImageShader::DirtyMaterialNetwork;
                    }
                    ++it;
                } while(it != end && it->Intersects(
                            HdImageShaderSchema::GetDefaultLocator()));
            }
        }
    } else {
        const auto fncIt = Hd_SPrimSToBFncs->find(primType);
        if (fncIt == Hd_SPrimSToBFncs->end()) {
            // unknown prim type, use AllDirty for anything
            if (_FindLocator(HdDataSourceLocator(), end, &it)) {
                bits |= HdChangeTracker::AllDirty;
            }
        } else {
            // call custom handler registered for this type
            fncIt->second(set, &bits);
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
    } else if (primType == HdPrimTypeTokens->renderSettings) {
        if (_FindLocator(HdRenderSettingsSchema::GetActiveLocator(),
                end, &it)) {
            bits |= HdRenderSettings::DirtyActive;
        }
        if (_FindLocator(HdRenderSettingsSchema::GetFrameLocator(),
                end, &it)) {
            bits |= HdRenderSettings::DirtyFrameNumber;
        }
        if (_FindLocator(HdRenderSettingsSchema::GetNamespacedSettingsLocator(),
                end, &it)) {
            bits |= HdRenderSettings::DirtyNamespacedSettings;
        }
        if (_FindLocator(HdRenderSettingsSchema::GetRenderProductsLocator(),
                end, &it)) {
            bits |= HdRenderSettings::DirtyRenderProducts;
        }
        if (_FindLocator(HdRenderSettingsSchema::GetIncludedPurposesLocator(),
                end, &it)) {
            bits |= HdRenderSettings::DirtyIncludedPurposes;
        }
        if (_FindLocator(
                HdRenderSettingsSchema::GetMaterialBindingPurposesLocator(),
                end, &it)) {
            bits |= HdRenderSettings::DirtyMaterialBindingPurposes;
        }
        if (_FindLocator(
                HdRenderSettingsSchema::GetRenderingColorSpaceLocator(),
                end, &it)) {
            bits |= HdRenderSettings::DirtyRenderingColorSpace;
        }
        if (_FindLocator(
                HdRenderSettingsSchema::GetShutterIntervalLocator(),
                end, &it)) {
            bits |= HdRenderSettings::DirtyShutterInterval;
        }
    } else if (HdLegacyPrimTypeIsVolumeField(primType)) {
        if (_FindLocator(HdVolumeFieldSchema::GetDefaultLocator(), end, &it)) {
            bits |= HdField::DirtyParams;
        }
    }

    return bits;
}

/*static*/
void
HdDirtyBitsTranslator::RegisterTranslatorsForCustomSprimType(
    TfToken const& primType,
    LocatorSetToDirtyBitsFnc sToBFnc,
    DirtyBitsToLocatorSetFnc bToSFnc)
{
    Hd_SPrimSToBFncs->insert({primType, sToBFnc});
    Hd_SPrimBToSFncs->insert({primType, bToSFnc});
}

PXR_NAMESPACE_CLOSE_SCOPE
