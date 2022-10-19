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
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/tf/hash.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


/*static*/
bool
HdBufferSpec::IsSubset(HdBufferSpecVector const &subset,
                       HdBufferSpecVector const &superset)
{
    HD_TRACE_FUNCTION();

    for(HdBufferSpec const& spec : subset) {
        if (std::find(superset.begin(), superset.end(), spec) == superset.end())
            return false;
    }
    return true;
}

/*static*/
HdBufferSpecVector
HdBufferSpec::ComputeUnion(HdBufferSpecVector const &specs1,
                           HdBufferSpecVector const &specs2)
{
    // This implementation assumes small inputs.

    HD_TRACE_FUNCTION();

    HdBufferSpecVector r;

    for (HdBufferSpec const& s : specs1) {
        if (std::find(r.begin(), r.end(), s) != r.end()) {
            continue;
        }
        r.push_back(s);
    }
    for (HdBufferSpec const& s : specs2) {
        if (std::find(r.begin(), r.end(), s) != r.end()) {
            continue;
        }
        r.push_back(s);
    }

    return r;
}

/*static*/
HdBufferSpecVector
HdBufferSpec::ComputeDifference(HdBufferSpecVector const &specs1,
                                HdBufferSpecVector const &specs2)
{
    // This implementation assumes small inputs.

    HD_TRACE_FUNCTION();

    HdBufferSpecVector r;

    for(HdBufferSpec const& s : specs1) {
        if (std::find(specs2.begin(), specs2.end(), s) != specs2.end()) {
            continue;
        }
        if (std::find(r.begin(), r.end(), s) != r.end()) {
            continue;
        }
        r.push_back(s);
    }

    return r;
}

size_t
HdBufferSpec::Hash() const
{
    return TfHash()(*this);
}

void
HdBufferSpec::Dump(HdBufferSpecVector const &specs)
{
    std::cout << "BufferSpecVector\n";
    for (int i = 0; i < (int)specs.size(); ++i) {
        std::cout << i << " : "
                  << specs[i].name << ", "
                  << TfEnum::GetName(specs[i].tupleType.type) << " ("
                  << specs[i].tupleType.type << "), "
                  << specs[i].tupleType.count << "\n";
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

