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
