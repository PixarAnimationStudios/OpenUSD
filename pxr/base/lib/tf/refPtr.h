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
#ifndef TF_REFPTR_H
#define TF_REFPTR_H

/// \file tf/refPtr.h
/// \ingroup group_tf_Memory
/// Reference counting.
///
/// \anchor refPtr_QuickStart
/// <B> Quick Start </B>
///
/// Here is how to make a class \c Bunny usable through \c TfRefPtr.
///
/// \code
///     #include "pxr/base/tf/refPtr.h"
///     typedef TfRefPtr<Bunny> BunnyRefPtr;
///
///     class Bunny : public TfRefBase {
///     public:
///         static BunnyRefPtr New() {
///            // warning: return new Bunny directly will leak memory!
///            return TfCreateRefPtr(new Bunny);
///         }
///         static BunnyRefPtr New(bool isRabid) {
///            return TfCreateRefPtr(new Bunny(isRabid));
///         }
///
///         ~Bunny();
///
///         bool IsHungry();
///      private:
///         Bunny();
///         Bunny(bool);
///     };
///
///     BunnyRefPtr nice = Bunny::New();
///     BunnyRefPtr mean = Bunny::New(true);    // this one is rabid
///
///     BunnyRefPtr mean2 = mean;               // two references to mean rabbit
///     mean.Reset();                        // one reference to mean rabbit
///
///     if (mean2->IsHungry())
///          nice.Reset();                   // nice bunny gone now...
///
///                                          // this function comes from
///                                          // TfRefBase; meaning is that
///     if (mean2->IsUnique())               // mean2 is the last pointer to
///         mean2.Reset();                   // this bunny...
/// \endcode
///
/// Pretty much any pointer operation that is legal with regular pointers
/// is legal with the \c BunnyRefPtr; continue reading for a more detailed
/// description.
///
/// Note that by virtue of deriving from \c TfRefBase, the reference
/// count can be queried: see \c TfRefBase for details.
///
/// \anchor refPtr_DetailedDiscussion
/// <B> Detailed Discussion: Overview </B>
///
/// Objects created by the \c new operator can easily be a source of
/// both memory leaks and dangling pointers.  One solution is
/// \e reference counting; when an object is created by \c new,
/// the number of pointers given the object's address is continually
/// tracked in a \e reference \e counter.  Then, \c delete is called on
/// the object \e only when the object's reference count drops to zero,
/// meaning there are no more pointers left pointing to the object.
/// Reference counting avoids both dangling pointers and memory leaks.
///
/// Access by regular pointers does not perform reference counting, but
/// the \c TfRefPtr<T> class implements reference-counted pointer access
/// to objects of type \c T, with minimal overhead.  The reference counting
/// is made thread-safe by use of atomic integers.
///
///
/// <B> Basic Use </B>
///
/// The use of a \c TfRefPtr is simple.  Whenever a \c TfRefPtr is
/// made to point at an object, either by initialization or assignment,
/// the object being pointed at has its reference count incremented.
/// When a \c TfRefPtr with a non-NULL address is reassigned, or
/// goes out of scope, the object being pointed to has its reference
/// count decremented.
///
/// A \c TfRefPtr<T> can access \c T's public members by the
/// \c -> operator; however, the dereference operator "\c *" is not defined.
/// Here is a simple example:
/// \code
///    #include "pxr/base/tf/refPtr.h"
///
///    typedef TfRefPtr<Simple> SimpleRefPtr;
///
///    class Simple : public TfRefBase {
///    public:
///        void Method1();
///        int  Method2();
///
///        static SimpleRefPtr New() {
///            return TfCreateRefPtr(new Simple);
///        }
///      private:
///        Simple();
///    };
///
///
///    SimpleRefPtr p1;                         // p1 points to NULL
///    SimpleRefPtr p2 = Simple::New();         // p2 points to new object A
///    SimpleRefPtr p3 = Simple::New();         // p3 points to new object B
///
///    p1->Method1();                        // runtime error -- p1 is NULL
///    p3 = p2;                              // object B is deleted
///    p2->Method1();                        // Method1 on object A
///    int value = p3->Method2();            // Method2 on object A
///
///    p2.Reset();                           // only p3 still points at A
///    p3 = p1;                              // object A is deleted
///
///    if (...) {
///        SimpleRefPtr p4 = Simple::New();     // p4 points to object C
///        p4->Method1();
///    }                                     // object C destroyed
/// \endcode
///
/// Note that \c Simple's constructor is private; this ensures that one
/// can only get at a \c Simple through \c Simple::New(), which is careful
/// to return a \c SimpleRefPtr.
///
/// Note that it is often useful to have both const and non-const
/// versions of \c TfRefPtr for the same data type. Our convention
/// is to use a \c typedef for each of these, with the const version
/// beginning with "Const", after any prefix. The const version can be
/// used as a parameter to a function in which you want to prevent
/// changes to the pointed-to object. For example:
/// \code
///    typedef TfRefPtr<XySimple>       XySimpleRefPtr;
///    typedef TfRefPtr<const XySimple> XyConstSimpleRefPtr;
///
///    void
///    Func1(const XySimpleRefPtr &p)
///    {
///        p->Modify();  // OK even if Modify() is not a const member
///    }
///
///    void
///    Func2(const XyConstSimpleRefPtr &p)
///    {
///        p->Query();   // OK only if Query() is a const member
///    }
/// \endcode
///
/// It is always possible to assign a non-const pointer to a const
/// pointer variable. In extremely rare cases where you want to do the
/// opposite, you can use the \c TfConst_cast function, as in:
/// \code
///    XyConstSimpleRefPtr p1;
///    XySimpleRefPtr      p2;
///
///    p1 = p2;                            // OK
///    p2 = p1;                            // Illegal!
///    p2 = TfConst_cast<XySimpleRefPtr>(p1); // OK, but discouraged
/// \endcode
///
/// <B> Comparisons and Tests </B>
///
/// Reference-counted pointers of like type can be compared; any \c TfRefPtr
/// can be tested to see it is NULL or not:
///
/// \code
///     TfRefPtr<Elf>   elf = Elf::New();
///     TfRefPtr<Dwarf> sleepy,
///                     sneezy = Dwarf::New();
///
///     if (!sleepy)
///         ...                          // true: sleepy is NULL
///
///     if (sneezy)
///         ...                          // true: sneezy is non-nULL
///
///     bool b1 = (sleepy != sneezy),
///          b2 = (sleepy == sneezy),
///          b3 = (elf == sneezy);       // compilation error -- type clash
///
/// \endcode
///
/// <B> Opaqueness </B>
///
/// A \c TfRefPtr can be used as an opaque pointer, just as a regular
/// pointer can.  For example, without having included the header file
/// for a class \c XySimple, the following will still compile:
/// \code
///     #include "pxr/base/tf/refPtr.h"
///
///     class XySimple;
///
///     class Complicated {
///      public:
///         void SetSimple(const TfRefPtr<XySimple>& s) {
///             _simplePtr = s;
///         }
///
///         TfRefPtr<XySimple> GetSimple() {
///             return _simplePtr;
///         }
///
///         void Forget() {
///              _simplePtr.Reset();
///         }
///
///      private:
///         TfRefPtr<XySimple> _simplePtr;
///     };
/// \endcode
///
/// Note that the call \c Forget() (or \c SetSimple() for that matter)
/// may implicitly cause destruction of an \c XySimple object, if the count
/// of the object pointed to by \c _simplePtr drops to zero.  Even though
/// the definition of \c XySimple is unknown, this compiles and works correctly.
///
/// The only time that the definition of \c XySimple is required is when
/// putting a raw \c XySimple* into a \c TfRefPtr<XySimple>; note however, that
/// this should in fact only be done within the class definition of
/// \c XySimple itself.
///
/// Other cases that require a definition of \c XySimple are parallel to
/// regular raw pointers, such as calling a member function, static or
/// dynamic casts (but not const casts) and using the \c TfTypeid
/// function.  Transferring a \c TfWeakPtr to a \c TfRefPtr also
/// requires knowing the definition of \c XySimple.
///
/// Sometimes a class may have many typedefs associated with it, having
/// to do with \c TfRefPtr or \c TfWeakPtr.  If it is useful to use
/// the class opaquely, we recommend creating a separate file
/// as follows:
///
/// \code
/// // file: proj/xy/simplePtrDefs.h
/// #include "pxr/base/tf/refPtr.h"
/// #include "pxr/base/tf/weakPtr.h"
///
/// typedef TfRefPtr<class XySimple>       XySimpleRefPtr;
/// typedef TfRefPtr<const class XySimple> XyConstSimpleRefPtr;
///
/// // typedefs for TfWeakPtr<XySimple> would follow,
/// // if XySimple derives from TfWeakBase
/// \endcode
///
/// The definition for \c XySimple would then use the typedefs:
///
/// \code
/// #include "Proj/Xy/SimpleRefPtrDefs.h"
///
/// class XySimple : public TfRefBase {
/// public:
///     static XySimpleRefPtr New();
///     ...
/// };
///
/// \endcode
///
/// The above pattern now allows a consumer of class \c XySimple the option
/// to include only \c simplePtrDefs.h, if using the type opaquely suffices,
/// or to include \c simple.h, if the class definition is required.
///
///
/// <B> Cyclic Dependencies </B>
///
/// If you build a tree using \c TfRefPtr, and you only have pointers
/// from parent to child, everything is fine: if you "lose" the root of the
/// tree, the tree will correctly destroy itself.
///
/// But what if childen point back to parents?  Then a simple parent/child
/// pair is stable, because the parent and child point at each other, and
/// even if nobody else has a pointer to the parent, the reference count
/// of the two nodes remains at one.
///
/// The solution to this is to make one of the links (typically from child back
/// to parent) use a \c TfWeakPtr.  If a class \c T is enabled for both
/// "guarding" (see \c TfWeakBase) and reference counting, then a \c TfRefPtr
/// can be converted to a \c TfWeakPtr (in this context, a "back pointer")
/// and vice versa.
///
/// <B> Inheritance </B>
///
///
/// Reference-counted pointers obey the same rules with respect to inheritance
/// as regular pointers.  In particular, if class \c Derived is derived
/// from class \c Base, then the following are legal:
///
/// \code
///     TfRefPtr<Base>    bPtr = new Base;
///     TfRefPtr<Derived> dPtr = new Derived;
///
///     TfRefPtr<Base> b2Ptr = dPtr;      // initialization
///     b2Ptr = dPtr;                        // assignment
///     b2Ptr == dPtr;                       // comparison
///
///     dPtr = bPtr;                         // Not legal: compilation error
/// \endcode
///
/// As the last example shows, initialization or assignment to
/// a \c TfRefPtr<Derived> from a \c TfRefPtr<Base> is illegal,
/// just as it is illegal to assign a \c Base* to a \c Derived*.
/// However, if \c Derived and \c Base are polymorphic
/// (i.e. have virtual functions) then the analogue of a \c dynamic_cast<>
/// is possible:
///
/// \code
///        dPtr = TfDynamic_cast<TfRefPtr<Derived> >(bPtr);
/// \endcode
///
/// Just like a regular \c dynamic_cast<> operation, the \c TfRefPtr
/// returned by \c TfDynamic_cast<> points to NULL if the conversion fails,
/// so that the operator can also be used to check types:
///
/// \code
///     if (! TfDynamic_cast<TfRefPtr<T2> >(ptr))
///         // complain: ptr is not of type T2
/// \endcode
///
/// Similarly, one can use the \c TfStatic_cast<> operator to statically
/// convert \c TfRefPtrs:
///
/// \code
///        dPtr = TfStatic_cast<TfRefPtr<Derived> >(bPtr);
/// \endcode
///
/// This is faster, but not as safe as using \c TfDynamic_cast.
///
/// Finally, remember that in \c C++, a \c Derived** is
/// not a \c Base**, nor is a \c Derived*& a \c Base*&.  This implies
/// that
///
/// \code
///    TfRefPtr<Base>*  bPtrPtr = &dPtr;      // compilation error
///    TfRefPtr<Base>&  bPtrRef = dPtr;       // compilation error
///    const TfRefPtr<Base>&bPtrRef = dPtr;   // OK
/// \endcode
///
/// The last initialization is legal because the compiler implicitly
/// converts dPtr into a temporary variable of type \c TfRefPtr<Base>.
///
///
/// <B> Thread Safety </B>
///
/// One more comment about thread-safety: the above examples are thread-safe
/// in the sense that if two or more threads create and destroy their \e own
/// \c TfRefPtr objects, the reference counts of the underlying objects are
/// always correct; said another way, the reference count it a thread-safe
/// quantity.
///
/// However, it is never safe for two threads to simultaneously try to alter
/// the same \c TfRefPtr object, nor can two threads safely call methods on the
/// same underlying object unless that object itself guarantees thread safety.
///
/// \anchor refPtr_Tracking
/// <B> Tracking References </B>
///
/// The \c TfRefPtrTracker singleton can track \c TfRefPtr objects that
/// point to particular instances.  The macros \c TF_DECLARE_REFBASE_TRACK
/// and \c TF_DEFINE_REFBASE_TRACK are used to enable tracking.  Tracking
/// is enabled at compile time but which instances to track is chosen at
/// runtime.
///
/// <B> Total Encapsulation  </B>
/// \anchor refPtr_encapsulation
///
/// If you're using \c TfRefPtrs on a type \c T, you probably want
/// to completely forbid clients from creating their own objects of
/// type \c T, and force them to go through \c TfRefPtrs.  Such
/// encapsulation is strongly encouraged.  Here is the recommended
/// technique:
///
/// \code
///
/// typedef TfRefPtr<class Simple> SimpleRefPtr;
///
/// class Simple : public TfRefBase {
/// private:         // use protected if you plan to derive later
///     Simple();
///     Simple(<arg-list>);
/// public:
///     static SimpleRefPtr New() {
///        return TfCreateRefPtr(new Simple);
///     }
///
///     static SimpleRefPtr New(<arg-list>) {
///        return TfCreateRefPtr(new Simple(<arg-list>));
///     }
///
///     ~Simple();
/// };
/// \endcode
///
/// Clients can now only create objects of type \c Simple using a
/// \c TfRefPtr:
///
/// \code
///     Simple    s;                                 // compilation error
///     SimpleRefPtr sPtr1 = new Simple;                // compilation error
///     SimpleRefPtr sPtr2 = Simple::New();             // OK
///     SimpleRefPtr sPtr3 = Simple::New(<arg-list>);   // Ok
/// \endcode
///

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/nullPtr.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/typeFunctions.h"
#include "pxr/base/tf/api.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/hints.h"

