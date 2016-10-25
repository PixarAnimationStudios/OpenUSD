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
#include "pxr/usd/usdUtils/stitch.h"

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"

#include "pxr/base/tf/warning.h"
#include "pxr/base/tf/token.h"

#include <set>
#include <string>
#include <algorithm>
 

// utility functions, not to be exposed as public facing API
namespace {
    ////////////////////////////////////////////////////////////////////////////
    // XXX(Frame->Time): backwards compatibility
    // Temporary helper functions to support backwards compatibility.
    // -------------------------------------------------------------------------

    static
    bool
    _HasStartFrame(const SdfLayerConstHandle &layer)
    {
        return layer->GetPseudoRoot()->HasInfo(SdfFieldKeys->StartFrame);
    }

    static
    bool
    _HasEndFrame(const SdfLayerConstHandle &layer)
    {
        return layer->GetPseudoRoot()->HasInfo(SdfFieldKeys->EndFrame);
    }

    static
    double
    _GetStartFrame(const SdfLayerConstHandle &layer)
    {
        VtValue startFrame = layer->GetPseudoRoot()->GetInfo(SdfFieldKeys->StartFrame);
        if (startFrame.IsHolding<double>())
            return startFrame.UncheckedGet<double>();
        return 0.0;
    }

    static
    double
    _GetEndFrame(const SdfLayerConstHandle &layer)
    {
        VtValue endFrame = layer->GetPseudoRoot()->GetInfo(SdfFieldKeys->EndFrame);
        if (endFrame.IsHolding<double>())
            return endFrame.UncheckedGet<double>();
        return 0.0;
    }

    // Backwards compatible helper function for getting the startTime of a 
    // layer. 
    static 
    double
    _GetStartTimeCode(const SdfLayerConstHandle &layer) 
    {
        return layer->HasStartTimeCode() ? 
            layer->GetStartTimeCode() : 
            (_HasStartFrame(layer) ? _GetStartFrame(layer) : 0.0);
    }

    // Backwards compatible helper function for getting the endTime of a 
    // layer. 
    static 
    double
    _GetEndTimeCode(const SdfLayerConstHandle &layer) 
    {
        return layer->HasEndTimeCode() ? 
            layer->GetEndTimeCode() : 
            (_HasEndFrame(layer) ? _GetEndFrame(layer) : 0.0);
    }

    ////////////////////////////////////////////////////////////////////////////

    // functions for combining ancilliary data in objects or layers
    // ------------------------------------------------------------------------
    void _StitchFrameRanges(const SdfLayerHandle&, const SdfLayerHandle&);
    void _StitchFramesPerSecond(const SdfLayerHandle&, const SdfLayerHandle&);
    void _StitchFramePrecision(const SdfLayerHandle&, const SdfLayerHandle&);
    void _StitchFrameInfo(const SdfLayerHandle&, const SdfLayerHandle&);

    // functions for merging two objects
    // ------------------------------------------------------------------------
    void _StitchPrims(const SdfPrimSpecHandle&, const SdfPrimSpecHandle&,
                      bool);
    void _StitchAttributes(const SdfPrimSpecHandle&, const SdfPrimSpecHandle&,
                           bool);
    void _StitchRelationships(const SdfPrimSpecHandle&, 
                              const SdfPrimSpecHandle&, bool);
    std::vector<TfToken> _ObtainRelevantKeysToStitch(const SdfSpecHandle&);

    // copy functions
    // ------------------------------------------------------------------------
    void _MakePrimCopy(const SdfPrimSpecHandle&, const SdfPrimSpecHandle&,
                       bool);
    void _MakeRelationshipCopy(const SdfPrimSpecHandle&, 
                               const SdfRelationshipSpecHandle&, bool);
    void _MakeAttributeCopy(const SdfPrimSpecHandle&, 
                            const SdfAttributeSpecHandle&, bool); 

    // generate warnings based on inconsistencies in the generated layer
    // ------------------------------------------------------------------------
    void _VerifyLayerIntegrity(const SdfLayerHandle&);

    // stitching functions
    // ------------------------------------------------------------------------
    void 
    _StitchFrameRanges(const SdfLayerHandle& strongLayer,
                       const SdfLayerHandle& weakLayer) 
    {
        if (weakLayer->HasStartTimeCode() or _HasStartFrame(weakLayer)) {
            double startTimeCode = std::min(_GetStartTimeCode(weakLayer), 
                                            _GetStartTimeCode(strongLayer));
            strongLayer->SetStartTimeCode(startTimeCode);
        }
        
        if (weakLayer->HasEndTimeCode() or _HasEndFrame(weakLayer)) {
            double endTimeCode = std::max(_GetEndTimeCode(weakLayer),
                                          _GetEndTimeCode(strongLayer));
            strongLayer->SetEndTimeCode(endTimeCode);
        }
    }

