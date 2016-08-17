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
#ifndef TF_DEBUG_H
#define TF_DEBUG_H

/// \file tf/debug.h
/// \ingroup group_tf_DebuggingOutput
/// Conditional debugging output class and macros.

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/arch/attributes.h"

#include <cstdio>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

class Tf_DebugSymbolRegistry;

/// \addtogroup group_tf_DebuggingOutput
///@{

/// \class TfDebug
///
/// Enum-based debugging messages.
///
/// The \c TfDebug class encapsulates a simple enum-based conditional
/// debugging message system.  It is meant as a tool for developers, and
/// \e NOT as a means of issuing diagnostic messages to end-users. (This is
/// not strictly true. The TfDebug class is extremely useful and has many
/// properties that make its use attractive for issuing messages to end-users.
/// However, for this purpose, please use the \c TF_INFO macro which more
/// clearly indicates its intent.)
///
/// The features of \c TfDebug are:
///   \li Debugging messages/calls for an entire enum group can be
///       compiled out-of-existence.
///   \li The cost of checking if a specific message should be printed
///       at runtime (assuming the enum group of the message has not been
///       compile-time disabled) is a single inline array lookup,
///       with a compile-time  index into a global array.
///   \li Parent/child relationships can be defined so that groups of messages
///       can be hierarchically enabled or disabled.
///
/// The use of the facility is simple:
/// \code
///   // header file
///   #include "pxr/base/tf/debug.h"
///   TF_DEBUG_CODES(MY_E1, MY_E2, MY_E3);
///
///   // source file
///   TF_DEBUG(MY_E2).Msg("something about e2\n");
///
///   TF_DEBUG(MY_E3).Msg("val = %d\n", value);
/// \endcode
///
/// The code in the header file declares the debug symbols to use.  Under
/// the hood, this creates an enum with the values given in the argument to
/// TF_DEBUG_CODES, along with a first and last sentinel values and passes
/// that to TF_DEBUG_RANGE. If you'd like to be more explicit (e.g., because 
/// you need the enum type name, or need to be able to turn off the facility 
/// at compile time), you could use the following, equivalent, form:
///
/// \code
///   // header file
///   #include "pxr/base/tf/debug.h"
///   enum MyDebugCodes { MY_FIRST, MY_E1, MY_E2, MY_E3, MY_LAST };
///   TF_DEBUG_RANGE(MyDebugCodes, MY_FIRST, MY_LAST, true);
///
///   // source file
///   TF_DEBUG(MY_E2).Msg("something about e2\n");
///
///   TF_DEBUG(MY_E3).Msg("val = %d\n", value);
/// \endcode
///
/// In the source file, the indicated debugging messages are printed
/// only if the debugging symbols are enabled.  Effectively, the construct
/// \code
///     TF_DEBUG(MY_E1).Msg(msgExpr)
/// \endcode
/// is translated to
/// \code
///     if (symbol-MY_E1-is-enabled)
///         output(msgExpr)
/// \endcode
///
/// The implications are that \c msgExpr is only evaluated if symbol \c MY_E1
/// symbol is enabled.  Further, if the last argument (which must be
/// a compile-time constant) to the \c TF_DEBUG_RANGE() macro is \c false,
/// then the test is known to fail at compile time; in this case, the
/// compiler will even eliminate outputting the code to execute \c msgExpr.
/// This scheme allows the costs of debugging code to be controlled at a
/// fine level of detail.
///
/// Most commonly debug symbols are inactive by default, but can be turned
/// on either by an environment variable \c TF_DEBUG, or interactively once
/// a program has started by a script interpreter.  Both of these are
/// accomplished as follows:
/// \code
///   // source file xyz/debugCodes.cpp
///
///   #include "proj/my/debugCodes.h"
///   #include "pxr/base/tf/debug.h"
///   #include "pxr/base/tf/registryManager.h"
///
///   TF_REGISTRY_FUNCTION(TfDebug, MyDebugCodes) {
///       TF_DEBUG_ENVIRONMENT_SYMBOL(MY_E1, "loading of blah-blah files");
///       TF_DEBUG_ENVIRONMENT_SYMBOL(MY_E2, "parsing of mdl code");
///       // etc.
///   }
/// \endcode
///
/// Once this is done, symbols are enabled as follows:
/// \code
///     TfDebug::DisableAll<MyDebugCodes>();     // disable everything
///
///     TfDebug::Enable(MY_E1);                  // enable just MY_E1
/// \endcode
///
class TfDebug {
public:
    /// Mark debugging as enabled for enum value \c val, and any descendents
    /// of \c val as defined by \c DefineParentChild().
    ///
    /// The default state for all debugging symbols is disabled. Note that the
    /// template parameter is deduced from \c val:
    /// \code
    ///     TfDebug::Enable(MY_E3);
    /// \endcode
    template <class T>
    static void Enable(T val) {
        _SetNodes(&_GetNode(val), 1, true);
    }