#include <boost/functional/hash_fwd.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include <typeinfo>
#include <type_traits>
#include <cstddef>

// Tf_SupportsUniqueChanged is a metafunction that may be specialized to return
// false for classes (and all derived classes) that *cannot* ever invoke unique
// changed listeners.
template <class T>
struct Tf_SupportsUniqueChanged {
    static const bool Value = true;
};

// Remnants are never able to support weak changed listeners.
class Tf_Remnant;
template <>
struct Tf_SupportsUniqueChanged<Tf_Remnant> {
    static const bool Value = false;
};

class TfHash;
class TfWeakBase;

template <class T> class TfWeakPtr;
template <template <class> class X, class Y>
class TfWeakPtrFacade;

// Functions used for tracking.  Do not implement these.
inline void Tf_RefPtrTracker_FirstRef(const void*, const void*) { }
inline void Tf_RefPtrTracker_LastRef(const void*, const void*) { }
inline void Tf_RefPtrTracker_New(const void*, const void*) { }
inline void Tf_RefPtrTracker_Delete(const void*, const void*) { }
inline void Tf_RefPtrTracker_Assign(const void*, const void*, const void*) { }

// This code is used to increment and decrement ref counts in the common case.
// It may lock and invoke the unique changed listener, if the reference count
// becomes unique or non-unique.
struct Tf_RefPtr_UniqueChangedCounter {
    static inline int
    AddRef(TfRefBase const *refBase,
           TfRefBase::UniqueChangedListener const &listener)
    {
        if (refBase) {
            // Check to see if we need to invoke the unique changed listener.
            if (refBase->_shouldInvokeUniqueChangedListener)
                return _AddRef(refBase, listener);
            else
                return refBase->GetRefCount()._FetchAndAdd(1);
        }
        return 0;
    }

