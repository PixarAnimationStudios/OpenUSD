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
#ifndef HD_SCENE_EXT_COMP_INPUT_SOURCE_H
#define HD_SCENE_EXT_COMP_INPUT_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/extCompInputSource.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// An Hd Buffer Source Computation that is used to bind an ExtComputation input
/// to a value provided by the scene delegate.
///
class Hd_SceneExtCompInputSource : public Hd_ExtCompInputSource {
public:
    /// Constructs the computation, binding inputName to the provided value.
    Hd_SceneExtCompInputSource(const TfToken &inputName, const VtValue &value);
    virtual ~Hd_SceneExtCompInputSource() = default;

    /// Set the state of the computation to resolved and returns true.
    virtual bool Resolve() override;

    /// Returns the value associated with this input.
    virtual const VtValue &GetValue() const override;


protected:
    /// Returns if this computation binding is valid.
    virtual bool _CheckValid() const override;

private:
    VtValue _value;

    // No copying, assignment or default construction.
    Hd_SceneExtCompInputSource()                                       = delete;
    Hd_SceneExtCompInputSource(const Hd_SceneExtCompInputSource &)     = delete;
    Hd_SceneExtCompInputSource &operator = (const Hd_SceneExtCompInputSource &)
                                                                       = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_SCENE_EXT_COMP_INPUT_SOURCE_H
