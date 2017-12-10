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
#ifndef TF_SCOPEDESCRIPTION_PRIVATE_H
#define TF_SCOPEDESCRIPTION_PRIVATE_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

// Helper class for getting the TfScopeDescription stacks as human readable text
// for crash reporting.
class Tf_ScopeDescriptionStackReportLock
{
    Tf_ScopeDescriptionStackReportLock(
        Tf_ScopeDescriptionStackReportLock const &) = delete;
    Tf_ScopeDescriptionStackReportLock &operator=(
        Tf_ScopeDescriptionStackReportLock const &) = delete;
public:
    // Lock and compute the report message.
    Tf_ScopeDescriptionStackReportLock();

    // Unlock.
    ~Tf_ScopeDescriptionStackReportLock();

    // Get the report message.  This could be nullptr if it was impossible to
    // obtain the report.
    char const *GetMessage() const { return _msg; }
    
private:
    char const *_msg;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_SCOPEDESCRIPTION_PRIVATE_H