    static inline bool
    RemoveRef(TfRefBase const* refBase,
              TfRefBase::UniqueChangedListener const &listener) {
        if (refBase) {
            // Check to see if we need to invoke the unique changed listener.
            return refBase->_shouldInvokeUniqueChangedListener ?
                        _RemoveRef(refBase, listener) :
                        refBase->GetRefCount()._DecrementAndTestIfZero();
        }
        return false;
    }

    TF_API static bool _RemoveRef(TfRefBase const *refBase,
                           TfRefBase::UniqueChangedListener const &listener);

    TF_API static int _AddRef(TfRefBase const *refBase,
                       TfRefBase::UniqueChangedListener const &listener);
};

// This code is used to increment and decrement ref counts in the case where
// the object pointed to explicitly does not support unique changed listeners.
struct Tf_RefPtr_Counter {
    static inline int
    AddRef(TfRefBase const *refBase,
           TfRefBase::UniqueChangedListener const &) {
        if (refBase)
            return refBase->GetRefCount()._FetchAndAdd(1);
        return 0;
    }

    static inline bool
    RemoveRef(const TfRefBase* ptr,
              TfRefBase::UniqueChangedListener const &) {
        return (ptr and (ptr->GetRefCount()._DecrementAndTestIfZero()));
    }
};

