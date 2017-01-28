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
#ifndef SDF_PROXY_TYPES_H
#define SDF_PROXY_TYPES_H

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
typedef SdfListEditorProxy<SdfReferenceTypePolicy> SdfReferenceEditorProxy;

typedef SdfChildrenView<Sdf_AttributeChildPolicy,
            SdfAttributeViewPredicate> SdfAttributeSpecView;
typedef SdfChildrenView<Sdf_MapperChildPolicy,
            SdfConnectionMapperViewPredicate> SdfConnectionMappersView;
typedef SdfChildrenView<Sdf_MapperArgChildPolicy> SdfMapperArgSpecView;
typedef SdfChildrenView<Sdf_PrimChildPolicy> SdfPrimSpecView;
typedef SdfChildrenView<Sdf_PropertyChildPolicy> SdfPropertySpecView;
typedef SdfChildrenView<Sdf_AttributeChildPolicy > SdfRelationalAttributeSpecView;
typedef SdfChildrenView<Sdf_RelationshipChildPolicy, SdfRelationshipViewPredicate>
            SdfRelationshipSpecView;
typedef SdfChildrenView<Sdf_VariantChildPolicy> SdfVariantView;
typedef SdfChildrenView<Sdf_VariantSetChildPolicy> SdfVariantSetView;

typedef SdfChildrenProxy<SdfConnectionMappersView> SdfConnectionMappersProxy;
typedef SdfChildrenProxy<SdfMapperArgSpecView> SdfMapperArgsProxy;
typedef SdfChildrenProxy<SdfVariantSetView> SdfVariantSetsProxy;

typedef SdfNameOrderProxy SdfNameChildrenOrderProxy;
typedef SdfNameOrderProxy SdfPropertyOrderProxy;
typedef SdfPathEditorProxy SdfConnectionsProxy;
typedef SdfPathEditorProxy SdfInheritsProxy;
typedef SdfPathEditorProxy SdfSpecializesProxy;
typedef SdfPathEditorProxy SdfTargetsProxy;
typedef SdfReferenceEditorProxy SdfReferencesProxy;
typedef SdfNameEditorProxy SdfVariantSetNamesProxy;

typedef SdfMapEditProxy<VtDictionary> SdfDictionaryProxy;
typedef SdfMapEditProxy<SdfVariantSelectionMap> SdfVariantSelectionProxy;
typedef SdfMapEditProxy<SdfRelocatesMap,
                        SdfRelocatesMapProxyValuePolicy> SdfRelocatesMapProxy;

/// Returns a path list editor proxy for the children in the value with the
/// given name.  If the value doesn't exist, doesn't contain an GdChildren,
/// or the object is invalid then this returns an invalid list editor.
SdfPathEditorProxy
SdfGetPathEditorProxy(const SdfSpecHandle& o, const TfToken & n);

/// Returns a reference list editor proxy for the children in the value with
/// the given name.  If the value doesn't exist, doesn't contain an GdChildren,
/// or the object is invalid then this returns an invalid list editor.  This
/// does \b not check that the children hold references.
SdfReferenceEditorProxy
SdfGetReferenceEditorProxy(const SdfSpecHandle& o, const TfToken & n);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_PROXY_TYPES_H