    void 
    _StitchFramesPerSecond(const SdfLayerHandle& strongLayer, 
                           const SdfLayerHandle& weakLayer) 
    {
        if (weakLayer->HasFramesPerSecond()) {
            if (strongLayer->HasFramesPerSecond()) {
               if (strongLayer->GetFramesPerSecond() !=
                   weakLayer->GetFramesPerSecond()) {
                    TF_WARN("Mismatched fps's in usd files ");
               }

            } else {
                strongLayer
                    ->SetFramesPerSecond(weakLayer->GetFramesPerSecond());
            }   
        }
    }

    void 
    _StitchFramePrecision(const SdfLayerHandle& strongLayer, 
                          const SdfLayerHandle& weakLayer) 
    {
        if (weakLayer->HasFramePrecision()) {
            if (strongLayer->HasFramePrecision()) {
                if (strongLayer->GetFramePrecision() != 
                    weakLayer->GetFramePrecision()) {
                    TF_WARN("Mismatched frame precisions in usd files"); 
                }
            } else {
                strongLayer->SetFramePrecision(weakLayer->GetFramePrecision());
            }
        }
    }

    void
    _StitchPrims(const SdfPrimSpecHandle& strongPrim,
                 const SdfPrimSpecHandle& weakPrim,
                 bool ignoreTimeSamples)
    {
        for (const auto& weakPrimIter : weakPrim->GetNameChildren()) {
            // lookup prim in strong layer
            SdfPrimSpecHandle strongPrimHandle 
                = strongPrim->GetPrimAtPath(weakPrimIter->GetPath()); 

            // if we don't have a matching prim in the strong layer
            // we can simply insert a copy of the weak's prim
            if (not strongPrimHandle) {
                _MakePrimCopy(strongPrim, weakPrimIter, ignoreTimeSamples); 
            } else {
                // Passing corresponding prims through
                //
                // i.e.
                //
                // ( root)             (root)
                // |                   |
                // |___(prim "foo")    |___(prim "foo")
                //
                // The prims should correspond to one another
                // in terms of path /root/primfoo /root/primfoo
                _StitchAttributes(strongPrimHandle, weakPrimIter,
                                  ignoreTimeSamples);
                _StitchRelationships(strongPrimHandle, weakPrimIter,
                                     ignoreTimeSamples);
                UsdUtilsStitchInfo(strongPrimHandle, weakPrimIter, 
                                   ignoreTimeSamples);
                _StitchPrims(strongPrimHandle, weakPrimIter, ignoreTimeSamples);
            }
        }
    }
    
    void 
    _StitchAttributes(const SdfPrimSpecHandle& strongPrim,
                      const SdfPrimSpecHandle& weakPrim,
                      bool ignoreTimeSamples) 
    {
        for (const auto& childAttribute : weakPrim->GetAttributes()) {
            SdfPath pathToChildAttr = childAttribute->GetPath();
            SdfAttributeSpecHandle strongAttrHandle
                = strongPrim->GetAttributeAtPath(pathToChildAttr);

            if (not strongAttrHandle) {
                _MakeAttributeCopy(strongPrim, childAttribute, 
                                   ignoreTimeSamples);
            } else {
                UsdUtilsStitchInfo(strongAttrHandle, childAttribute,
                                   ignoreTimeSamples);

                if (ignoreTimeSamples) {
                    continue;
                }

                SdfLayerHandle weakParent = weakPrim->GetLayer();
                SdfLayerHandle strongParent = strongPrim->GetLayer();
                SdfPath currAttrPath = strongAttrHandle->GetPath();

                // time samples needs special attention, we can't simply
                // call UsdUtilsStitchInfo(), because both attributes will
                // have the key 'timeSamples', and we must do an inner compare
                for (const double timeSamplePoint 
                        : weakParent->ListTimeSamplesForPath(currAttrPath)) {
                    // if the parent doesn't contain the time
                    // sample point key in its dict
                    if (not strongParent->QueryTimeSample(pathToChildAttr,
                                                          timeSamplePoint)) {
                        VtValue timeSampleValue; 
                        weakParent->QueryTimeSample(pathToChildAttr,
                                                    timeSamplePoint,
                                                    &timeSampleValue);

                        strongParent->SetTimeSample(pathToChildAttr,
                                                    timeSamplePoint,
                                                    timeSampleValue);
                    }
                }
            }
        }
    }

