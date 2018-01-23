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
// Types.cpp
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/connectionListEditor.h"
#include "pxr/usd/sdf/listOpListEditor.h"
#include "pxr/usd/sdf/reference.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    // Other.
    TfType::Define<SdfDictionaryProxy>();
    TfType::Define<SdfVariantSelectionProxy>();
    TfType::Define<SdfRelocatesMapProxy>();

    TfType::Define<SdfInheritsProxy>()
        .Alias(TfType::GetRoot(), "SdfInheritsProxy")
        ;
    TfType::Define<SdfReferencesProxy>()
        .Alias(TfType::GetRoot(), "SdfReferencesProxy")
        ;
}

template <class P>
struct Sdf_ListEditorProxyTraits {
};

template <>
struct Sdf_ListEditorProxyTraits<SdfPathEditorProxy> {
    typedef SdfPathEditorProxy::TypePolicy TypePolicy;

    static std::shared_ptr<Sdf_ListEditor<TypePolicy> > GetListEditor(
        const SdfSpecHandle& o, const TfToken& n)
    {
        if (n == SdfFieldKeys->TargetPaths) {
            return std::shared_ptr<Sdf_ListEditor<TypePolicy> >(
                new Sdf_RelationshipTargetListEditor(o, TypePolicy(o)));
        }
        else if (n == SdfFieldKeys->ConnectionPaths) {
            return std::shared_ptr<Sdf_ListEditor<TypePolicy> >(
                new Sdf_AttributeConnectionListEditor(o, TypePolicy(o)));
        }

        return std::shared_ptr<Sdf_ListEditor<TypePolicy> >(
            new Sdf_ListOpListEditor<TypePolicy>(o, n, TypePolicy(o)));
    }
};

template <>
struct Sdf_ListEditorProxyTraits<SdfReferenceEditorProxy> {
    typedef SdfReferenceEditorProxy::TypePolicy TypePolicy;

    static std::shared_ptr<Sdf_ListEditor<TypePolicy> > GetListEditor(
        const SdfSpecHandle& o, const TfToken& n)
    {
        return std::shared_ptr<Sdf_ListEditor<TypePolicy> >(
            new Sdf_ListOpListEditor<SdfReferenceTypePolicy>(o, n));
    }
};

template <class Proxy>
inline
Proxy SdfGetListEditorProxy(const SdfSpecHandle& o, const TfToken & n)
{
    typedef Sdf_ListEditorProxyTraits<Proxy> Traits;
    return Proxy(Traits::GetListEditor(o, n));
}

SdfPathEditorProxy
SdfGetPathEditorProxy(const SdfSpecHandle& o, const TfToken & n)
{
    return SdfGetListEditorProxy<SdfPathEditorProxy>(o, n);
}

SdfReferenceEditorProxy
SdfGetReferenceEditorProxy(const SdfSpecHandle& o, const TfToken & n)
{
    return SdfGetListEditorProxy<SdfReferenceEditorProxy>(o, n);
}

PXR_NAMESPACE_CLOSE_SCOPE
