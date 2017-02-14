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
#ifndef TF_REGISTRYMANAGER_H
#define TF_REGISTRYMANAGER_H

/// \file tf/registryManager.h
/// \ingroup group_tf_Initialization

#include "pxr/pxr.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/api.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfRegistryManager
/// \ingroup group_tf_Initialization
///
/// Manage initialization of registries.
///
/// See \ref page_tf_RegistryManager for a detailed description.
///
class TfRegistryManager : boost::noncopyable {
public:
    // The type of a registration function.  The arguments are not used.
    typedef void (*RegistrationFunctionType)(void*, void*);
    typedef boost::function<void ()> UnloadFunctionType;

    /// Return the singleton \c TfRegistryManager instance.
    TF_API static TfRegistryManager& GetInstance();

    /// Request that any initialization for service \c T be performed.
    ///
    /// Calling \c SubscribeTo<T>() causes all existing \c
    /// TF_REGISTRY_FUNCTION() functions of type \c T to be run.  Once
    /// this call is made, when new code is dynamically loaded then any
    /// \c TF_REGISTRY_FUNCTION() functions of type \c T in the new code
    /// will automatically be run when the code is loaded.
    template <class T>
    void SubscribeTo() {
        _SubscribeTo(typeid(T));
    }

    /// Cancel any previous subscriptions to service \c T.
    ///
    /// After this call, newly added code will no longer have \c
    /// TF_REGISTRY_FUNCTION() functions of type \c T run.
    template <class T>
    void UnsubscribeFrom() {
        _UnsubscribeFrom(typeid(T));
    }

    /// Add an action to be performed at code unload time.
    ///
    /// When a \c TF_REGISTRY_FUNCTION() is run, it often needs to register an
    /// inverse action to be taken when the code containing that function is
    /// unloaded.  For example, a plugin that adds information to a registry
    /// will typically want to remove that information when the registry is
    /// unloaded.
    ///
    /// Calling \c AddFunctionForUnload() requests that the given function be
    /// run if the code from which the funtion is called is unloaded.
    /// However, this is detectable only if this call is made from within the
    /// call chain of some \c TF_REGISTRY_FUNCTION() function.  In this case,
    /// \c AddFunctionForUnload() returns true.  Otherwise, false is returned
    /// and the function is never run.
    ///
    /// Note however that by default, no unload functions are run when code is
    /// being unloaded because exit() has been called.  This is an
    /// optimization, because most registries don't need to be deconstructed
    /// at exit time. This behavior can be changed by calling \c
    /// RunUnloadersAtExit().
    TF_API bool AddFunctionForUnload(const UnloadFunctionType&);

    /// Run unload functions program exit time.
    ///
    /// The functions added by \c AddFunctionForUnload() are normally not run
    /// when a program exits.  For debugging purposes (e.g. checking for
    /// memory leaks) it may be desirable to run the functions even at program
    /// exit time.  This call will force functions to be run at program exit
    /// time.
    ///
    /// Note that this call does not cause construction of the singleton \c
    /// TfRegistryManager object if it does not already exist.
    TF_API static void RunUnloadersAtExit();

private:
    TF_API TfRegistryManager();
    TF_API ~TfRegistryManager();

    TF_API void _SubscribeTo(const std::type_info&);
    TF_API void _UnsubscribeFrom(const std::type_info&);
};

// Private class used to indicate the library has finished registering
// functions, to indicate that the library is being unloaded and to
// add functions to the registry.
class Tf_RegistryInit {
public:
    TF_API Tf_RegistryInit(const char* name);
    TF_API ~Tf_RegistryInit();

    TF_API static void Add(const char* libName,
                    TfRegistryManager::RegistrationFunctionType func,
                    const char* typeName);
    template <class T, class U>
    static void Add(const char* libName,
                    void (*func)(T*, U*),
                    const char* typeName)
    {
        Add(libName,(TfRegistryManager::RegistrationFunctionType)func,typeName);
    }

private:
    const char* _name;
};

