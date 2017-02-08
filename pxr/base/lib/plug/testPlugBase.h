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
#ifndef TEST_PLUGBASE_H
#define TEST_PLUGBASE_H

#include "pxr/pxr.h"
#include "pxr/base/plug/api.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"

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

#endif // TEST_PLUGBASE_H
