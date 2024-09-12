//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_NOTICE_WRAPPER_H
#define PXR_BASE_TF_PY_NOTICE_WRAPPER_H

#include "pxr/pxr.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjectFinder.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python/bases.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/extract.hpp"
#include "pxr/external/boost/python/handle.hpp"

#include <type_traits>
#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

struct Tf_PyNoticeObjectGenerator {
    typedef Tf_PyNoticeObjectGenerator This;
    typedef pxr_boost::python::object (*MakeObjectFunc)(TfNotice const &);

    // Register the generator for notice type T.
    template <typename T>
    static void Register() {
        // XXX this stuff should be keyed directly off TfType now
        (*_generators)[typeid(T).name()] = This::_Generate<T>;
    }
    
    // Produce a pxr_boost::python::object for the correct derived type of \a n.
    TF_API static pxr_boost::python::object Invoke(TfNotice const &n);

private:

    template <typename T>
    static pxr_boost::python::object _Generate(TfNotice const &n) {
        // Python locking is left to the caller.
        return pxr_boost::python::object(static_cast<T const &>(n));
    }

    static MakeObjectFunc _Lookup(TfNotice const &n);

    TF_API static TfStaticData<std::map<std::string, MakeObjectFunc> > _generators;

};

struct TfPyNoticeWrapperBase : public TfType::PyPolymorphicBase {
    TF_API virtual ~TfPyNoticeWrapperBase();
    virtual pxr_boost::python::handle<> GetNoticePythonObject() const = 0;
};

template <class Notice>
struct Tf_PyNoticeObjectFinder : public Tf_PyObjectFinderBase {
    virtual ~Tf_PyNoticeObjectFinder() {}
    virtual pxr_boost::python::object Find(void const *objPtr) const {
        using namespace pxr_boost::python;
        TfPyLock lock;
        Notice const *wrapper = static_cast<Notice const *>(objPtr);
        return wrapper ? object(wrapper->GetNoticePythonObject()) : object();
    }
};

template <typename NoticeType, typename BaseType>
struct TfPyNoticeWrapper : public NoticeType, public TfPyNoticeWrapperBase {
private:
    static_assert(std::is_base_of<TfNotice, NoticeType>::value
                  || std::is_same<TfNotice, NoticeType>::value,
                  "Notice type must be derived from or equal to TfNotice.");

    static_assert(std::is_base_of<TfNotice, BaseType>::value
                  || std::is_same<TfNotice, BaseType>::value,
                  "BaseType type must be derived from or equal to TfNotice.");

    static_assert(std::is_base_of<BaseType, NoticeType>::value
                  || (std::is_same<NoticeType, TfNotice>::value
                      && std::is_same<BaseType, TfNotice>::value),
                  "BaseType type must be a base of notice, unless both "
                  "BaseType and Notice type are equal to TfNotice.");

public:

    typedef TfPyNoticeWrapper<NoticeType, BaseType> This;

    // If Notice is really TfNotice, then this is the root of the hierarchy and
    // bases is empty, otherwise bases contains the base class.
    using Bases = std::conditional_t<std::is_same<NoticeType, TfNotice>::value,
                                     pxr_boost::python::bases<>,
                                     pxr_boost::python::bases<BaseType>>;

    typedef pxr_boost::python::class_<NoticeType, This, Bases> ClassType;

    static ClassType Wrap(std::string const &name = std::string()) {
        std::string wrappedName = name;
        if (wrappedName.empty()) {
            // Assume they want the last bit of a qualified name.
            wrappedName = TfType::Find<NoticeType>().GetTypeName();
            if (!TfStringGetSuffix(wrappedName, ':').empty())
                wrappedName = TfStringGetSuffix(wrappedName, ':'); 
        }
        Tf_PyNoticeObjectGenerator::Register<NoticeType>();
        Tf_RegisterPythonObjectFinderInternal
            (typeid(TfPyNoticeWrapper),
             new Tf_PyNoticeObjectFinder<TfPyNoticeWrapper>);
        return ClassType(wrappedName.c_str(), pxr_boost::python::no_init)
            .def(TfTypePythonClass());
    }

    // Implement the base class's virtual method.
    virtual pxr_boost::python::handle<> GetNoticePythonObject() const {
        TfPyLock lock;
        return pxr_boost::python::handle<>(pxr_boost::python::borrowed(_self));
    }

    // Arbitrary argument constructor (with a leading PyObject *) which
    // forwards to the base Notice class's constructor.
    template <typename... Args>
    TfPyNoticeWrapper(PyObject *self, Args... args)
        : NoticeType(args...)
        , _self(self) {}
    
private:
    PyObject *_self;

};

#define TF_INSTANTIATE_NOTICE_WRAPPER(T, Base) \
TF_REGISTRY_FUNCTION(TfType) \
{ \
    TfType::Define< TfPyNoticeWrapper<T, Base>, \
                    TfType::Bases<Base> >(); \
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_NOTICE_WRAPPER_H
