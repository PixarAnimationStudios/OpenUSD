//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Types.cpp
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/connectionListEditor.h"
#include "pxr/usd/sdf/listOpListEditor.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/vectorListEditor.h"

#include "pxr/base/tf/registryManager.h"

#include <memory>

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
    TfType::Define<SdfPayloadsProxy>()
        .Alias(TfType::GetRoot(), "SdfPayloadsProxy")
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

template <>
struct Sdf_ListEditorProxyTraits<SdfPayloadEditorProxy> {
    typedef SdfPayloadEditorProxy::TypePolicy TypePolicy;

    static std::shared_ptr<Sdf_ListEditor<TypePolicy> > GetListEditor(
        const SdfSpecHandle& o, const TfToken& n)
    {
        return std::shared_ptr<Sdf_ListEditor<TypePolicy> >(
            new Sdf_ListOpListEditor<SdfPayloadTypePolicy>(o, n));
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

SdfPayloadEditorProxy
SdfGetPayloadEditorProxy(const SdfSpecHandle& o, const TfToken & n)
{
    return SdfGetListEditorProxy<SdfPayloadEditorProxy>(o, n);
}

SdfNameOrderProxy
SdfGetNameOrderProxy(const SdfSpecHandle& spec, const TfToken& orderField)
{
    if (!spec) {
        return SdfNameOrderProxy(SdfListOpTypeOrdered);
    }

    std::shared_ptr<Sdf_ListEditor<SdfNameTokenKeyPolicy> > editor(
        new Sdf_VectorListEditor<SdfNameTokenKeyPolicy>(
            spec, orderField, SdfListOpTypeOrdered));
    return SdfNameOrderProxy(editor, SdfListOpTypeOrdered);
}

PXR_NAMESPACE_CLOSE_SCOPE
