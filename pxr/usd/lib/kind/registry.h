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
#ifndef KIND_REGISTRY_H
#define KIND_REGISTRY_H

/// \file kind/registry.h

#include "pxr/pxr.h"
#include "pxr/usd/kind/api.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define KIND_TOKENS \
    (model) \
        (component) \
        (group) \
            (assembly) \
    (subcomponent)

/// \anchor KindTokens
/// Provides static, efficient TfToken's for built-in Kinds
///
/// See \ref kind_coreKinds for description of the builtin kinds.
TF_DECLARE_PUBLIC_TOKENS(KindTokens, KIND_API, KIND_TOKENS);

/// \class KindRegistry
///
/// A singleton that holds known kinds and information about them.  See
/// \ref mainpage_kind "Kind Overview" for a description of why kind exists,
/// what the builtin registered kinds are, and how to extend the core kinds.
///
/// \section kind_threadsafety KindRegistry Threadsafty
///
/// KindRegistry serves performance-critical clients that operate under the
/// stl threading model, and therefore itself follows that model in order
/// to avoid locking during HasKind() and IsA() queries. 
///
/// To make this robust, KindRegistry exposes no means to mutate the registry.
/// All extensions must be accomplished via plugInfo.json files, which are
/// consumed once during the registry initialization (See \ref kind_extensions )
class KindRegistry : public TfWeakBase
{
public:
    // Disallow copies
    KindRegistry(const KindRegistry&) = delete;
    KindRegistry& operator=(const KindRegistry&) = delete;

    /// Return the single \c KindRegistry instance.
    KIND_API static KindRegistry& GetInstance();

    /// Test whether \a kind is known to the registry.
    KIND_API static bool HasKind(const TfToken& kind);

    /// Return the base kind of the given kind.
    /// If there is no base, the result will be an empty token.
    /// Issues a coding error if \a kind is unknown to the registry.
    KIND_API static TfToken GetBaseKind(const TfToken &kind);

    /// Test whether \a derivedKind is the same as \a baseKind or
    /// has it as a base kind (either directly or indirectly).
    ///
    /// It is \em not required that \a derivedKind or \a baseKind
    /// be known to the registry: if they are unknown but equal,
    /// IsA will return \c true; otherwise if either is unknown, we
    /// will simply return false.
    ///
    /// Therefore this method will not raise any errors.
    KIND_API static bool IsA(const TfToken& derivedKind, const TfToken &baseKind);

    /// Return an unordered vector of all kinds known to the registry.
    KIND_API static std::vector<TfToken>  GetAllKinds();

private:
    friend class TfSingleton<KindRegistry>;

    KindRegistry();
    virtual ~KindRegistry();
 
    bool _HasKind(const TfToken& kind) const;

    TfToken _GetBaseKind(const TfToken &kind) const;

    bool _IsA(const TfToken& derivedKind, const TfToken &baseKind) const;

    std::vector<TfToken> _GetAllKinds() const;

    /// Register the given \a kind with the given \a baseKind.
    /// It is valid for \a baseKind to be empty (the default),
    /// in which case \a kind represents a root of the kind hierarchy.
    void _Register(const TfToken& kind,
                   const TfToken& baseKind = TfToken());

    void _RegisterDefaults();

    struct _KindData {
        TfToken baseKind;
    };

    typedef std::unordered_map<TfToken, _KindData, TfToken::HashFunctor>
        _KindMap;

private:
    _KindMap _kindMap;
};

KIND_API_TEMPLATE_CLASS(TfSingleton<KindRegistry>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // KIND_REGISTRY_H
