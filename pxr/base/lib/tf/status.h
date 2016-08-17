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
#ifndef TF_STATUS_H
#define TF_STATUS_H

#include "pxr/base/tf/diagnosticBase.h"

/// \class TfStatus
/// \ingroup group_tf_Diagnostic
///
/// Represents an object that contains information about a status message.
/// 
/// See \ref page_tf_Diagnostic in the C++ API reference for a detailed
/// description of the status message issuing API.  For a example of how to
/// post a status message, see \c TF_STATUS(), also in the C++ API reference.
///
/// In the Python API, you can issue a status message with Tf.Status().
///
class TfStatus: public TfDiagnosticBase
{
private:
    TfStatus(TfEnum statusCode, char const *statusCodeString,
            TfCallContext const &context, const std::string& commentary,
            TfDiagnosticInfo info, bool quiet)
        : TfDiagnosticBase(statusCode, statusCodeString, context,
                            commentary, info, quiet)
    { }

    friend class TfDiagnosticMgr;
};

#endif // TF_STATUS_H