// The ARCH_CONSTRUCTOR priority for registering registry functions.
#define TF_REGISTRY_PRIORITY 100

// Tell registry when this library loads/unloads.
static Tf_RegistryInit tf_registry_init(BOOST_PP_STRINGIZE(MFB_ALT_PACKAGE_NAME));

//
// Macros for adding registry functions outside class templates.
//

// Define a registry function outside of a template.  Follow the macro with
// the body of the function inside braces.  KEY_TYPE and TAG must be types.
#define TF_REGISTRY_DEFINE_WITH_TYPE(KEY_TYPE, TAG) \
    static void _Tf_RegistryFunction(KEY_TYPE*, TAG*); \
    ARCH_CONSTRUCTOR(_Tf_RegistryAdd, TF_REGISTRY_PRIORITY, KEY_TYPE*, TAG*) \
    { \
        Tf_RegistryInit::Add(BOOST_PP_STRINGIZE(MFB_ALT_PACKAGE_NAME), \
                             (void(*)(KEY_TYPE*, TAG*))_Tf_RegistryFunction, \
                             BOOST_PP_STRINGIZE(KEY_TYPE)); \
    } \
    static void _Tf_RegistryFunction(KEY_TYPE*, TAG*)

// Define a registry function outside of a template.  Follow the macro with
// the body of the function inside braces.  KEY_TYPE must be a type and NAME
// must be a valid C++ name.
#define TF_REGISTRY_DEFINE(KEY_TYPE, NAME) \
    static void _Tf_RegistryFunction ## NAME(KEY_TYPE*, void*); \
    ARCH_CONSTRUCTOR(_Tf_RegistryAdd ## NAME, TF_REGISTRY_PRIORITY, KEY_TYPE*) \
    { \
        Tf_RegistryInit::Add(BOOST_PP_STRINGIZE(MFB_ALT_PACKAGE_NAME), \
                             (void(*)(KEY_TYPE*, void*))_Tf_RegistryFunction ## NAME, \
                             BOOST_PP_STRINGIZE(KEY_TYPE)); \
    } \
    static void _Tf_RegistryFunction ## NAME(KEY_TYPE*, void*)

//
// Macros for adding registry functions inside class templates.
//

// Define a registry function inline in a template.  Follow the macro with
// the body of the function inside braces.
#define TF_REGISTRY_TEMPLATE_DEFINE(KEY_TYPE, TAG) \
    ARCH_CONSTRUCTOR(_Tf_RegistryAdd, TF_REGISTRY_PRIORITY, KEY_TYPE*, TAG*) \
    { \
        Tf_RegistryInit::Add(BOOST_PP_STRINGIZE(MFB_ALT_PACKAGE_NAME), \
                             (void(*)(KEY_TYPE*, TAG*))_Tf_RegistryFunction, \
                             BOOST_PP_STRINGIZE(KEY_TYPE)); \
    } \
    static ARCH_HIDDEN void _Tf_RegistryFunction(KEY_TYPE*, TAG*)

// Declare a registry function in a template.
// Use \c TF_REGISTRY_TEMPLATE_SIGNATURE to define the function out-of-line,
// e.g. template<> void MyTemplateClass<MyType>::TF_REGISTRY_TEMPLATE_SIGNATURE(Key, Tag)
#define TF_REGISTRY_TEMPLATE_DECLARE(KEY_TYPE, TAG) \
    static ARCH_HIDDEN void _Tf_RegistryFunction(KEY_TYPE*, TAG*); \
    ARCH_CONSTRUCTOR(_Tf_RegistryAdd, TF_REGISTRY_PRIORITY, KEY_TYPE*, TAG*) \
    { \
        Tf_RegistryInit::Add(BOOST_PP_STRINGIZE(MFB_ALT_PACKAGE_NAME), \
                             (void(*)(KEY_TYPE*, TAG*))_Tf_RegistryFunction, \
                             BOOST_PP_STRINGIZE(KEY_TYPE)); \
    }

// Function name and signature for registry function in a template.
#define TF_REGISTRY_TEMPLATE_SIGNATURE(KEY_TYPE, TAG) \
    _Tf_RegistryFunction(KEY_TYPE*, TAG*)

/// Define a function that is called on demand by \c TfRegistryManager.
///
/// This is a simpler form of TF_REGISTRY_FUNCTION_WITH_TAG() that provides
/// a tag for you, based on the MFB package, file name, and line number being
/// compiled.  For most cases (private registry functions inside .cpp files)
/// this should do.
///
/// A very common use is to symbolically define enum names (see \c TfEnum):
/// \code
/// TF_REGISTRY_FUNCTION(TfEnum)
/// {
///        // Bit-depth types.
///        TF_ADD_ENUM_NAME(ELEM_BITDEPTH_8);
///        TF_ADD_ENUM_NAME(ELEM_BITDEPTH_10);
///        TF_ADD_ENUM_NAME(ELEM_BITDEPTH_32);
///
///        // Destination types.
///        TF_ADD_ENUM_NAME(ELEM_DESTINATION_DISKFARM);
///        TF_ADD_ENUM_NAME(ELEM_DESTINATION_JOBDIR);
/// 
///        // Renderer types.
///        TF_ADD_ENUM_NAME(ELEM_RENDERER_GRAIL);
///        TF_ADD_ENUM_NAME(ELEM_RENDERER_PRMAN);
/// }
/// \endcode
///
/// \hideinitializer
#define TF_REGISTRY_FUNCTION(KEY_TYPE) \
    TF_REGISTRY_FUNCTION_WITH_TAG(KEY_TYPE, __LINE__)

/// Define a function that is called on demand by \c TfRegistryManager.
///
/// Here is an example of using this macro:
/// \code
/// #include "pxr/base/tf/registryManager.h"
///
/// TF_REGISTRY_FUNCTION_WITH_TAG(XyzRegistry, MyTag)
/// {
///      // calls to, presumably, XyzRegistry:
///      ...
/// }
///\endcode
///
/// Given the above, a call to \c TfRegistryManager::SubscribeTo<XyzRegistry>()
/// will cause the above function to be immediately run.  (If the above function
/// has not yet been loaded, but is loaded in the future, it will be run then.)
/// The second type, \c MyType, is unimportant, but cannot be repeated with the
/// first type (i.e. there can be at most one call to \c TF_REGISTRY_FUNCTION()
/// for a given pair of types).
///
/// In contrast to the typical static-constructor design, the code within a
/// TF_REGISTRY_FUNCTION() function is (usually) not run before main();
/// specifically, it is not run unless and until a call to SubscribeTo<T>()
/// occurs.  This is important: if there are no subscribers, the code may never
/// be run.
///
/// Note two restrictions: the type-names \p KEY_TYPE and \c TAG passed
/// to this macro must be untemplated, and not qualified with a
/// namespace. (Translation: the name as given must consist solely of letters and
/// numbers: no "\<", "\>" or ":" characters are allowed.)  KEY_TYPE may be inside
/// a namespace but must not be explicitly qualified; you must use 'using
/// namespace &lt;foo\>::KEY_TYPE' before calling this macro.  Every use of \c
/// TF_REGISTRY_FUNCTION() must use a different pair for \c KEY_TYPE and \c
/// TAG or a multiply defined symbol will result at link time.  Note
/// that this means the same KEY_TYPE in two or more namespaces may not be
/// registered in more than one namespace.
///
/// \hideinitializer
#define TF_REGISTRY_FUNCTION_WITH_TAG(KEY_TYPE, TAG) \
    TF_REGISTRY_DEFINE(KEY_TYPE, TAG)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_REGISTRYMANAGER_H
