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
#ifndef TF_DIAGNOSTIC_NOTICE_H
#define TF_DIAGNOSTIC_NOTICE_H

/// \file tf/diagnosticNotice.h

#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/diagnosticBase.h"

class TfWarning;
class TfStatus;
class TfDiagnosticBase;

#include <string>

/// \class TfDiagnosticNotice
///
/// This is a wrapper class containing the actual notices that TfDiagnosticMgr
/// sends.
class TfDiagnosticNotice {
public:

    /// Return if warnings/error are echoed to stderr.
    static bool GetStderrOutputState();

    /// Set stderr output state, returning the previous state.
    static bool SetStderrOutputState(bool state);

    /// Base notification class for TfDiagnosticMgr.
    class Base : public TfNotice {
    public:
        Base();
        TfDiagnosticBase const& GetDiagnosticData() const;

        ~Base();

        bool IsFatal() const {
            return _data->IsFatal();
        }

        bool IsCodingError() const {
            return _data->IsCodingError();
        }
        
        std::string const& GetExplanation() const {
            return _data->GetCommentary();
        }

    protected:
        void _SetData(TfDiagnosticBase const& data);
        
    private:
        TfDiagnosticBase const* _data;
    };

    /// Notification sent when an error is issued.
    class IssuedError: public Base {
      public:
        IssuedError(const TfError &error);
        virtual ~IssuedError();

        const TfError &GetError() const { return _error; }

      private:
        const TfError &_error;
    };

    /// Notification sent when a warning is issued.
    class IssuedWarning : public Base {
      public:
        IssuedWarning(const TfWarning &warning);
        virtual ~IssuedWarning();

        const TfWarning &GetWarning() const { return _warning; }

      private:
        const TfWarning &_warning;
    };

    /// Notification sent when a status message is issued.
    class IssuedStatus : public Base {
      public:
        IssuedStatus(const TfStatus &status);
        virtual ~IssuedStatus();

        const TfStatus &GetStatus() const { return _status; }

      private:
        const TfStatus &_status;
    };

    /// Notification sent when a fatal error is encountered.
    class IssuedFatalError : public Base {
      public:
        IssuedFatalError(const std::string &msg, const TfCallContext &context);
        virtual ~IssuedFatalError();

        void SetData(TfDiagnosticBase const&);
        
        const std::string &GetMessage() const { return _msg; }

        const TfCallContext &GetContext() const { return _context; }

      private:
        const std::string &_msg;
        const TfCallContext &_context;
        TfDiagnosticBase _data;
    };
private:
    static bool _stderrOutputState;
};

#endif