    /// Mark debugging as disabled for enum value \c val, and any descendents
    /// of \c val as defined by \c DefineParentChild().
    template <class T>
    static void Disable(T val) {
        _SetNodes(&_GetNode(val), 1, false);
    }

    /// Mark debugging as enabled for all enum values of type \c T.
    ///
    /// Note that the template parameter must be explicitly supplied:
    /// \code
    ///     TfDebug::EnableAll<MyDebugCodes>()
    /// \endcode
    template <class T>
    static void EnableAll() {
        _SetNodes(&_Data<T>::nodes[0], _Traits<T>::n, true);
    }

    /// Mark debugging as disabled for all enum values of type \c T.
    template <class T>
    static void DisableAll() {
        _SetNodes(&_Data<T>::nodes[0], _Traits<T>::n, false);
    }

    /// Define a parent/child relationship.
    ///
    /// Enum value \c child is marked as a child of \c parent; this means that
    /// enabling or disabling \c parent enable or disables not only parent,
    /// but, recursively, all descendents of \c parent as well.
    ///
    /// To avoid cycles, \c child cannot have been made a parent at the time
    /// of this call.
    template <class T1, class T2>
    static void DefineParentChild(T1 parent, T2 child) {
        _SetParentChild(&_GetNode(parent), &_GetNode(child));
    }

    /// True if debugging is enabled for the enum value \c val.
    ///
    /// Note that not only must the specific enum value \c val be marked as
    /// enabled, but the enum type \c T must be globally enabled; this is
    /// controlled by the last argument to the \c TF_DEBUG_RANGE() must have
    /// been \c true.
    template <class T>
    static bool IsEnabled(T val) {
        return _Traits<T>::compileTimeEnabled && _GetNode(val).enabled;
    }

    /// True if debugging can be activated at run-time, whether or not it is
    /// currently enabled.
    template <class T>
    static bool IsCompileTimeEnabled() {
        return _Traits<T>::compileTimeEnabled;
    }
    
    /// Return the number of debugging symbols of this type.
    ///
    /// Returns the number of different enums in the range specified by \c
    /// TF_DEBUG_RANGE().
    template <class T>
    static size_t GetDebugRangeCount() {
        return _Traits<T>::n;
    }

    /// Return the index-th debug symbol of this type.
    ///
    /// If \p index-th is out of range (i.e. greater than or equal to
    /// \c GetDebugRangeCount()) the last symbol is returned.
    ///
    /// The above two functions can be used to print out the names of all
    /// debug symbols, if they have been registered via \c TF_ADD_ENUM_NAME():
    /// \code
    /// for (size_t i = 0; i < TfDebug::GetDebugRangeCount<Codes>(); i++)
    ///    cout << TfEnum::GetName(TfDebug::GetDebugSymbol<T>(i)) << "\n";
    /// \endcode
    template <class T>
    static T GetDebugSymbol(size_t index) {
        if (index >= _Traits<T>::n)
            index = _Traits<T>::n-1;
        
        return T(_Traits<T>::min + index);
    }

#if !defined(doxygen)
    struct Helper {
        static TF_API void Msg(const std::string& msg);
        static TF_API void Msg(const char* msg, ...) ARCH_PRINTF_FUNCTION(1,2);
    };
#endif

    template <bool B>
    struct ScopeHelper {
        ScopeHelper(bool enabled, const char* name) {
            if ((active = enabled)) {
                str = name;
                TfDebug::_ScopedOutput(true, str);
            }
            else
                str = NULL;
        }

        ~ScopeHelper() {
            if (active)
                TfDebug::_ScopedOutput(false, str);
        }

        bool active;
        const char* str;
    };