    void
    _StitchRelationships(const SdfPrimSpecHandle& strongPrim,
                         const SdfPrimSpecHandle& weakPrim,
                         bool ignoreTimeSamples) 
    {
        for (const auto& childRelationship : weakPrim->GetRelationships()) {
            SdfPath pathToChildRel = childRelationship->GetPath();
            SdfRelationshipSpecHandle strongRelHandle
                = strongPrim->GetRelationshipAtPath(pathToChildRel);

            if (not strongRelHandle) {
                _MakeRelationshipCopy(strongPrim, childRelationship,
                                      ignoreTimeSamples);
            } else {
                UsdUtilsStitchInfo(strongRelHandle, childRelationship,
                                   ignoreTimeSamples);
            }
        }
    }

    void
    _StitchFrameInfo(const SdfLayerHandle& strongLayer,
                            const SdfLayerHandle& weakLayer)
    {
        _StitchFrameRanges(strongLayer, weakLayer);
        _StitchFramePrecision(strongLayer, weakLayer);
        _StitchFramesPerSecond(strongLayer, weakLayer);
    }

    // functions for creating a copy of some object type under a root
    // -----------------------------------------------------------------------
    void
    _MakePrimCopy(const SdfPrimSpecHandle& strongPrim,
                  const SdfPrimSpecHandle& primToCopy,
                  bool ignoreTimeSamples) 
    {
        SdfPrimSpecHandle newPrim = SdfPrimSpec::New(strongPrim, 
                                                     primToCopy->GetName(),
                                                     primToCopy->GetSpecifier(),
                                                     primToCopy->GetTypeName());
       
        // Stitch will make copies in this case because none of the infoKeys 
        // will be in the fresh copy 'newPrim'
        UsdUtilsStitchInfo(newPrim, primToCopy, ignoreTimeSamples);

        // copy child prims
        for (const auto& childPrim : primToCopy->GetNameChildren()) {
            _MakePrimCopy(newPrim, childPrim, ignoreTimeSamples); 
        }

        // copy child attributes
        for (const auto& childAttribute : primToCopy->GetAttributes()) {
            _MakeAttributeCopy(newPrim, childAttribute, ignoreTimeSamples);
        }

        // copy child relationships
        for (const auto& childRelationship : primToCopy->GetRelationships()) {
            _MakeRelationshipCopy(newPrim, childRelationship,ignoreTimeSamples);
        }
    }

    void
    _MakeRelationshipCopy(const SdfPrimSpecHandle& strongPrim,
                          const SdfRelationshipSpecHandle& relToCopy,
                          bool ignoreTimeSamples) 
    {
        SdfRelationshipSpecHandle newRel 
            = SdfRelationshipSpec::New(strongPrim,
                                       relToCopy->GetName(),
                                       relToCopy->IsCustom(),
                                       relToCopy->GetVariability());
     
        UsdUtilsStitchInfo(newRel, relToCopy, ignoreTimeSamples);
    }

    void
    _MakeAttributeCopy(const SdfPrimSpecHandle& strongPrim,
                       const SdfAttributeSpecHandle& attrToCopy,
                       bool ignoreTimeSamples) 
    {
        // XXX: note that USD doesn't currently support expressions nor mappers.
        SdfAttributeSpecHandle newAttr 
            = SdfAttributeSpec::New(strongPrim, 
                                    attrToCopy->GetName(),
                                    attrToCopy->GetTypeName(),
                                    attrToCopy->GetVariability(),
                                    attrToCopy->IsCustom());

        UsdUtilsStitchInfo(newAttr, attrToCopy, ignoreTimeSamples);
    }

    // misc helper functions
    // ------------------------------------------------------------------------
    void
    _VerifyLayerIntegrity(const SdfLayerHandle& strongLayer)
    {
        // verify that if one frame-info is self consistent 
        std::string strongFileName = strongLayer->GetDisplayName();
        bool hasStartTimeCode = strongLayer->HasStartTimeCode();
        bool hasEndTimeCode = strongLayer->HasEndTimeCode();
        std::set<double> timeSamples = strongLayer->ListAllTimeSamples();

        if (hasStartTimeCode and not hasEndTimeCode) {
            TF_WARN("Missing endTimeCode in %s", strongFileName.c_str());
        } else if (not hasStartTimeCode and hasEndTimeCode) {
            TF_WARN("Missing startTimeCode in %s", strongFileName.c_str());
        } else if (not timeSamples.empty() 
                    and (hasStartTimeCode and hasEndTimeCode)) {
            if (*timeSamples.begin() < strongLayer->GetStartTimeCode()) {
                TF_WARN("Result has time sample point before startTimeCode");
            }
            if (*timeSamples.rbegin() > strongLayer->GetEndTimeCode()) {
                TF_WARN("Result has time sample point after endTimeCode");
            }
        }
    }

