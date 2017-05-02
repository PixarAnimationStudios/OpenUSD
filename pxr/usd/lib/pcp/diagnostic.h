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
#ifndef PCP_DIAGNOSTIC_H
#define PCP_DIAGNOSTIC_H

/// \file pcp/diagnostic.h
/// Diagnostic helpers.

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/errors.h"

#include "pxr/base/arch/hints.h"

#include <boost/preprocessor/cat.hpp>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class PcpLayerStackSite;
class PcpNodeRef;
class PcpPrimIndex;
class PcpSite;
class PcpPrimIndex_StackFrame;

PCP_API
std::string PcpDump(const PcpPrimIndex& primIndex,
                    bool includeInheritOriginInfo = false,
                    bool includeMaps = false);

PCP_API
std::string PcpDump(const PcpNodeRef& node,
                    bool includeInheritOriginInfo = false,
                    bool includeMaps = false);

PCP_API
void PcpDumpDotGraph(const PcpPrimIndex& primIndex, 
                     const char *filename, 
                     bool includeInheritOriginInfo = true,
                     bool includeMaps = false);

PCP_API
void PcpDumpDotGraph(const PcpNodeRef& node, 
                     const char *filename, 
                     bool includeInheritOriginInfo = true,
                     bool includeMaps = false);

// Enable this #define for extra runtime validation.
// This is normally disabled because it is expensive.
// #define PCP_DIAGNOSTIC_VALIDATION

// Private helpers.
inline PcpPrimIndex const *
Pcp_ToIndex(PcpPrimIndex const *index) { return index; }

template <class T>
inline PcpPrimIndex const *
Pcp_ToIndex(T const &obj) { return obj->GetOriginatingIndex(); }

/// \name PcpPrimIndex Debugging Output
///
/// The following macros are used to annotate the prim indexing process.
/// The annotations can be output to the terminal or to .dot graphs to allow 
/// users to trace the steps in indexing.
///
/// @{

/// Opens a scope indicating a particular phase during prim indexing.
#define PCP_INDEXING_PHASE(indexer, node, ...)                                 \
    auto BOOST_PP_CAT(_pcpIndexingPhase, __LINE__) =                           \
        ARCH_UNLIKELY(TfDebug::IsEnabled(PCP_PRIM_INDEX)) ?                    \
        Pcp_IndexingPhaseScope(Pcp_ToIndex(indexer),                           \
                               node, TfStringPrintf(__VA_ARGS__)) :            \
        Pcp_IndexingPhaseScope()

/// Indicates that the prim index currently being constructed has been
/// updated.
#define PCP_INDEXING_UPDATE(indexer, node, ...)                         \
    if (ARCH_UNLIKELY(TfDebug::IsEnabled(PCP_PRIM_INDEX))) {            \
        Pcp_IndexingUpdate(Pcp_ToIndex(indexer), node,                  \
                           TfStringPrintf(__VA_ARGS__));                \
    }

/// Annotates the current phase of prim indexing with the given message.
#define PCP_INDEXING_MSG(indexer, ...)                                  \
    if (ARCH_UNLIKELY(TfDebug::IsEnabled(PCP_PRIM_INDEX))) {            \
        Pcp_IndexingMsg(Pcp_ToIndex(indexer), __VA_ARGS__);             \
    }

/// @}

/// Opens a scope indicating the construction of the prim index
/// \p index for \p site.
class Pcp_PrimIndexingDebug {
public:
    Pcp_PrimIndexingDebug(PcpPrimIndex const *index,
                          PcpPrimIndex const *originatingIndex,
                          PcpLayerStackSite const &site)
        : _index(nullptr)
        , _originatingIndex(nullptr) {
        if (ARCH_UNLIKELY(TfDebug::IsEnabled(PCP_PRIM_INDEX))) {
            _index = index;
            _originatingIndex = originatingIndex;
            _PushIndex(site);
        }
    }          

    Pcp_PrimIndexingDebug(Pcp_PrimIndexingDebug const &) = delete;
    Pcp_PrimIndexingDebug &operator=(Pcp_PrimIndexingDebug const &) = delete;

    inline ~Pcp_PrimIndexingDebug() {
        if (ARCH_UNLIKELY(_index)) {
            _PopIndex();
        }
    }
private:
    void _PushIndex(PcpLayerStackSite const &site) const;
    void _PopIndex() const;

    PcpPrimIndex const *_index;
    PcpPrimIndex const *_originatingIndex;
};

// Implementation details; private helper objects and functions for debugging 
// output. Use the macros above instead.

class Pcp_IndexingPhaseScope {
public:
    Pcp_IndexingPhaseScope() : _index(nullptr) {}
    Pcp_IndexingPhaseScope(PcpPrimIndex const *index,
                           const PcpNodeRef& node, std::string &&msg);
    Pcp_IndexingPhaseScope(Pcp_IndexingPhaseScope const &) = delete;
    Pcp_IndexingPhaseScope(Pcp_IndexingPhaseScope &&other)
        : _index(other._index) {
        other._index = nullptr;
    }
    Pcp_IndexingPhaseScope &operator=(Pcp_IndexingPhaseScope const &) = delete;
    inline Pcp_IndexingPhaseScope &operator=(Pcp_IndexingPhaseScope &&other) {
        if (&other != this) {
            _index = other._index;
            other._index = nullptr;
        }
        return *this;
    }
    inline ~Pcp_IndexingPhaseScope() {
        if (ARCH_UNLIKELY(_index)) {
            _EndScope();
        }
    }    
private:
    void _EndScope() const;
    PcpPrimIndex const *_index;
};

void
Pcp_IndexingMsg(PcpPrimIndex const *index,
                const PcpNodeRef& a1,
                char const *fmt, ...) ARCH_PRINTF_FUNCTION(3, 4);
void
Pcp_IndexingMsg(PcpPrimIndex const *index,
                const PcpNodeRef& a1, const PcpNodeRef& a2,
                char const *fmt, ...) ARCH_PRINTF_FUNCTION(4, 5);
void
Pcp_IndexingUpdate(PcpPrimIndex const *index,
                   const PcpNodeRef& node, std::string &&msg);

PCP_API
std::string Pcp_FormatSite(const PcpSite& site);
PCP_API
std::string Pcp_FormatSite(const PcpLayerStackSite& site);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_DIAGNOSTIC_H