/// \class TfRefPtr
/// \ingroup group_tf_Memory
///
/// Reference-counted smart pointer utility class
///
/// The \c TfRefPtr class implements a reference counting on objects
/// that inherit from \c TfRefBase.
///
/// For more information, see either the \ref refPtr_QuickStart "Quick Start"
/// example or read the \ref refPtr_DetailedDiscussion "detailed discussion".
///
template <class T>
class TfRefPtr {
    // Select the counter based on whether T supports unique changed listeners.
    typedef typename boost::mpl::if_c<
        Tf_SupportsUniqueChanged<T>::Value &&
        !boost::is_convertible<T*, TfSimpleRefBase*>::value,
        Tf_RefPtr_UniqueChangedCounter,
        Tf_RefPtr_Counter>::type _Counter;
    
public:
    /// Convenience type accessor to underlying type \c T for template code.
    typedef T DataType;


    template <class U> struct Rebind {
        typedef TfRefPtr<U> Type;
    };    

    /// Initialize pointer to nullptr.
    ///
    /// The default constructor leaves the pointer initialized to point to the
    /// NULL object. Attempts to use the \c -> operator will cause an abort
    /// until the pointer is given a value.
    TfRefPtr() : _refBase(nullptr) {
        Tf_RefPtrTracker_New(this, _GetObjectForTracking());
    }

    /// Initializes \c *this to point at \p p's object.
    ///
    /// Increments \p p's object's reference count.
    TfRefPtr(const TfRefPtr<T>& p) : _refBase(p._refBase) {
        _AddRef();
        Tf_RefPtrTracker_New(this, _GetObjectForTracking());
    }

    /// Initializes \c *this to point at \p gp's object.
    ///
    /// Increments \p gp's object's reference count.
    template <template <class> class X, class U>
    inline TfRefPtr(const TfWeakPtrFacade<X, U>& p,
                    typename boost::enable_if<
                        boost::is_convertible<U*, T*>
                    >::type *dummy = 0);

    /// Transfer a raw pointer to a reference-counted pointer.
    ///
    /// The \c TfCreateRefPtr() function should only be used from within a
    /// static \c New() function (or similarly, a \c Clone() function) of a
    /// reference-counted class.  Reference-counted objects have their
    /// reference count initially set to one to account for the fact that a
    /// newly created object must always persist at least until its \c New()
    /// function returns.  Therefore, the transfer of the pointer returned by
    /// \c new into a reference pointer must \e not increase the reference
    /// count.  The transfer of the raw pointer returned by \c new into the
    /// object returned by \c New() is a "transfer of ownership" and does not
    /// represent an additional reference to the object.
    ///
    /// In summary, this code is wrong, and will return an object that can
    /// never be destroyed:
    ///
    /// \code
    ///     SimpleRefPtr Simple::New() {
    ///         return SimpleRefPtr(new Simple);      // legal, but leaks memory: beware!!
    ///     }
    /// \endcode
    ///
    /// The correct form is
    ///
    /// \code
    ///     SimpleRefPtr Simple::New() {
    ///         return TfCreateRefPtr(new Simple);
    ///     }
    /// \endcode
    ///
    /// Note also that a function which is essentially like \c New(),
    /// for example \c Clone(), would also want to use \c TfCreateRefPtr().
#if defined(doxygen)
    friend inline TfRefPtr TfCreateRefPtr(T*);
#else
    template <class U>
    friend inline TfRefPtr<U> TfCreateRefPtr(U*);
#endif

