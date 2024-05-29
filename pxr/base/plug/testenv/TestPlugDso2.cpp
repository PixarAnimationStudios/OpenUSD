//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/testPlugBase.h"

PXR_NAMESPACE_OPEN_SCOPE

class TestPlugDerived2 : public _TestPlugBase2 {
  public:
    typedef TestPlugDerived2 This;
    typedef TfRefPtr<This> RefPtr;
    typedef TfWeakPtr<This> Ptr;

    virtual ~TestPlugDerived2() {}

    virtual std::string GetTypeName() { return "TestPlugDerived2"; }

    static RefPtr New() {
        return TfCreateRefPtr(new This());
    }

  protected:
    TestPlugDerived2() {}
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TestPlugDerived2,
                   TfType::Bases<_TestPlugBase2> >()
        .SetFactory<_TestPlugFactory<TestPlugDerived2> >()
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