    template <bool B>
    struct TimedScopeHelper {
        TimedScopeHelper(bool enabled, const char* fmt, ...) 
            ARCH_PRINTF_FUNCTION(3, 4);
        ~TimedScopeHelper();

        bool active;
        std::string str;
        TfStopwatch stopwatch;
    };

    /// Set registered debug symbols matching \p pattern to \p value.
    ///
    /// All registered debug symbols matching \p pattern are set to \p value.
    /// The only matching is an exact match with \p pattern, or if \p pattern
    /// ends with an '*' as is otherwise a prefix of a debug symbols.  The
    /// names of all debug symbols set by this call are returned as a vector.
    TF_API
    static std::vector<std::string> SetDebugSymbolsByName(
        const std::string& pattern, bool value);

    /// True if the specified debug symbol is set.
    TF_API
    static bool IsDebugSymbolNameEnabled(const std::string& name);

    /// Get a description of all debug symbols and their purpose.
    ///
    /// A single string describing all registered debug symbols along with
    /// short descriptions is returned.
    TF_API
    static std::string GetDebugSymbolDescriptions();

    /// Get a listing of all debug symbols.
    TF_API
    static std::vector<std::string> GetDebugSymbolNames();

    /// Get a description for the specified debug symbol.
    ///
    /// A short description of the debug symbol is returned. This is the same
    /// description string that is embedded in the return value of
    /// GetDebugSymbolDescriptions.
    TF_API
    static std::string GetDebugSymbolDescription(const std::string& name);

    /// Direct debug output to \a either stdout or stderr.
    ///
    /// Note that \a file MUST be either stdout or stderr.  If not, issue an
    /// error and do nothing.  Debug output is issued to stdout by default.
    /// If the environment variable TF_DEBUG_OUTPUT_FILE is set to 'stderr',
    /// then output is issued to stderr by default.
    TF_API
    static void SetOutputFile(FILE *file);
    
    struct _Node;

    // Public, to be used in TF_DEBUG_ENVIRONMENT_SYMBOL() macro,
    // but not meant to be used otherwise.
    template <class T>
    static _Node* _GetSymbolAddr(T val, const char* name) {
        _Traits<T>::Enum_Not_Listed_In_Any_TF_DEBUG_RANGE();
        int index = int(val) - _Traits<T>::min;
        if (index < 0 || index >= int(_Traits<T>::n))
            _ComplainAboutInvalidSymbol(name);

        return &_GetNode(val);
    }

    // Public, to be used in TF_DEBUG_ENVIRONMENT_SYMBOL() macro,
    // but not meant to be used otherwise.
    TF_API
    static void _RegisterDebugSymbol(TfEnum val, _Node* addr,
                                     const char* descrip);

    // This function is only meant for use by TfRegistryManager.
    // It's relatively expensive, and not meant for large-scale use.
    TF_API
    static bool _CheckEnvironmentForMatch(const std::string& enumName);

    // Unfortunately, we need to make both _Traits and _Node, below
    // public because of their use in macros.
    // Please treat both as a private data structures!

    template <class T>
    struct _Traits {
        static const int min = 0;
        static const size_t n = 1;
        enum{compileTimeEnabled = 0};
    };
    
    // Note: this is a POD (plain old data structure) so it is initialized
    // to zero statically.
    struct _Node {
        std::vector<_Node*>* children;
        bool enabled;
        bool hasParent;
        ~_Node() {
            delete children;
        }
    };

private:
    
    template <class T>
    struct _Data {
        static _Node nodes[_Traits<T>::n];
    };

    template <class T>
    static _Node& _GetNode(T val) {
        return _Data<T>::nodes[int(val) - _Traits<T>::min];
    }

    friend class Tf_DebugSymbolRegistry;
    
    TF_API
    static void _ComplainAboutInvalidSymbol(const char*);
    static void _SetNodes(_Node* ptr, size_t nNodes, bool state);
    static void _SetParentChild(_Node* parent, _Node* child);
    static void _ScopedOutput(bool start, const char* str);
};

template <class T>
TfDebug::_Node TfDebug::_Data<T>::nodes[];

template <>
struct TfDebug::TimedScopeHelper<false> {
    TimedScopeHelper(bool, const char*, ...) 
        ARCH_PRINTF_FUNCTION(3, 4) { 
    }
};