    /// Initializes to point at \c *ptr.
    ///
    /// Increments \c *ptr's reference count.  Note that newly constructed
    /// objects start with a reference count of one.  Therefore, you should \e
    /// NOT use this constructor (either implicitly or explicitly) from within
    /// a \c New() function.  Use \c TfCreateRefPtr() instead.
    template <class U>
    explicit TfRefPtr(
        U* ptr, typename std::enable_if<
            std::is_convertible<U*, T*>::value>::type * = nullptr) :
        _refBase(ptr)
    {
        _AddRef();
        Tf_RefPtrTracker_New(this, _GetObjectForTracking());
    }

    /// Implicit conversion from \a TfNullPtr to TfRefPtr.
    TfRefPtr(TfNullPtrType) : _refBase(nullptr)
    {
        Tf_RefPtrTracker_New(this, _GetObjectForTracking());
    }

    /// Implicit conversion from \a nullptr to TfRefPtr.
    TfRefPtr(std::nullptr_t) : _refBase(nullptr)
    {
        Tf_RefPtrTracker_New(this, _GetObjectForTracking());
    }

    /// Assigns pointer to point at \c p's object, and increments reference
    /// count.
    ///
    /// The object (if any) pointed at before the assignment has its
    /// reference count decremented, while the object newly pointed at
    /// has its reference count incremented.
    /// If the object previously pointed to now has nobody left to point at it,
    /// the object will typically be destroyed at this point.
    ///
    /// An assignment
    /// \code
    ///     ptr = TfNullPtr;
    /// \endcode
    ///
    /// can be used to make \c ptr "forget" where it is pointing; note
    /// however that this has an important side effect, since it
    /// decrements the reference count of the object previously pointed
    /// to by \c ptr, possibly triggering destruction of that object.
    TfRefPtr<T>& operator= (const TfRefPtr<T>& p) {
        //
        // It is quite possible for
        //   ptr = TfNullPtr;
        // to delete the space that ptr actually lives in (this happens
        // when you use a circular reference to keep an object alive).
        // To avoid a crash, we have to ensure that deletion of the object
        // is the last thing done in the assignment; so we use some
        // local variables to help us out.
        //

        Tf_RefPtrTracker_Assign(this, p._GetObjectForTracking(),
                                _GetObjectForTracking());

        const TfRefBase* tmp = _refBase;
        _refBase = p._refBase;

        p._AddRef();            // first!
        _RemoveRef(tmp);        // second!
        return *this;
    }

    /// Decrements reference count of object being pointed to.
    ///
    /// If the reference count of the object (if any) that was just pointed at
    /// reaches zero, the object will typically be destroyed at this point.
    ~TfRefPtr() {
        Tf_RefPtrTracker_Delete(this, _GetObjectForTracking());
        _RemoveRef(_refBase);
    }

private:
    
    // Compile error if a U* cannot be assigned to a T*.
    template <class U>
    static void _CheckTypeAssignability() {
        T* unused = (U*)0;
        if (unused) unused = 0;
    }

    // Compile error if a T* and U* cannot be compared.
    template <class U>
    static void _CheckTypeComparability() {
        bool unused = ((T*)0 == (U*)0);
        if (unused) unused = false;
    }

public:
    
    /// Initializes to point at \c p's object, and increments reference count.
    ///
    /// This initialization is legal only if
    /// \code
    ///     U* uPtr;
    ///     T* tPtr = uPtr;
    /// \endcode
    /// is legal.
#if !defined(doxygen)
    template <class U>
#endif
    TfRefPtr(const TfRefPtr<U>& p) : _refBase(p._refBase) {
        if (!boost::is_same<T,U>::value) {
            if (false)
                _CheckTypeAssignability<U>();
        }

        _AddRef();
        Tf_RefPtrTracker_New(this, _GetObjectForTracking());
    }

    /// Assigns pointer to point at \c p's object, and increments reference
    /// count.
    ///
    /// This assignment is legal only if
    /// \code
    ///     U* uPtr;
    ///     T* tPtr;
    ///     tPtr = uPtr;
    /// \endcode
    /// is legal.
#if !defined(doxygen)
    template <class U>
#endif
    TfRefPtr<T>& operator= (const TfRefPtr<U>& p) {
        if (!boost::is_same<T,U>::value) {
            if (false)
                _CheckTypeAssignability<U>();
        }

        Tf_RefPtrTracker_Assign(this,
                                reinterpret_cast<T*>(p._GetObjectForTracking()),
                                _GetObjectForTracking());
        const TfRefBase* tmp = _refBase;
        _refBase = p._GetData();
        p._AddRef();            // first!
        _RemoveRef(tmp);        // second!
        return *this;
    }

