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

#include "pxr/usd/pcp/errors.h"

#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>

class PcpLayerStackSite;
class PcpNodeRef;
class PcpPrimIndex;
class PcpSite;

std::string PcpDump(const PcpPrimIndex& primIndex,
                    bool includeInheritOriginInfo = false,
                    bool includeMaps = false);

std::string PcpDump(const PcpNodeRef& node,
                    bool includeInheritOriginInfo = false,
                    bool includeMaps = false);

void PcpDumpDotGraph(const PcpPrimIndex& primIndex, 
                     const char *filename, 
                     bool includeInheritOriginInfo = true,
                     bool includeMaps = false);

void PcpDumpDotGraph(const PcpNodeRef& node, 
                     const char *filename, 
                     bool includeInheritOriginInfo = true,
                     bool includeMaps = false);

// Enable this #define for extra runtime validation.
// This is normally disabled because it is expensive.
// #define PCP_DIAGNOSTIC_VALIDATION

/// \name PcpPrimIndex Debugging Output
///
/// The following macros are used to annotate the prim indexing process.
/// The annotations can be output to the terminal or to .dot graphs to allow 
/// users to trace the steps in indexing.
///
/// @{

/// Opens a scope indicating the construction of the prim index
/// \p index for \p site.
#define PCP_GRAPH(index, site)                                          \
    Pcp_GraphScope _graph(index, site);

/// Opens a scope indicating a particular phase during prim indexing.
#define PCP_GRAPH_PHASE(node, ...)                                      \
    Pcp_PhaseScope BOOST_PP_CAT(_pcpPhase, __LINE__)(node,              \
        TfDebug::IsEnabled(PCP_PRIM_INDEX)                              \
            ? Pcp_PhaseScope::Helper(__VA_ARGS__).c_str()               \
            : NULL);

/// Indicates that the prim index currently being constructed has been
/// updated.
#define PCP_GRAPH_UPDATE(...)                                           \
    if (!TfDebug::IsEnabled(PCP_PRIM_INDEX)) { }                     \
    else {                                                              \
        Pcp_GraphUpdate(__VA_ARGS__);                                   \
    }

/// Annotates the current phase of prim indexing with the given message.
#define PCP_GRAPH_MSG(...)                                              \
    if (!TfDebug::IsEnabled(PCP_PRIM_INDEX)) { }                     \
    else {                                                              \
        Pcp_GraphMsg(__VA_ARGS__);                                      \
    }

/// @}

// Implementation details; private helper objects and functions for debugging 
// output. Use the macros above instead.
class Pcp_GraphScope {
public:
    Pcp_GraphScope(PcpPrimIndex* index, const PcpLayerStackSite& site);
    ~Pcp_GraphScope();
private:
    bool _on;
};

class Pcp_PhaseScope {
public:
    Pcp_PhaseScope(const PcpNodeRef& node, const char* msg);
    ~Pcp_PhaseScope();

    static std::string Helper(const char* f, ...) ARCH_PRINTF_FUNCTION(1, 2);

private:
    bool _on;
};

#define BOOST_PP_LOCAL_LIMITS (0, 2)
#define BOOST_PP_LOCAL_MACRO(n)                                        \
void Pcp_GraphMsg(                                                     \
    BOOST_PP_ENUM_PARAMS(n, const PcpNodeRef& a) BOOST_PP_COMMA_IF(n)  \
    const char* format, ...)                                           \
    ARCH_PRINTF_FUNCTION(BOOST_PP_ADD(n, 1), BOOST_PP_ADD(n, 2));

#include BOOST_PP_LOCAL_ITERATE()

void 
Pcp_GraphUpdate(
    const PcpNodeRef& node, const char* msg, ...) ARCH_PRINTF_FUNCTION(2, 3);

std::string
Pcp_FormatSite(const PcpSite& site);

std::string
Pcp_FormatSite(const PcpLayerStackSite& site);

#endif