/// Define debugging symbols.
///
/// This is a simple macro that takes care of declaring your enum, providing a
/// first and last symbol and declaring the range. Use it as follows:
/// \code
/// TF_DEBUG_CODES(
///   MY_E1,
///   MY_E2
/// );
/// \endcode
///
/// \hideinitializer
#define TF_DEBUG_CODES(...)                                                  \
    enum _TF_DEBUG_ENUM_NAME(__VA_ARGS__) {                                  \
        TF_PP_CAT( _TF_DEBUG_ENUM_NAME(__VA_ARGS__), __FIRST)                \
        , __VA_ARGS__                                                        \
        , TF_PP_CAT( _TF_DEBUG_ENUM_NAME(__VA_ARGS__), __LAST)               \
    };                                                                       \
    TF_DEBUG_RANGE(                                                          \
        _TF_DEBUG_ENUM_NAME(__VA_ARGS__),                                    \
        TF_PP_CAT( _TF_DEBUG_ENUM_NAME(__VA_ARGS__), __FIRST),               \
        TF_PP_CAT( _TF_DEBUG_ENUM_NAME(__VA_ARGS__), __LAST),                \
        true);

// In the _TF_DEBUG_ENUM_NAME macro below we pass 'dummy' to
// _TF_DEBUG_FIRST_CODE as the second argument to ensure that we always
// have more than one argument as expected by _TF_DEBUG_FIRST_CODE.
#define _TF_DEBUG_ENUM_NAME(...)                                             \
    TF_PP_CAT( _TF_DEBUG_FIRST_CODE(__VA_ARGS__, dummy), __DebugCodes )

#define _TF_DEBUG_FIRST_CODE(first, ...)                                     \
    first

/// Define the range for an enum class for debugging symbols.
///
/// The parameters \c first and \c last should be constant values of
/// enumerated type \c enumType; furthermore, \c first should be numerically
/// less than (or equal to) \c last when compared as integers.  The fourth
/// parameter \c enabled should be a constant boolean value; if \c false, then
/// all debugging symbols for enumerated type \c enumType are treated as
/// disabled, regardless of any calls to \c TfDebug::Enable() or
/// \c TfDebug::EnableAll().  Furthermore, this mode of disabling is
/// discernable at compile time, rendering the \c TF_DEBUG() macro a literal
/// no-op in terms of code-generation for this enumeration class.
///
/// \hideinitializer
#define TF_DEBUG_RANGE(enumType, first, last, enabled)                  \
    template <>                                                         \
    struct TfDebug::_Traits<enumType> {                                 \
        static const int  min = first;                                  \
        static const size_t n = 1 + int(last) - int(first);             \
        enum {compileTimeEnabled = enabled};                            \
        static void Enum_Not_Listed_In_Any_TF_DEBUG_RANGE() { }         \
    };

/// Evaluate and print debugging message \c msg if \c enumVal is enabled for
/// debugging.
///
/// This macro is a newer, more convenient form of the \c TF_DEBUG() macro.
/// Writing
/// \code
///      TF_DEBUG_MSG(enumVal, msg, ...);
/// \endcode
/// is equivalent to
/// \code
///     TF_DEBUG(enumVal).Msg(msg, ...);
/// \endcode
///
/// The TF_DEBUG_MSG() macro allows either an std::string argument or
/// a printf-like format string followed by a variable number of arguments:
/// \code
///     TF_DEBUG_MSG(enumVal, "opening file %s\n", file.c_str());
///
///     TF_DEBUG_MSG(enumVal, "opening file " + file);
/// \endcode
///
/// \hideinitializer
#define TF_DEBUG_MSG(enumVal, ...)                                                \
    if (!TfDebug::IsEnabled(enumVal)) /* empty */ ; else TfDebug::Helper().Msg(__VA_ARGS__)