    /// Returns true if \c *this and \c p point to the same object (or if they
    /// both point to NULL).
    ///
    /// The comparison is legal only if a \c T* and a \c U* are comparable.
#if !defined(doxygen)
    template <class U>
#endif
    bool operator== (const TfRefPtr<U>& p) const {
        if (false)
            _CheckTypeComparability<U>();

        return _refBase == p._refBase;
    }

    /// Returns true if the address of the object pointed to by \c *this
    /// compares less than the address of the object pointed to by \p p.
    ///
    /// The comparison is legal only if a \c T* and a \c U* are comparable.
#if !defined(doxygen)
    template <class U>
#endif
    bool operator< (const TfRefPtr<U>& p) const {
        if (false)
            _CheckTypeComparability<U>();

        return _refBase < p._refBase;
    }

#if !defined(doxygen)
    template <class U>
#endif
    bool operator> (const TfRefPtr<U>& p) const {
        if (false)
            _CheckTypeComparability<U>();

        return _refBase > p._refBase;
    }

#if !defined(doxygen)
    template <class U>
#endif
    bool operator<= (const TfRefPtr<U>& p) const {
        if (false)
            _CheckTypeComparability<U>();

        return _refBase <= p._refBase;
    }

#if !defined(doxygen)
    template <class U>
#endif
    bool operator>= (const TfRefPtr<U>& p) const {
        if (false)
            _CheckTypeComparability<U>();

        return _refBase >= p._refBase;
    }

    /// Returns true if \c *this and \c p do not point to the same object.
    ///
    /// The comparison is legal only if a \c T* and a \c U* are comparable.
#if !defined(doxygen)
    template <class U>
#endif
    bool operator!= (const TfRefPtr<U>& p) const {
        if (false)
            _CheckTypeComparability<U>();

        return _refBase != p._refBase;
    }

    /// Accessor to \c T's public members.
    T* operator ->() const {
        if (ARCH_UNLIKELY(!static_cast<void*>(_refBase)))
            TF_FATAL_ERROR("attempted member lookup on NULL %s",
                           ArchGetDemangled(typeid(TfRefPtr)).c_str());
        return static_cast<T*>(const_cast<TfRefBase*>(_refBase));
    }

#if !defined(doxygen)
    using UnspecifiedBoolType = const TfRefBase * (TfRefPtr::*);
#endif

    /// True if the pointer points to an object.
    operator UnspecifiedBoolType() const {
        return static_cast<void*>(_refBase) ? &TfRefPtr::_refBase : NULL;
    }

    /// True if the pointer points to \c NULL.
    bool operator !() const {
        return _refBase == nullptr;
    }

    /// Swap this pointer with \a other.
    /// After this operation, this pointer will point to what \a other
    /// formerly pointed to, and \a other will point to what this pointer
    /// formerly pointed to.
    void swap(TfRefPtr &other) {
        Tf_RefPtrTracker_Assign(this, other._GetObjectForTracking(),
                                _GetObjectForTracking());
        Tf_RefPtrTracker_Assign(&other, _GetObjectForTracking(),
                                other._GetObjectForTracking());
        std::swap(_refBase, other._refBase);
    }

    /// Set this pointer to point to no object.
    /// Equivalent to assignment with TfNullPtr.
    void Reset() {
        *this = TfNullPtr;
    }        

private:
    const TfRefBase* _refBase;

    friend class TfHash;
    template <class U>
    friend inline size_t hash_value(const TfRefPtr<U>&);

    friend T *get_pointer(TfRefPtr const &p) {
        return static_cast<T *>(const_cast<TfRefBase *>(p._refBase));
    }

    // Used to distinguish construction in TfCreateRefPtr.
    class _CreateRefPtr { };

    // private constructor, used by TfCreateRefPtr()
    TfRefPtr(T* ptr, _CreateRefPtr /* unused */)
        : _refBase(ptr)
    {
        /* reference count is NOT bumped */
        Tf_RefPtrTracker_FirstRef(this, _GetObjectForTracking());
        Tf_RefPtrTracker_New(this, _GetObjectForTracking());
    }

    // Hide confusing internals of actual C++ definition (e.g. DataType)
    // for doxygen output:

    /// Allows dynamic casting of a \c TfRefPtr.
    ///
    /// If it is legal to dynamically cast a \c T* to a \c D* , then
    /// the following is also legal:
    /// \code
    ///     TfRefPtr<T> tPtr = ... ;
    ///     TfRefPtr<D> dPtr;
    ///
    ///     if (!(dPtr = TfDynamic_cast< TfRefPtr<D> >(tPtr)))
    ///         ...;       // cast failed
    /// \endcode
    /// The runtime performance of this function is exactly the same
    /// as a \c dynamic_cast (i.e. one virtual function call). If the pointer
    /// being cast is NULL or does not point to an object of the requisite
    /// type, the result is a \c TfRefPtr pointing to NULL.
#if defined(doxygen)
    // Sanitized for documentation:
    template <class D>
    friend inline TfRef<D> TfDynamic_cast(const TfRefPtr<T>&);
#else
    template <class D, class B>
    friend TfRefPtr<typename D::DataType>
    TfDynamic_cast(const TfRefPtr<B>&);

