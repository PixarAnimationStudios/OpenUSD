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
#include "pxr/base/tf/diagnosticNotice.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/status.h"
#include "pxr/base/tf/warning.h"
#include "pxr/base/tf/diagnosticBase.h"
#include "pxr/base/tf/type.h"

using std::string;

bool TfDiagnosticNotice::_stderrOutputState = true;

bool
TfDiagnosticNotice::GetStderrOutputState()
{
    return TfDiagnosticNotice::_stderrOutputState;
}

bool
TfDiagnosticNotice::SetStderrOutputState(bool state)
{
    bool oldState = TfDiagnosticNotice::_stderrOutputState;
    TfDiagnosticNotice::_stderrOutputState = state;
    return oldState;
}

void
Tf_DiagnosticNoticeDefineTypes() {
    static bool ran = false;
    if (ran)
        return;

    ran = true;
    TfType::Define< TfDiagnosticNotice::Base,
                    TfType::Bases<TfNotice> >();
    TfType::Define< TfDiagnosticNotice::IssuedError,
                    TfType::Bases<TfDiagnosticNotice::Base> >();
    TfType::Define< TfDiagnosticNotice::IssuedWarning,
                    TfType::Bases<TfDiagnosticNotice::Base> >();
    TfType::Define< TfDiagnosticNotice::IssuedStatus,
                    TfType::Bases<TfDiagnosticNotice::Base> >();
    TfType::Define< TfDiagnosticNotice::IssuedFatalError,
                    TfType::Bases<TfDiagnosticNotice::Base> >();
}

TF_REGISTRY_FUNCTION(TfType)
{
    Tf_DiagnosticNoticeDefineTypes();
}

TfDiagnosticNotice::Base::Base()
    : _data(0)
{
}

void
TfDiagnosticNotice::Base::_SetData(TfDiagnosticBase const& data)
{
    _data = &data;
}

TfDiagnosticBase const&
TfDiagnosticNotice::Base::GetDiagnosticData() const
{
    return *_data;
}

/* virtual */
TfDiagnosticNotice::Base::~Base() {}

TfDiagnosticNotice::IssuedError::IssuedError(TfError const& error)
    : _error(error)
{
    _SetData(_error);
}

/* virtual */
TfDiagnosticNotice::IssuedError::~IssuedError() {}


TfDiagnosticNotice::IssuedWarning::IssuedWarning(TfWarning const& warning)
    : _warning(warning)
{
    _SetData(_warning);
}

/* virtual */
TfDiagnosticNotice::IssuedWarning::~IssuedWarning() {}

TfDiagnosticNotice::IssuedStatus::IssuedStatus(TfStatus const& status)
    : _status(status)
{
    _SetData(status);
}

/* virtual */
TfDiagnosticNotice::IssuedStatus::~IssuedStatus() {}

TfDiagnosticNotice::IssuedFatalError::IssuedFatalError(string const& msg,
                                                       TfCallContext const& context)
    : _msg(msg),
      _context(context),
      _data(TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE, "", context, msg,
            TfDiagnosticInfo(), false /*quiet*/)
{
    _SetData(_data);
}

void
TfDiagnosticNotice::IssuedFatalError::SetData(TfDiagnosticBase const& data)
{
    _SetData(data);
}

/* virtual */
TfDiagnosticNotice::IssuedFatalError::~IssuedFatalError() {}