/// Evaluate and print debugging message \c msg if \c enumVal is enabled for
/// debugging.
///
/// The \c TF_DEBUG() macro is used as follows:
/// \code
///     TF_DEBUG(enumVal).Msg("opening file %s, count = %d\n",
///                           file.c_str(), count);
/// \endcode
///
/// If \c enumVal is of enumerated type \c enumType, and \c enumType
/// has been enabled for debugging (see \c TF_DEBUG_RANGE()), and
/// the specific value \c enumVal has been enabled for debugging by a call
/// to \c TfDebug::Enable(), then the arguments in the \c Msg() call are
/// evaluated and printed.  The argument to \c Msg() may either be a
/// \c const \c char* and a variable number of arguments, using standard
/// printf-formatting rules, or a \c std::string variable:
/// \code
///     TF_DEBUG(enumVal).Msg("opening file " + file + "\n");
/// \endcode
///
/// Note that the arguments to \c Msg() are unevaluated when the value
/// \c enumVal is not enabled for debugging, so \c Msg() must be free
/// of side-effects; however, when \c enumVal is not enabled, there is
/// no expense incurred in computing the arguments to \c Msg().  Note
/// that if the entire enum type corresponding to \c enumVal is
/// disabled (a compile-time determination) then the code for the \e
/// entire \c TF_DEBUG().Msg() statement will typically not even be
/// generated!
///
/// \sa TF_DEBUG_MSG()
///
/// \hideinitializer
#define TF_DEBUG(enumVal)                                               \
    if (!TfDebug::IsEnabled(enumVal)) /* empty */ ; else TfDebug::Helper()

/// Evaluate and print diagnostic messages intended for end-users.
///
/// The TF_INFO(x) macro is cosmetic; it actually just calls the TF_DEBUG
/// macro (see above).  This macro should be used if its output is intended to
/// be seen by end-users.
///
/// \hideinitializer
#define TF_INFO(x) TF_DEBUG(x)

/// Print description and time spent in scope upon beginning and exiting it if
/// \p enumVal is enabled for debugging.
///
/// The \c TF_DEBUG_TIMED_SCOPE() macro is used as follows:
/// \code
/// void Attribute::Compute()
/// {
///     TF_DEBUG_TIMED_SCOPE(ATTR_COMPUTE, "Computing %s", name.c_str());
///     ...
/// }
/// \endcode
///
/// When the \c TF_DEBUG_TIMED_SCOPE macro is invoked, a timer is started and
/// the supplied description is printed. When the enclosing scope is exited
/// (in the example, when Attribute::Compute() finishes) the timer is stopped
/// and the scope description and measured time are printed. This allows for
/// very fine-grained timing of operations.
///
/// Note that if the entire enum type corresponding to \p enumVal is disabled
/// (a compile-time determination) then the presence of a
/// \c TF_DEBUG_TIMED_SCOPE() macro should not produce any extra generated
/// code (in an optimized build).  If the enum type is enabled, but the
/// particular value \p enumVal is disabled, the cost of the macro should be
/// quite minimal; still, it would be best not to embed the macro in functions
/// that are called in very tight loops, in final released code.
///
/// \hideinitializer
#define TF_DEBUG_TIMED_SCOPE(enumVal, ...)                              \
    TfDebug::TimedScopeHelper<                                          \
        TfDebug::_Traits<                                               \
            std::decay<decltype(enumVal)>::type>::compileTimeEnabled>   \
    TF_PP_CAT(local__TfScopeDebugSwObject, __LINE__)(                   \
        TfDebug::IsEnabled(enumVal), __VA_ARGS__)

/// Register an enum symbol for debugging.
///
/// This call should be used in source files, not header files, and should
/// This macro should usually appear within a 
/// \c TF_REGISTRY_FUNCTION(TfDebug,...) call.  The first argument should be
/// the literal name of the enum symbol, while the second argument should be a
/// (short) description of what debugging will be enabled if the symbol is
/// activated.  The enum being registered must be one which is contained in
/// some TF_DEBUG_RANGE() call. For example:
/// \code
///   TF_REGISTRY_FUNCTION(TfDebug, MyDebugCodes) {
///       TF_DEBUG_ENVIRONMENT_SYMBOL(MY_E1, "loading of blah-blah files");
///       TF_DEBUG_ENVIRONMENT_SYMBOL(MY_E2, "parsing of mdl code");
///       // etc.
///   }
/// \endcode
///
/// \hideinitializer
#define TF_DEBUG_ENVIRONMENT_SYMBOL(VAL, descrip)                         \
    if (TfDebug::_Traits<                                                 \
        std::decay<decltype(VAL)>::type>::compileTimeEnabled) {           \
        TF_ADD_ENUM_NAME(VAL);                                            \
        TfDebug::_RegisterDebugSymbol(TfEnum(VAL),                        \
                                      TfDebug::_GetSymbolAddr(VAL, #VAL), \
                                      descrip);                           \
    }

///@}

#endif
