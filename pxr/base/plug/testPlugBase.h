//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_PLUG_TEST_PLUG_BASE_H
#define PXR_BASE_PLUG_TEST_PLUG_BASE_H

#include "pxr/pxr.h"
#include "pxr/base/plug/api.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/plug/plugin.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

template <int M>
class _TestPlugBase : public TfRefBase, public TfWeakBase {
  public:
    typedef _TestPlugBase This;
    typedef TfRefPtr<This> RefPtr;
    typedef TfWeakPtr<This> Ptr;
    constexpr static int N = M;

    virtual ~_TestPlugBase() {}

    virtual std::string GetTypeName() {
        return TfType::Find(this).GetTypeName();
    }

    static RefPtr New() {
        return TfCreateRefPtr(new This());
    }

    bool TestAcceptPluginSequence(const PlugPluginPtrVector &plugins) {
        return true;
    }

    PLUG_API
    static RefPtr Manufacture(const std::string & subclass);

  protected:
    _TestPlugBase() {}
};

template <int N>
class _TestPlugFactoryBase : public TfType::FactoryBase {
public:
    virtual TfRefPtr<_TestPlugBase<N> > New() const = 0;
};

template <typename T>
class _TestPlugFactory : public _TestPlugFactoryBase<T::N> {
public:
    virtual TfRefPtr<_TestPlugBase<T::N> > New() const
    {
        return T::New();
    }
};

typedef _TestPlugBase<1> _TestPlugBase1;
typedef _TestPlugBase<2> _TestPlugBase2;
typedef _TestPlugBase<3> _TestPlugBase3;
typedef _TestPlugBase<4> _TestPlugBase4;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_PLUG_TEST_PLUG_BASE_H
