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
#ifndef HD_BUFFER_SPEC_H
#define HD_BUFFER_SPEC_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/token.h"
#include <vector>

typedef std::vector<class HdBufferSpec> HdBufferSpecVector;

/// HdBufferSpec describes each named resource of buffer array.
///
/// for example:
/// HdBufferSpecVector
///    0: name = points, glDataType = GL_FLOAT, numComponents = 3
///    1: name = normals, glDataType = GL_FLOAT, numComponents = 3
///    2: name = colors, glDataType = GL_FLOAT, numComponents = 4
///

struct HdBufferSpec {
    /// Constructor.
    HdBufferSpec(TfToken const &name, GLenum glDataType, int numComponents,
                 int arraySize=1) :
        name(name), glDataType(glDataType), numComponents(numComponents),
        arraySize(arraySize) {}

    /// Util function for adding buffer specs of sources into bufferspecs.
    template<typename T>
    static void AddBufferSpecs(HdBufferSpecVector *bufferSpecs,
                               T const &sources) {
        TF_FOR_ALL (it, sources) {
            if ((*it)->IsValid()) {
                (*it)->AddBufferSpecs(bufferSpecs);
            }
        }
    }

    /// Returns true if \p subset is a subset of \p superset.
    static bool IsSubset(HdBufferSpecVector const &subset,
                         HdBufferSpecVector const &superset);

    /// Returns union set of \p spec1 and \p spec2.
    /// Duplicated entries are uniquified.
    static HdBufferSpecVector ComputeUnion(HdBufferSpecVector const &spec1,
                                           HdBufferSpecVector const &spec2);

    /// Debug output
    static void Dump(HdBufferSpecVector const &specs);

    /// equality checks
    bool operator == (HdBufferSpec const &other) const {
        return name == other.name and
            glDataType == other.glDataType and
            numComponents == other.numComponents and
            arraySize == other.arraySize;
    }
    bool operator != (HdBufferSpec const &other) const {
        return not (*this == other);
    }

    /// ordering
    bool operator < (HdBufferSpec const &other) const {
        return name < other.name or (name == other.name and
              (glDataType < other.glDataType or (glDataType == other.glDataType and
              (numComponents < other.numComponents or (numComponents == other.numComponents and
              (arraySize < other.arraySize))))));
    }

    TfToken name;
    GLenum glDataType;
    int numComponents;
    int arraySize;
};

#endif  // HD_BUFFER_SPEC_H
