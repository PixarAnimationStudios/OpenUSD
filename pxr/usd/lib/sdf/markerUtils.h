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
#ifndef SDF_MARKER_UTILS_H
#define SDF_MARKER_UTILS_H

#include "pxr/usd/sdf/path.h"
#include <string>

template <class T> class Sdf_MarkerUtilsPolicy;

// Helper functions for common functionality related to markers on
// attribute connections and relationship targets.
//
// Note that 'connection path' is considered synonymous to 'target path'
// for the interface below.
template <class Spec>
class Sdf_MarkerUtils
{
public:
    // Returns all connection paths on \p owner for which a marker is
    // defined.
    static SdfPathVector GetMarkerPaths(const Spec& owner);

    // Returns the marker on the given \p connectionPath on \p owner.
    // If no marker is specified, the empty string is returned.
    static std::string GetMarker(
        const Spec& owner, const SdfPath& connectionPath);

    // Sets the marker on the given \p connectionPath on \p owner to
    // \p marker. If \p marker is empty, any existing marker will be cleared.
    static void SetMarker(
        Spec* owner, const SdfPath& connectionPath, 
        const std::string& marker);

    // Clears the marker on the given \p connectionPath on \p owner.
    static void ClearMarker(
        Spec* owner, const SdfPath& connectionPath);

    // Map from connection path to marker used for bulk replacement
    // of markers.
    typedef std::map<SdfPath, std::string, SdfPath::FastLessThan> MarkerMap;

    // Sets all markers on \p owner to those specified in \p markers.
    static void SetMarkers(
        Spec* owner, const MarkerMap& markers);

private:
    typedef Sdf_MarkerUtilsPolicy<Spec> _MarkerPolicy;

    // Unimplemented private c'tor to disallow construction of an 
    // Sdf_MarkerUtils object.
    Sdf_MarkerUtils();
};

#endif // SDF_MARKER_UTILS_H