    template <class D, class B>
    friend TfRefPtr<typename D::DataType>
    TfSafeDynamic_cast(const TfRefPtr<B>&);
#endif

    /// Allows static casting of a \c TfRefPtr.
    ///
    /// If it is legal to statically cast a \c T* to a \c D* , then
    /// the following is also legal:
    /// \code
    ///     TfRefPtr<T> tPtr = ... ;
    ///     TfRefPtr<D> dPtr;
    ///
    ///     dPtr = TfStatic_cast< TfRefPtr<D> >(tPtr);
    /// \endcode
    /// The runtime performance of this function is exactly the same
    /// as a regular \c TfRefPtr initialization, since the cost of
    /// the underlying \c static_cast is zero.  Of course, a \c TfDynamic_cast
    /// is preferred, assuming the underlying types are polymorphic
    /// (i.e. have virtual functions).
    ///
#if defined(doxygen)
    // Sanitized for documentation:
    template <class D>
    friend inline TfRefPtr<D> TfStatic_cast(const TfRefPtr<T>&);
#else
    template <class D, class B>
    friend TfRefPtr<typename D::DataType>
    TfStatic_cast(const TfRefPtr<B>&);

#endif

    /// Allows const casting of a \c TfRefPtr.
    ///
    /// The following is always legal:
    /// \code
    ///     TfRefPtr<const T> cPtr = ...;
    ///     TfRefPtr<T>       tPtr;
    ///
    ///     tPtr = TfConst_cast< TfRefPtr<T> >(cPtr);
    /// \endcode
    /// As with the C++ \c const_cast operator, use of this function is
    /// discouraged.
#if defined(doxygen)
    // Sanitized for documentation:
    template <class D>
    friend inline TfRefPtr<D> TfConst_cast(const TfRefPtr<const D>&);
#else
    template <class D>
    friend TfRefPtr<typename D::DataType>
    TfConst_cast(const TfRefPtr<const typename D::DataType>&);
#endif

    T* _GetData() const {
        return static_cast<T*>(const_cast<TfRefBase*>(_refBase));
    }
    
    // This method is only used when calling the hook functions for
    // tracking.  We reinterpret_cast instead of static_cast so that
    // we don't need the definition of T.  However, if TfRefBase is
    // not the first base class of T then the resulting pointer may
    // not point to a T.  Nevertheless, it should be consistent to
    // all calls to the tracking functions.
    T* _GetObjectForTracking() const {
        return reinterpret_cast<T*>(const_cast<TfRefBase*>(_refBase));
    }

    /// Call \c typeid on the object pointed to by a \c TfRefPtr.
    ///
    /// If \c ptr is a \c TfRefPtr, \c typeid(ptr) will return
    /// type information about the \c TfRefPtr.  To access type
    /// information about the object pointed to by a \c TfRefPtr,
    /// one can use \c TfTypeid.

    template <class U>
    friend const std::type_info& TfTypeid(const TfRefPtr<U>& ptr);

    void _AddRef() const {
        _Counter::AddRef(_refBase, TfRefBase::_uniqueChangedListener);
    }

    void _RemoveRef(const TfRefBase* ptr) const {
        if (_Counter::RemoveRef(ptr, TfRefBase::_uniqueChangedListener)) {
            Tf_RefPtrTracker_LastRef(this,
                reinterpret_cast<T*>(const_cast<TfRefBase*>(ptr)));
            delete ptr;
        }
    }

#if ! defined(doxygen)
    // doxygen is very confused by this. It declares all TfRefPtrs
    // to be friends.
    template <class U> friend class TfRefPtr;
    template <class U> friend class TfWeakPtr;
    friend class Tf_Remnant;

    template <class U>
    friend TfRefPtr<U> TfCreateRefPtrFromProtectedWeakPtr(TfWeakPtr<U> const &);
#endif
    friend class TfWeakBase;
};

#if !defined(doxygen)

//
// nullptr comparisons
//
// These are provided to avoid ambiguous overloads due to
// TfWeakPtrFacade::Derived comparisons with TfRefPtr.
//

template <class T>
inline bool operator== (const TfRefPtr<T> &p, std::nullptr_t)
{
    return not p;
}
template <class T>
inline bool operator== (std::nullptr_t, const TfRefPtr<T> &p)
{
    return not p;
}

template <class T>
inline bool operator!= (const TfRefPtr<T> &p, std::nullptr_t)
{
    return not (p == nullptr);
}
template <class T>
inline bool operator!= (std::nullptr_t, const TfRefPtr<T> &p)
{
    return not (nullptr == p);
}

template <class T>
inline bool operator< (const TfRefPtr<T> &p, std::nullptr_t)
{
    return std::less<const TfRefBase *>()(get_pointer(p), nullptr);
}
template <class T>
inline bool operator< (std::nullptr_t, const TfRefPtr<T> &p)
{
    return std::less<const TfRefBase *>()(nullptr, get_pointer(p));
}

