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
#ifndef HD_COMPUTATION_H
#define HD_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"
#include <boost/shared_ptr.hpp>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;
typedef boost::shared_ptr<class HdComputation> HdComputationSharedPtr;
typedef std::vector<HdComputationSharedPtr> HdComputationVector;

/// \class HdComputation
///
/// An interface class for GPU computation.
///
/// GPU computation fills the result into range, which has to be allocated
/// using buffer specs determined by AddBufferSpecs, and registered as a pair
/// of computation and range.
///
class HdComputation
{
public:
    HD_API
    virtual ~HdComputation();

    /// Execute computation.
    virtual void Execute(
        HdBufferArrayRangeSharedPtr const &range,
        HdResourceRegistry *resourceRegistry) = 0;

    /// Returns the size of its destination buffer (located by range argument
    /// of Execute()). This function will be called after all HdBufferSources
    /// have been resolved and commited, so it can use the result of those
    /// buffer source results.
    /// Returning 0 means it doesn't need to resize.
    virtual int GetNumOutputElements() const = 0;

    /// Add the buffer spec for this computation into given bufferspec vector.
    /// Caller has to allocate the destination buffer with respect to the
    /// BufferSpecs, and passes the range when registering the computation.
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const = 0;

    /// This function is needed as HdComputation shares a templatized interface
    /// with HdBufferSource.
    ///
    /// It is a check to see if the AddBufferSpecs would produce a valid result.
    bool IsValid() { return true; }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_COMPUTATION_H
