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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <nanobind/nanobind.h>

#include <type_traits>
#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

struct Tf_PyNoticeObjectGenerator {
    typedef Tf_PyNoticeObjectGenerator This;
    typedef nanobind::object (*MakeObjectFunc)(TfNotice const &);

    // Register the generator for notice type T.
    template <typename T>
    static void Register() {
        // XXX this stuff should be keyed directly off TfType now
        (*_generators)[typeid(T).name()] = This::_Generate<T>;
    }

    // Produce a nanobind::object for the correct derived type of \a n.
    TF_API static nanobind::object Invoke(TfNotice const &n);

private:

    template <typename T>
    static nanobind::object _Generate(TfNotice const &n) {
        // Python locking is left to the caller.
        return nanobind::cast(static_cast<T const &>(n),
                              nanobind::rv_policy::copy);
    }

    static MakeObjectFunc _Lookup(TfNotice const &n);

    TF_API static TfStaticData<std::map<std::string, MakeObjectFunc> > _generators;

};

#define TF_INSTANTIATE_NOTICE_WRAPPER(T, Base)

template <typename NoticeType, typename BaseType>
struct TfNbNoticeWrapper {
    using ClassType = std::conditional_t<
        std::is_same<NoticeType, TfNotice>::value,
        nanobind::class_<NoticeType>,
        nanobind::class_<NoticeType, BaseType>>;

    static ClassType Wrap(nanobind::handle scope,
                          std::string const &name = std::string()) {
        std::string wrappedName = name;
        if (wrappedName.empty()) {
            wrappedName = TfType::Find<NoticeType>().GetTypeName();
            if (!TfStringGetSuffix(wrappedName, ':').empty())
                wrappedName = TfStringGetSuffix(wrappedName, ':');
        }
        Tf_PyNoticeObjectGenerator::Register<NoticeType>();
        ClassType cls(scope, wrappedName.c_str());
        cls.def(TfTypePythonClass());
        return cls;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_NOTICE_WRAPPER_H
