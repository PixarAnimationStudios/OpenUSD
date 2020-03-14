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
#ifndef PXR_USD_SDF_PROXY_TYPES_H
#define PXR_USD_SDF_PROXY_TYPES_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/childrenProxy.h"
#include "pxr/usd/sdf/childrenView.h"
#include "pxr/usd/sdf/childrenPolicies.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/listEditorProxy.h"
#include "pxr/usd/sdf/listProxy.h"
#include "pxr/usd/sdf/mapEditProxy.h"
#include "pxr/usd/sdf/proxyPolicies.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfSpec);

typedef SdfListProxy<SdfNameTokenKeyPolicy> SdfNameOrderProxy;
typedef SdfListProxy<SdfSubLayerTypePolicy> SdfSubLayerProxy;                    
typedef SdfListEditorProxy<SdfNameKeyPolicy> SdfNameEditorProxy;
typedef SdfListEditorProxy<SdfPathKeyPolicy> SdfPathEditorProxy;
typedef SdfListEditorProxy<SdfPayloadTypePolicy> SdfPayloadEditorProxy;
typedef SdfListEditorProxy<SdfReferenceTypePolicy> SdfReferenceEditorProxy;

typedef SdfChildrenView<Sdf_AttributeChildPolicy,
            SdfAttributeViewPredicate> SdfAttributeSpecView;
typedef SdfChildrenView<Sdf_PrimChildPolicy> SdfPrimSpecView;
typedef SdfChildrenView<Sdf_PropertyChildPolicy> SdfPropertySpecView;
typedef SdfChildrenView<Sdf_AttributeChildPolicy > SdfRelationalAttributeSpecView;
typedef SdfChildrenView<Sdf_RelationshipChildPolicy, SdfRelationshipViewPredicate>
            SdfRelationshipSpecView;
typedef SdfChildrenView<Sdf_VariantChildPolicy> SdfVariantView;
typedef SdfChildrenView<Sdf_VariantSetChildPolicy> SdfVariantSetView;
typedef SdfChildrenProxy<SdfVariantSetView> SdfVariantSetsProxy;

typedef SdfNameOrderProxy SdfNameChildrenOrderProxy;
typedef SdfNameOrderProxy SdfPropertyOrderProxy;
typedef SdfPathEditorProxy SdfConnectionsProxy;
typedef SdfPathEditorProxy SdfInheritsProxy;
typedef SdfPathEditorProxy SdfSpecializesProxy;
typedef SdfPathEditorProxy SdfTargetsProxy;
typedef SdfPayloadEditorProxy SdfPayloadsProxy;
typedef SdfReferenceEditorProxy SdfReferencesProxy;
typedef SdfNameEditorProxy SdfVariantSetNamesProxy;

typedef SdfMapEditProxy<VtDictionary> SdfDictionaryProxy;
typedef SdfMapEditProxy<SdfVariantSelectionMap> SdfVariantSelectionProxy;
typedef SdfMapEditProxy<SdfRelocatesMap,
                        SdfRelocatesMapProxyValuePolicy> SdfRelocatesMapProxy;

/// Returns a path list editor proxy for the path list op in the given
/// \p pathField on \p spec.  If the value doesn't exist or \p spec is 
/// invalid then this returns an invalid list editor.
SdfPathEditorProxy
SdfGetPathEditorProxy(
    const SdfSpecHandle& spec, const TfToken& pathField);

/// Returns a reference list editor proxy for the references list op in the
/// given \p referenceField on \p spec. If the value doesn't exist or the object
/// is invalid then this returns an invalid list editor.
SdfReferenceEditorProxy
SdfGetReferenceEditorProxy(
    const SdfSpecHandle& spec, const TfToken& referenceField);

/// Returns a payload list editor proxy for the payloads list op in the given
/// \p payloadField on \p spec.  If the value doesn't exist or the object is 
/// invalid then this returns an invalid list editor.
SdfPayloadEditorProxy
SdfGetPayloadEditorProxy(
    const SdfSpecHandle& spec, const TfToken& payloadField);

/// Returns a name order list proxy for the ordering specified in the given
/// \p orderField on \p spec.  If the value doesn't exist or the object is
/// invalid then this returns an invalid list editor.
SdfNameOrderProxy
SdfGetNameOrderProxy(
    const SdfSpecHandle& spec, const TfToken& orderField);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PROXY_TYPES_H
