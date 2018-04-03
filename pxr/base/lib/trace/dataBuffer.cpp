//
// Copyright 2018 Pixar
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

#include "pxr/base/trace/dataBuffer.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

constexpr size_t _GetMaxAlign() {
    // Our version of GCC does not have std::max_align_t
    using namespace std;
    return alignof(max_align_t);
}

void
TraceDataBuffer::Allocator::AllocateBlock(const size_t align, 
                                         const size_t desiredSize)
{
    const size_t maxAlign = _GetMaxAlign();
    const size_t blockSize = 
        std::max(align > maxAlign ? (align + desiredSize) : desiredSize,
            _desiredBlockSize);
    BlockPtr block(new Byte[ blockSize ]);
    _next = block.get();
    _blockEnd = _next + blockSize;
    _blocks.push_back(std::move(block));
}

PXR_NAMESPACE_CLOSE_SCOPE