template <class T>
inline bool operator<= (const TfRefPtr<T> &p, std::nullptr_t)
{
    return not (nullptr < p);
}
template <class T>
inline bool operator<= (std::nullptr_t, const TfRefPtr<T> &p)
{
    return not (p < nullptr);
}

template <class T>
inline bool operator> (const TfRefPtr<T> &p, std::nullptr_t)
{
    return nullptr < p;
}
template <class T>
inline bool operator> (std::nullptr_t, const TfRefPtr<T> &p)
{
    return p < nullptr;
}

template <class T>
inline bool operator>= (const TfRefPtr<T> &p, std::nullptr_t)
{
    return not (p < nullptr);
}
template <class T>
inline bool operator>= (std::nullptr_t, const TfRefPtr<T> &p)
{
    return not (nullptr < p);
}


template <typename T>
inline TfRefPtr<T> TfCreateRefPtr(T* ptr) {
    return TfRefPtr<T>(ptr, typename TfRefPtr<T>::_CreateRefPtr());
}

template <class T>
const std::type_info&
TfTypeid(const TfRefPtr<T>& ptr)
{
    if (ARCH_UNLIKELY(!ptr._refBase))
        TF_FATAL_ERROR("called TfTypeid on NULL TfRefPtr");

    return typeid(*ptr._GetData());
}

template <class D, class T>
inline
TfRefPtr<typename D::DataType>
TfDynamic_cast(const TfRefPtr<T>& ptr)
{
    typedef TfRefPtr<typename D::DataType> RefPtr;
    return RefPtr(dynamic_cast<typename D::DataType*>(ptr._GetData()));
}

template <class D, class T>
inline
TfRefPtr<typename D::DataType>
TfSafeDynamic_cast(const TfRefPtr<T>& ptr)
{
    typedef TfRefPtr<typename D::DataType> RefPtr;
    return RefPtr(TfSafeDynamic_cast<typename D::DataType*>(ptr._GetData()));
}

template <class D, class T>
inline
TfRefPtr<typename D::DataType>
TfStatic_cast(const TfRefPtr<T>& ptr)
{
    typedef TfRefPtr<typename D::DataType> RefPtr;
    return RefPtr(static_cast<typename D::DataType*>(ptr._GetData()));
}

template <class T>
inline
TfRefPtr<typename T::DataType>
TfConst_cast(const TfRefPtr<const typename T::DataType>& ptr)
{
    // this ugly cast allows TfConst_cast to work without requiring
    // a definition for T.
    typedef TfRefPtr<typename T::DataType> NonConstRefPtr;
    return *((NonConstRefPtr*)(&ptr));
}

// Specialization: prevent construction of a TfRefPtr<TfRefBase>.

template <>
class TfRefPtr<TfRefBase> {
private:
    TfRefPtr<TfRefBase>() {
    }
};

template <>
class TfRefPtr<const TfRefBase> {
private:
    TfRefPtr<const TfRefBase>() {
    }
};

template <class T>
struct TfTypeFunctions<TfRefPtr<T> > {
    static T* GetRawPtr(const TfRefPtr<T>& t) {
        return t.operator-> ();
    }

    static TfRefPtr<T> ConstructFromRawPtr(T* ptr) {
        return TfRefPtr<T>(ptr);
    }
    
    static bool IsNull(const TfRefPtr<T>& t) {
        return !t;
    }

    static void Class_Object_MUST_Be_Passed_By_Address() { }
    static void Class_Object_MUST_Not_Be_Const() { }
};

template <class T>
struct TfTypeFunctions<TfRefPtr<const T> > {
    static const T* GetRawPtr(const TfRefPtr<const T>& t) {
        return t.operator-> ();
    }

    static TfRefPtr<const T> ConstructFromRawPtr(T* ptr) {
        return TfRefPtr<const T>(ptr);
    }

    static bool IsNull(const TfRefPtr<const T>& t) {
        return !t;
    }

    static void Class_Object_MUST_Be_Passed_By_Address() { }
};

#endif

#if !defined(doxygen)

namespace boost {

template<typename T>
T *
get_pointer(TfRefPtr<T> const& p)
{
    return get_pointer(p);
}

}

// Extend boost::hash to support TfRefPtr.
template <class T>
inline size_t
hash_value(const TfRefPtr<T>& ptr)
{
    // Make the boost::hash type depend on T so that we don't have to always
    // include boost/functional/hash.hpp in this header for the definition of
    // boost::hash.
    auto refBase = ptr._refBase;
    return boost::hash<decltype(refBase)>()(refBase);
}

#endif // !doxygen

#define TF_SUPPORTS_REFPTR(T)   boost::is_base_of<TfRefBase, T >::value

#if defined(ARCH_COMPILER_MSVC) 
// There is a bug in the compiler which means we have to provide this
// implementation. See here for more information:
// https://connect.microsoft.com/VisualStudio/Feedback/Details/2852624

#define TF_REFPTR_CONST_VOLATILE_GET(x)                                       \
        namespace boost                                                       \
        {                                                                     \
            template<>                                                        \
            const volatile x*                                                 \
                get_pointer(const volatile x* p)                              \
            {                                                                 \
                return p;                                                     \
            }                                                                 \
        }
#else
#define TF_REFPTR_CONST_VOLATILE_GET(x)
#endif

#endif
