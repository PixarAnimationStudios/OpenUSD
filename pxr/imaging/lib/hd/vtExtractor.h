//
// Copyright 2017 Pixar
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
#ifndef HD_VT_EXTRACTOR_H
#define HD_VT_EXTRACTOR_H

#include "pxr/pxr.h"
#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

class VtValue;

///
/// Internal utility class for obtaining type information and
/// raw byte access to a value stored in the VtValue type.
///
class Hd_VtExtractor final {
public:
    Hd_VtExtractor();
    ~Hd_VtExtractor() = default;

    ///
    /// Process the pass in value and store the information
    /// about the value in the class members.
    ///
    void Extract(const VtValue &value);

    ///
    /// Returns the type of the single components that make up an
    /// element in the VtValue, if each component is of a uniform type.
    /// If the components are non-uniform, the type is the same as the element
    /// type and number of components is 1.
    ///
    /// Example: GfVec3f would return a float type.
    ///
    int GetGLCompontentType() const { return _glComponentType; }

    ///
    /// Returns the overall type of the elements as a whole in the VtValue.
    ///
    /// Example: GfVec3f would return a float3 type.
    ///
    int GetGLElementType()    const { return _glElementType;   }

    ///
    /// Returns the total size of the contained data in bytes.
    ///
    size_t GetSize()          const { return _size;            }

    ///
    /// If the components that make up an element are of a uniform type,
    /// returns the number of components that make up the element.
    ///
    /// For non-uniform elements, the value is always 1.
    ///
    /// Example: GfVec3f would return 3.
    ///
    short GetNumComponents()  const { return _numComponents;   }

    ///
    /// Returns a raw pointer to the data stored in the VtValue.
    /// This pointer is valid as long as the VtValue passed to Extract
    /// is still alive.
    ///
    void const* GetData()     const { return _data;            }

private:
    int         _glComponentType;
    int         _glElementType;
    size_t      _size;
    short       _numComponents;
    void const *_data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VT_EXTRACTOR_H
