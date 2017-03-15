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
#ifndef HDX_TASK_CONTROLLER_H
#define HDX_TASK_CONTROLLER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/task.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdxTaskController {
public:
    /// Obtain the set of tasks managed by the task controller
    /// suitable for execution.
    virtual HdTaskSharedPtrVector const &GetTasks() = 0;

    /// Specifies the camera to be used when executing the
    /// tasks managed by this controller.
    /// @param  cameraId The path to the camera Sprim.
    virtual void SetCamera(const SdfPath &cameraId) = 0;


protected:
    /// This class must be derived from
    HdxTaskController()          = default;
    virtual ~HdxTaskController();

private:
    ///
    /// This class is not intended to be copied.
    ///
    HdxTaskController(const HdxTaskController &) = delete;
    HdxTaskController &operator=(const HdxTaskController &) = delete;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_TASK_CONTROLLER_H
