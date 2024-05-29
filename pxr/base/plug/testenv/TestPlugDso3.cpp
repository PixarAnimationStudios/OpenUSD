//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/testPlugBase.h"

PXR_NAMESPACE_OPEN_SCOPE

class TestPlugDerived3_3 : public _TestPlugBase3 {
  public:
    typedef TestPlugDerived3_3 This;
    typedef TfRefPtr<This> RefPtr;
    typedef TfWeakPtr<This> Ptr;

    virtual ~TestPlugDerived3_3() {}

    virtual std::string GetTypeName() { return "TestPlugDerived3_3"; }

    static RefPtr New() {
        return TfCreateRefPtr(new This());
    }

  protected:
    TestPlugDerived3_3() {}
};

class TestPlugDerived3_4 : public _TestPlugBase4 {
  public:
    typedef TestPlugDerived3_4 This;
    typedef TfRefPtr<This> RefPtr;
    typedef TfWeakPtr<This> Ptr;

    virtual ~TestPlugDerived3_4() {}

    virtual std::string GetTypeName() { return "TestPlugDerived3_4"; }

    static RefPtr New() {
        return TfCreateRefPtr(new This());
    }

  protected:
    TestPlugDerived3_4() {}
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TestPlugDerived3_3,
                   TfType::Bases<_TestPlugBase3> >()
        .SetFactory<_TestPlugFactory<TestPlugDerived3_3> >()
        ;
    TfType::Define<TestPlugDerived3_4,
                   TfType::Bases<_TestPlugBase4> >()
        .SetFactory<_TestPlugFactory<TestPlugDerived3_4> >()
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
