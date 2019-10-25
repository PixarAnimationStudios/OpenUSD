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
#ifndef HDX_PROGRESSIVE_TASK_H
#define HDX_PROGRESSIVE_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/task.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxProgressiveTask
///
/// This is an interface class that declares that derived tasks implement
/// some form of progressive rendering, as queried by the virtual
/// IsConverged().
///
/// Applications with data-driven task lists can determine their convergence
/// state by determining which tasks are progressive tasks and then querying
/// specifically those tasks.
class HdxProgressiveTask : public HdTask {
public:
    HDX_API
    HdxProgressiveTask(SdfPath const& id);

    HDX_API
    virtual ~HdxProgressiveTask();

    virtual bool IsConverged() const = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_PROGRESSIVE_TASK_H
