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
#ifndef TF_ERROR_MARK
#define TF_ERROR_MARK

/// \file tf/errorMark.h

#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/errorTransport.h"

#include <boost/noncopyable.hpp>

/// \class TfErrorMark
/// \ingroup group_tf_TfError
///
/// Class used to record the end of the error-list.
///
/// See \ref page_tf_TfError for a detailed description.
///
/// A \c TfErrorMark is used as follows:
/// \code
///     TfErrorMark m;
///
///     m.SetMark();             // (A)
///     ... ;
///     ... ;
///                              // (B)
///     if (!m.IsClean()) {
///       // errors occurred between (A) and (B)
///     }
/// \endcode
///
/// Another common pattern is
/// \code
///      TfErrorMark m;
///      if (TF_HAS_ERRORS(m, expr)) {
///          // handle errors;
///      }
/// \endcode
///
class TfErrorMark : boost::noncopyable
{
  public:

    typedef TfDiagnosticMgr::ErrorIterator Iterator;
    
    /// Default constructor.
    ///
    /// The default constructor automatically calls \c SetMark() at the point
    /// of declaration.
    TfErrorMark();

    /// Destroy this ErrorMark.
    ///
    /// If this is the last ErrorMark on this thread of execution and there
    /// are pending errors, this will report them via the diagnostic delegate
    /// (if one is instanlled) otherwise by printing to stderr.
    ~TfErrorMark();

    /// Record future errors.
    ///
    /// \c SetMark() arranges to record future errors in \c *this.
    inline void SetMark() {
        _mark = TfDiagnosticMgr::GetInstance()._nextSerial;
    }

    /// Return true if no new errors were posted in this thread since the last
    /// call to \c SetMark(), false otherwise.
    ///
    /// When no threads are issuing errors the cost of this function is an
    /// atomic integer read and comparison.  Otherwise thread-specific data is
    /// accessed to make the determination.  Thus, this function is fast when
    /// diagnostics are not being issued.
    inline bool IsClean() const {
        TfDiagnosticMgr &mgr = TfDiagnosticMgr::GetInstance();
        return _mark >= mgr._nextSerial || _IsCleanImpl(mgr);
    }

    /// Remove all errors in this mark from the error system.  Return true if
    /// any errors were cleared, false if there were no errors in this mark.
    ///
    /// Clear all errors contained in this mark from the error system.
    /// Subsequently, these errors will be considered handled.
    inline bool Clear() const {
        TfDiagnosticMgr &mgr = TfDiagnosticMgr::GetInstance();
        auto b = GetBegin(), e = mgr.GetErrorEnd();
        if (b != e) {
            mgr.EraseRange(b, e);
            return true;
        }
        return false;
    }

    /// Remove all errors in this mark fom the error system and return them in
    /// a TfErrorTransport.
    /// 
    /// This can be used to transfer errors from one thread to another.  See
    /// TfErrorTransport for more information.  As with Clear(), all the
    /// removed errors are considered handled for this thread.  See also
    /// TransportTo().
    inline TfErrorTransport Transport() const {
        TfDiagnosticMgr &mgr = TfDiagnosticMgr::GetInstance();
        return TfErrorTransport(mgr._errorList.local(),
                                GetBegin(), mgr.GetErrorEnd());
    }

    /// Remove all errors in this mark fom the error system and return them in
    /// a TfErrorTransport.
    /// 
    /// This is a variant of Transport().  Instead of returning a new
    /// TfErrorTransport object it fills an existing one.
    inline void TransportTo(TfErrorTransport &dest) const {
        Transport().swap(dest);
    }

    /// Return an iterator to the first error added to the error list after
    /// \c SetMark().
    ///
    /// If there are no errors on the error list that were not already present
    /// when \c SetMark() was called, the iterator returned is equal to the
    /// iterator returned by \c TfDiagnosticMgr::GetErrorEnd(). Otherwise, the
    /// iterator points to the earliest error added to the list since
    /// \c SetMark() was called.
    ///
    /// This function takes O(n) time where n is the number of errors from the
    /// end of the list to the mark i.e. \c GetMark() walks the list from the
    /// end until it finds the mark and then returns an iterator to that spot.
    ///
    /// If \c nErrors is non-NULL, then \c *nErrors is set to the number of
    /// errors between the returned iterator and the end of the list.
    Iterator GetBegin(size_t *nErrors = 0) const {
        return
            TfDiagnosticMgr::GetInstance()._GetErrorMarkBegin(_mark, nErrors);
    }

    /// Return an iterator past the last error in the error system.
    ///
    /// This iterator is always equivalend to the iterator returned by \c
    /// TfDiagnosticMgr::GetErrorEnd().
    Iterator GetEnd() const {
        return TfDiagnosticMgr::GetInstance().GetErrorEnd();
    }
    
  private:
    friend class TfDiagnosticMgr;

    // Helper to check if the _mark identifies any errors present on the
    // thread-local error list.
    bool _IsCleanImpl(TfDiagnosticMgr &mgr) const;

    void _ReportErrors(TfDiagnosticMgr &mgr) const;

    size_t _mark;
};


/// Convenience macro to check if errors occurred.
///
/// This macro is equivalent to
/// \code
///     (marker.SetMark(), (expr), !marker.IsClean())
/// \endcode
///
/// which enables it to be used as an expression:
/// \code
///    if (TF_HAS_ERRORS(m, expr))
///       // cope!
/// \endcode
///
/// \ingroup group_tf_TfError
/// \hideinitializer
#define TF_HAS_ERRORS(marker, expr) \
        (marker.SetMark(), (expr), !marker.IsClean())

/// Report current TfErrorMark instances and the stack traces that created
/// them to stdout for debugging purposes.
///
/// To call this function, set _enableTfErrorMarkStackTraces in errorMark.cpp
/// and enable the TF_ERROR_MARK_TRACKING TfDebug code.
///
/// \ingroup group_tf_TfError
void TfReportActiveErrorMarks();

#endif // TF_ERROR_MARK
