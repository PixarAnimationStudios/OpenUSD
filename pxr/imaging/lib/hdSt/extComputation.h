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
#ifndef HDST_EXT_COMPUATION_H
#define HDST_EXT_COMPUATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include <boost/shared_ptr.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
typedef boost::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;

/// \class HdStExtComputation
///
/// Specialization of HdExtComputation which manages inputs as GPU resources.
///
class HdStExtComputation : public HdExtComputation {
public:
    /// Construct a new ExtComputation identified by id.
    HDST_API
    HdStExtComputation(SdfPath const &id);

    HDST_API
    virtual ~HdStExtComputation() = default;

    HDST_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;

    HDST_API
    virtual VtValue Get(TfToken const &token) const override;

    HDST_API
    HdBufferArrayRangeSharedPtr const & GetInputRange() const {
        return _inputRange;
    }

private:
    // No default construction or copying
    HdStExtComputation() = delete;
    HdStExtComputation(const HdStExtComputation &) = delete;
    HdStExtComputation &operator =(const HdStExtComputation &) = delete;

    HdBufferArrayRangeSharedPtr _inputRange;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_EXT_COMPUATION_H
