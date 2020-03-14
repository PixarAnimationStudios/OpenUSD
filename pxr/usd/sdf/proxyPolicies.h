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
#ifndef PXR_USD_SDF_PROXY_POLICIES_H
#define PXR_USD_SDF_PROXY_POLICIES_H

/// \file sdf/proxyPolicies.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/spec.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfReference;

/// \class SdfNameKeyPolicy
///
/// Key policy for \c std::string names.
///
class SdfNameKeyPolicy {
public:
    typedef std::string value_type;

    static const value_type& Canonicalize(const value_type& x)
    {
        return x;
    }
    
    static const std::vector<value_type>& Canonicalize(
        const std::vector<value_type>& x) 
    {
        return x;
    }
};

/// \class SdfNameTokenKeyPolicy
///
/// Key policy for \c TfToken names.
///
class SdfNameTokenKeyPolicy {
public:
    typedef TfToken value_type;

    static const value_type& Canonicalize(const value_type& x)
    {
        return x;
    }

    static const std::vector<value_type>& Canonicalize(
        const std::vector<value_type>& x) 
    {
        return x;
    }
};

/// \class SdfPathKeyPolicy
///
/// Key policy for \c SdfPath; converts all SdfPaths to absolute.
///
class SdfPathKeyPolicy {
public:
    typedef SdfPath value_type;

    SdfPathKeyPolicy() { }
    explicit SdfPathKeyPolicy(const SdfSpecHandle& owner) : _owner(owner) { }


    value_type Canonicalize(const value_type& x) const
    {
        return _Canonicalize(x, _GetAnchor());
    }

    std::vector<value_type> Canonicalize(const std::vector<value_type>& x) const
    {
        if (x.empty()) {
            return x;
        }

        const SdfPath anchor = _GetAnchor();

        std::vector<value_type> result = x;
        TF_FOR_ALL(it, result) {
            *it = _Canonicalize(*it, anchor);
        }
        return result;
    }

private:
    // Get the most recent SdfPath of the owning object, for expanding
    // relative SdfPaths to absolute
    SdfPath _GetAnchor() const
    {
        return _owner ? _owner->GetPath().GetPrimPath() :
                        SdfPath::AbsoluteRootPath();
    }

    value_type _Canonicalize(const value_type& x, const SdfPath& primPath) const
    {
        return x.IsEmpty() ? value_type() : x.MakeAbsolutePath(primPath);
    }

private:
    SdfSpecHandle _owner;
};

// Cannot get from a VtValue except as the correct type.
template <>
struct Vt_DefaultValueFactory<SdfPathKeyPolicy> {
    static Vt_DefaultValueHolder Invoke() {
        TF_AXIOM(false && "Failed VtValue::Get<SdfPathKeyPolicy> not allowed");
        return Vt_DefaultValueHolder::Create((void*)0);
    }
};

/// \class SdfPayloadTypePolicy
///
/// List editor type policy for \c SdfPayload.
///
class SdfPayloadTypePolicy {
public:
    typedef SdfPayload value_type;

    static const value_type& Canonicalize(const value_type& x)
    {
        return x;
    }

    static const std::vector<value_type>& Canonicalize(
        const std::vector<value_type>& x)
    {
        return x;
    }
};

// Cannot get from a VtValue except as the correct type.
template <>
struct Vt_DefaultValueFactory<SdfPayloadTypePolicy> {
    static Vt_DefaultValueHolder Invoke() {
        TF_AXIOM(false && "Failed VtValue::Get<SdfPayloadTypePolicy> not allowed");
        return Vt_DefaultValueHolder::Create((void*)0);
    }
};

/// \class SdfReferenceTypePolicy
///
/// List editor type policy for \c SdfReference.
///
class SdfReferenceTypePolicy {
public:
    typedef SdfReference value_type;

    static const value_type& Canonicalize(const value_type& x)
    {
        return x;
    }

    static const std::vector<value_type>& Canonicalize(
        const std::vector<value_type>& x)
    {
        return x;
    }
};

// Cannot get from a VtValue except as the correct type.
template <>
struct Vt_DefaultValueFactory<SdfReferenceTypePolicy> {
    static Vt_DefaultValueHolder Invoke() {
        TF_AXIOM(false && "Failed VtValue::Get<SdfReferenceTypePolicy> not allowed");
        return Vt_DefaultValueHolder::Create((void*)0);
    }
};

/// \class SdfSubLayerTypePolicy
///
/// List editor type policy for sublayers.
///
class SdfSubLayerTypePolicy {
public:
    typedef std::string value_type;

    static const value_type& Canonicalize(const value_type& x)
    {
        return x;
    }

    static const std::vector<value_type>& Canonicalize(
        const std::vector<value_type>& x)
    {
        return x;
    }
};

/// \class SdfRelocatesMapProxyValuePolicy
///
/// Map edit proxy value policy for relocates maps.  This absolutizes all
/// paths.
///
class SdfRelocatesMapProxyValuePolicy {
public:
    typedef std::map<SdfPath, SdfPath> Type;
    typedef Type::key_type key_type;
    typedef Type::mapped_type mapped_type;
    typedef Type::value_type value_type;

    SDF_API
    static Type CanonicalizeType(const SdfSpecHandle& v, const Type& x);
    SDF_API
    static key_type CanonicalizeKey(const SdfSpecHandle& v,
                                    const key_type& x);
    SDF_API
    static mapped_type CanonicalizeValue(const SdfSpecHandle& v,
                                         const mapped_type& x);
    SDF_API
    static value_type CanonicalizePair(const SdfSpecHandle& v,
                                       const value_type& x);
};

/// \class SdfGenericSpecViewPredicate
///
/// Predicate for viewing properties.
///
class SdfGenericSpecViewPredicate {
public:
    SdfGenericSpecViewPredicate(SdfSpecType type) : _type(type) { }

    template <class T>
    bool operator()(const SdfHandle<T>& x) const
    {
        // XXX: x is sometimes null. why?
        if (x) {
            return x->GetSpecType() == _type;
        }
        return false;
    }

private:
    SdfSpecType _type;
};

/// \class SdfAttributeViewPredicate
///
/// Predicate for viewing attributes.
///
class SdfAttributeViewPredicate : public SdfGenericSpecViewPredicate {
public:
    SDF_API
    SdfAttributeViewPredicate();
};

/// \class SdfRelationshipViewPredicate
///
/// Predicate for viewing relationships.
///
class SdfRelationshipViewPredicate : public SdfGenericSpecViewPredicate {
public:
    SDF_API
    SdfRelationshipViewPredicate();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PROXY_POLICIES_H