    // These keys represent data we wish to filter out of our token search
    // when stitching data in a SdfSpec.
    TF_MAKE_STATIC_DATA((std::vector<TfToken>), _SortedChildrenTokens) {
        *_SortedChildrenTokens = SdfChildrenKeys->allTokens;
        std::sort(_SortedChildrenTokens->begin(), _SortedChildrenTokens->end());
    } 

    // This function returns the keys we will want to stitch for a 
    // particular spec. It filters out various book-keeping data that
    // are stored in the set of fields but need not be copied.
    std::vector<TfToken> 
    _ObtainRelevantKeysToStitch(const SdfSpecHandle& spec) {
        
        std::vector<TfToken> specFields = spec->ListFields();
        std::sort(specFields.begin(), specFields.end());

        std::vector<TfToken> relevantKeys;
        relevantKeys.reserve(specFields.size());
        std::set_difference(specFields.begin(), 
                            specFields.end(),
                            _SortedChildrenTokens->begin(), 
                            _SortedChildrenTokens->end(),
                            std::back_inserter(relevantKeys));
        return relevantKeys;
    }
} // end anon namespace

// public facing API
// ----------------------------------------------------------------------------

void
UsdUtilsStitchInfo(const SdfSpecHandle& strongObj,
                   const SdfSpecHandle& weakObj,
                   bool ignoreTimeSamples)
{
    for (const auto& key : _ObtainRelevantKeysToStitch(weakObj)) {
        const VtValue strongValue = strongObj->GetSchema().GetFallback(key); 
            
        // if we have a dictionary type, we need to do a merge
        // on the contained dicts recursively with VtDictionaryOver
        if (strongValue.IsHolding<VtDictionary>()) {
            // construct VtDicts and merge them 
            VtDictionary strongDict 
                = strongObj->GetInfo(key).Get<VtDictionary>();
            VtDictionary weakDict 
                = weakObj->GetInfo(key).Get<VtDictionary>();
            // note that VtDictionaryOver() implicitly gives 
            // opinion strength to its leftmost 'strong' argument
            // which preserves our opinion strength convention
            VtValue mergedDict 
                = VtValue(VtDictionaryOver(strongDict,weakDict));
            strongObj->SetInfo(key, mergedDict);

        // XXX used by stitchmodelclips for ignoring time samples
        } else if(ignoreTimeSamples and key == SdfFieldKeys->TimeSamples) {
           continue; 

        // if the info is a target path, we need to copy via the 
        // targetpath API as set-info would cause a read-only failure
        // by simply using SetInfo as we do below
        } else if (key == SdfFieldKeys->TargetPaths) { 
            if (not strongObj->HasInfo(key)) {
                // we can safely create relationship handles here as we 
                // know target paths will only pop up on relationships
                SdfRelationshipSpecHandle strongRelHandle 
                    = TfDynamic_cast<SdfRelationshipSpecHandle>(strongObj);
                SdfRelationshipSpecHandle weakRelHandle 
                    = TfDynamic_cast<SdfRelationshipSpecHandle>(weakObj);

                // ensure proper prim handles were obtained
                if (not TF_VERIFY(strongRelHandle and weakRelHandle)) {
                    // if they weren't skip this iteration
                    continue;
                }

                // we need to use copyitems to get a fresh copy of the contents
                // of weakRelHandles target path list
                strongRelHandle->GetTargetPathList().CopyItems( 
                        weakRelHandle->GetTargetPathList());
            }
        } else {
            // if its not a dictionary type, insert as normal
            // so long as it isn't contained in the strong object 
            if (not strongObj->HasInfo(key)) {
                strongObj->SetInfo(key, weakObj->GetInfo(key));
            }
        }
    }
}

void 
UsdUtilsStitchLayers(const SdfLayerHandle& strongLayer,
                     const SdfLayerHandle& weakLayer,
                     bool ignoreTimeSamples)
{
    // Get a prim-based handle to simplify subroutines
    SdfPrimSpecHandle weakPseudoRoot = weakLayer->GetPseudoRoot();
    SdfPrimSpecHandle strongPseudoRoot = strongLayer->GetPseudoRoot();
    
    // Stitching layers starting with root prims
    UsdUtilsStitchInfo(strongPseudoRoot, weakPseudoRoot, ignoreTimeSamples);
    _StitchPrims(strongPseudoRoot, weakPseudoRoot, ignoreTimeSamples);

    // Stitch initial layer elements. 
    // Note: This needs to happen after we call UsdUtilsStitchInfo on the 
    // pseudoRoot. If not, start/endTimeCode metadate could get overwritten 
    // with wrong values.
    _StitchFrameInfo(strongLayer, weakLayer);
    
    // Send warnings if erroneous generation occurs
    _VerifyLayerIntegrity(strongLayer);
}
