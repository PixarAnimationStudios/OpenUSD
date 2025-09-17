//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_STATIC_DATA_H
#define PXR_BASE_TF_STATIC_DATA_H

/// \file tf/staticData.h
/// \ingroup group_tf_Initialization

#include "pxr/pxr.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

#include <atomic>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfStaticData
/// \ingroup group_tf_Initialization
///
/// Create or return a previously created object instance of global data.
///
/// Any form of global data that requires an constructor (even a default
/// constructor) is unsafe to declare as global data.  By global data we mean
/// either a variable defined at file-scope (outside of a function) or a
/// static member of a class.  This is because the initialization order of
/// globals is undefined across translation units.
/// 
/// The only exceptions are constexpr constructors and "plain old data" types
/// such as integral or float/double type and pointers.  In contrast, \c
/// std::string requires construction, as do most \c STL types, and most
/// user-defined types as well.  Note that static local variables in functions
/// are also safe and are initialized in a thread-safe manner the first time
/// they're encountered.
///
/// One way to handle this problem is to go the singleton route, which can be
/// done using the \c TfSingleton pattern.  However, a fair amount of coding is
/// required for this, and at times, something more lightweight is appropriate.
/// For these few cases, the following construct may be employed:
///
/// \code
/// // source file:
///
/// #include <set>
/// #include <string>
///
/// static TfStaticData<set<string> > Xyz_nameSet;
///
/// void XyzAddName(string name) {
///     Xyz_nameSet->insert(name);
///
///     ...
/// }
/// \endcode
///
/// One uses a \c TfStaticData<T> as if it were a pointer; upon first use
/// however, the item is initialized to point at a new object of type \c T.  Note
/// that the type \c T must have a default constructor; that is, the newly
/// created object is created by calling \c "new T".
///
/// If you have no need to access the data, but need to make sure it has been
/// initialized (for example, if the type's constructor will have some effect
/// that you need to be sure has happened), you can call the Touch() method.
///
/// Warning: the \c TfStaticData construct relies upon zero-initialization of
/// global data: therefore, you can only use this structure for static data
/// member of classes or variables declare at file-scope.  Do \e not declare
/// a \c TfStaticData object as a local variable, as a member of a class or
/// structure, or as a function parameter.  Use normal static local variable
/// initialization inside a function.
///
/// One can either call member functions using the "->" operator, or use the
/// dereference "*" operator:
/// 
/// \code
/// TfStaticData<string> Xyz_curName;
///
/// void Xyz_SetLastName(string s) {
///     *Xyz_curName = s;
///
///     vector<string> v;
///     v.push_back(*Xyz_curName);
/// }
/// \endcode
///
template <class T>
struct Tf_StaticDataDefaultFactory {
    static T *New() { return new T; }
};

template <class T, class Factory = Tf_StaticDataDefaultFactory<T> >
class TfStaticData {
public:
    /// Return a pointer to the underlying data object. It is created and
    /// initialized if necessary.
    inline T* operator-> () const { return Get(); }

    /// Member lookup. The underlying data object is created and initialized
    /// if necessary.
    inline T& operator* () const { return *Get(); }

    /// Return a pointer to the underlying object, creating and initializing
    /// it if necessary.
    inline T* Get() const {
        T *p = _data;
        return ARCH_LIKELY(p) ? p : _TryToCreateData();
    }
    
    /// Return true if the underlying data object is created and initialized.
    /// Return false otherwise.
    inline bool IsInitialized() const { return _data.load() != nullptr; }

private:
    T *_TryToCreateData() const {
        // Allocate an instance.
        T *tmp = Factory::New();

        // Try to atomically set the pointer from null to tmp.
        T *n = nullptr;
        if (ARCH_LIKELY(_data.compare_exchange_strong(n, tmp)))
            return tmp;

        // Another thread won the initialization race.
        delete tmp;
        return _data;
    }

    mutable std::atomic<T *> _data;
};

/// Create a static data object, initializing it with code.
/// 
/// The macro takes two arguments. The first is the type of static data, the
/// second is the name of the variable. The block of code following the macro
/// will be invoked to initialize the static data when it is first used. See
/// example usage:
///
/// \code
/// TF_MAKE_STATIC_DATA(string, myString) { *myString = "hello!"; }
/// 
/// TF_MAKE_STATIC_DATA(vector<string>, someNames) {
///     someNames->push_back("hello");
///     someNames->push_back("static");
///     someNames->push_back("world");
/// }
///
/// TF_MAKE_STATIC_DATA((map<int, int>), intMap) {
///     (*intMap)[1] = 11;
///     (*intMap)[2] = 22;
/// }
/// \endcode
///
/// If the type uses commas to separate template arguments you need to enclose
/// the type in parentheses as shown in the last example.
///
/// If the data does not need to be mutated after initialization, you may
/// specify a const type. The underlying data is non-const but the
/// TfStaticData accessors only provide const access.
///
/// \code
/// TF_MAKE_STATIC_DATA(const vector<string>, constNames) {
///     constNames->push_back("hello");
///     constNames->push_back("static const");
///     constNames->push_back("world");
/// }
/// \endcode
///
/// Note that this macro may only be used at namespace scope (not function
/// scope).
///
/// Also note that in multithreaded code, it is possible for the provided code
/// to be invoked more than once (with different target objects) for the same
/// static data instance.  This is fine as long as the initialization code does
/// not have side-effects, but you should be aware of it.
///
/// \hideinitializer
#define TF_MAKE_STATIC_DATA(Type, Name)                                        \
    static void TF_PP_CAT(Name,_Tf_StaticDataFactoryImpl)(                     \
        std::remove_const_t<TF_PP_EAT_PARENS(Type)> *);                        \
    namespace {                                                                \
    struct TF_PP_CAT(Name,_Tf_StaticDataFactory) {                             \
        static TF_PP_EAT_PARENS(Type) *New() {                                 \
            auto *p = new std::remove_const_t<TF_PP_EAT_PARENS(Type)>;         \
            TF_PP_CAT(Name,_Tf_StaticDataFactoryImpl)(p);                      \
            return p;                                                          \
        }                                                                      \
    };                                                                         \
    }                                                                          \
    static TfStaticData<                                                       \
        TF_PP_EAT_PARENS(Type), TF_PP_CAT(Name,_Tf_StaticDataFactory)> Name;   \
    static void TF_PP_CAT(Name,_Tf_StaticDataFactoryImpl)(                     \
        std::remove_const_t<TF_PP_EAT_PARENS(Type)> *Name)

PXR_NAMESPACE_CLOSE_SCOPE

#endif
