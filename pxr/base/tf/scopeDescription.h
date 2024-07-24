//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SCOPE_DESCRIPTION_H
#define PXR_BASE_TF_SCOPE_DESCRIPTION_H

#include "pxr/pxr.h"

#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/api.h"

#include <optional>
#include <vector>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfScopeDescription
///
/// This class is used to provide high-level descriptions about scopes of
/// execution that could possibly block, or to provide relevant information
/// about high-level action that would be useful in a crash report.
///
/// This class is reasonably fast to use, especially if the message strings are
/// not dynamically created, however it should not be used in very highly
/// performance sensitive contexts.  The cost to push & pop is essentially a TLS
/// lookup plus a couple of atomic operations.
class TfScopeDescription
{
    TfScopeDescription() = delete;
    TfScopeDescription(TfScopeDescription const &) = delete;
    TfScopeDescription &operator=(TfScopeDescription const &) = delete;
public:
    /// Construct with a description.  Push \a description on the stack of
    /// descriptions for this thread.  Caller guarantees that the string
    /// \p description lives at least as long as this TfScopeDescription object.
    TF_API explicit
    TfScopeDescription(std::string const &description,
                       TfCallContext const &context = TfCallContext());

    /// Construct with a description.  Push \a description on the stack of
    /// descriptions for this thread.  This object adopts ownership of the
    /// rvalue \p description.
    TF_API explicit
    TfScopeDescription(std::string &&description,
                       TfCallContext const &context = TfCallContext());

    /// Construct with a description.  Push \a description on the stack of
    /// descriptions for this thread.  Caller guarantees that the string
    /// \p description lives at least as long as this TfScopeDescription object.
    TF_API explicit
    TfScopeDescription(char const *description,
                       TfCallContext const &context = TfCallContext());

    /// Destructor.
    /// Pop the description stack in this thread.
    TF_API ~TfScopeDescription();

    /// Replace the description stack entry for this scope description.  Caller
    /// guarantees that the string \p description lives at least as long as this
    /// TfScopeDescription object.
    TF_API void SetDescription(std::string const &description);

    /// Replace the description stack entry for this scope description.  This
    /// object adopts ownership of the rvalue \p description.
    TF_API void SetDescription(std::string &&description);

    /// Replace the description stack entry for this scope description.  Caller
    /// guarantees that the string \p description lives at least as long as this
    /// TfScopeDescription object.
    TF_API void SetDescription(char const *description);

private:
    friend inline TfScopeDescription *
    Tf_GetPreviousScopeDescription(TfScopeDescription *d) {
        return d->_prev;
    }
    friend inline char const *
    Tf_GetScopeDescriptionText(TfScopeDescription *d) {
        return d->_description;
    }
    friend inline TfCallContext const &
    Tf_GetScopeDescriptionContext(TfScopeDescription *d) {
        return d->_context;
    }
    
    inline void _Push();
    inline void _Pop() const;
    
    std::optional<std::string> _ownedString;
    char const *_description;
    TfCallContext _context;
    void *_localStack;
    TfScopeDescription *_prev; // link to parent scope.
};

/// Return a copy of the current description stack for the "main" thread as
/// identified by ArchGetMainThreadId() as a vector of strings.  The most
/// recently pushed description is at back(), and the least recently pushed
/// description is at front().
TF_API std::vector<std::string>
TfGetCurrentScopeDescriptionStack();

/// Return a copy of the current description stack for the current thread of
/// execution as a vector of strings.  The most recently pushed description is
/// at back(), and the least recently pushed description is at front().
TF_API std::vector<std::string>
TfGetThisThreadScopeDescriptionStack();

/// Macro that accepts either a single string, or printf-style arguments and
/// creates a scope description local variable with the resulting string.
#define TF_DESCRIBE_SCOPE(...)                                                 \
    TfScopeDescription __scope_description__                                   \
    (Tf_DescribeScopeFormat(__VA_ARGS__), TF_CALL_CONTEXT);                    \

template <typename... Args>
inline std::string
Tf_DescribeScopeFormat(const char* fmt, Args&&... args) {
    return TfStringPrintf(fmt, std::forward<Args>(args)...);
}

// If there are no formatting arguments, the string can be forwarded to the
// scope description constructor. In C++17, consider if std::string_view could
// reduce the need for as many of these overloads
inline const char*
Tf_DescribeScopeFormat(const char* fmt) { return fmt; }

inline std::string&&
Tf_DescribeScopeFormat(std::string&& fmt) { return std::move(fmt); }

inline const std::string&
Tf_DescribeScopeFormat(const std::string& fmt) { return fmt; }

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SCOPE_DESCRIPTION_H
