//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/typeInfoMap.h"
#include "pxr/base/tf/regTest.h"

PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfTypeInfoMap()
{
    TfTypeInfoMap<int> m;

    TF_AXIOM(m.Exists("doubleAlias") == false);
    TF_AXIOM(m.Exists(typeid(double)) == false);
    TF_AXIOM(m.Exists(typeid(double).name()) == false);

    TF_AXIOM(m.Find("doubleAlias") == NULL);
    TF_AXIOM(m.Find(typeid(double)) == NULL);
    TF_AXIOM(m.Find(typeid(double).name()) == NULL);

    m.Set(typeid(double), 13);

    TF_AXIOM(m.Find(typeid(double)) && *m.Find(typeid(double)) == 13);
    TF_AXIOM(m.Find("doubleAlias") == NULL);
    TF_AXIOM(m.Exists(typeid(double)));
    TF_AXIOM(m.Exists(typeid(double).name()));

    m.CreateAlias("doubleAlias", typeid(double));
    TF_AXIOM(m.Exists("doubleAlias"));

    m.Remove(typeid(double));
    
    TF_AXIOM(m.Exists("doubleAlias") == false);
    TF_AXIOM(m.Exists(typeid(double)) == false);
    TF_AXIOM(m.Exists(typeid(double).name()) == false);

    m.Set(typeid(double).name(), 14);
    TF_AXIOM(m.Exists(typeid(double)));
    m.CreateAlias("doubleAlias", typeid(double).name());
    TF_AXIOM(m.Exists("doubleAlias"));

    return true;
}

TF_ADD_REGTEST(TfTypeInfoMap);
