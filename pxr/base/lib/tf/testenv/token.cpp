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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/diagnostic.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfToken()
{
    TfToken empty1, empty2, nonEmpty3("nonEmpty");
    std::string  sEmpty1;
    TF_AXIOM(empty1 == empty2);
    TF_AXIOM(empty1 != nonEmpty3);
    TF_AXIOM(empty1.Hash() == empty2.Hash());
    TF_AXIOM(empty1 == "" && empty2 == "");
    TF_AXIOM(sEmpty1 == empty1 && sEmpty1 == empty2);
    TF_AXIOM(empty1 == sEmpty1 && empty2 == sEmpty1);
    TF_AXIOM(sEmpty1 != nonEmpty3);
    TF_AXIOM(nonEmpty3 != sEmpty1);
    TF_AXIOM("" == empty1);
    TF_AXIOM("" != nonEmpty3);
    TF_AXIOM(empty1.IsEmpty());
    TF_AXIOM( !nonEmpty3.IsEmpty());

    // Test swapping.
    empty1.Swap(nonEmpty3);
    TF_AXIOM(nonEmpty3.IsEmpty());
    TF_AXIOM(!empty1.IsEmpty());
    TF_AXIOM(empty1 == "nonEmpty");
    TF_AXIOM(nonEmpty3 == "");

    std::swap(nonEmpty3, empty1);
    TF_AXIOM(empty1.IsEmpty());
    TF_AXIOM(!nonEmpty3.IsEmpty());
    TF_AXIOM(empty1 == "");
    TF_AXIOM(nonEmpty3 == "nonEmpty");
    
    std::string a1("alphabet");
    const char *a2 = "alphabet";

    std::cout << TfToken(a1);

    std::string b1("barnacle");
    const char *b2 = "barnacle";

    std::string c1("cinnamon");
    const char *c2 = "cinnamon";

    TF_AXIOM(TfToken(a1) < TfToken(b1));


    TF_AXIOM(TfToken(a1) == TfToken(a1));
    TF_AXIOM(TfToken(a1) == TfToken(a2));
    TF_AXIOM(TfToken(a1).Hash() == TfToken(a2).Hash());

    TF_AXIOM(TfToken(b1) == TfToken(b1));
    TF_AXIOM(TfToken(b1) == TfToken(b2));
    TF_AXIOM(TfToken(b1).Hash() == TfToken(b2).Hash());

    TF_AXIOM(TfToken(c1) == TfToken(c1));
    TF_AXIOM(TfToken(c1) == TfToken(c2));
    TF_AXIOM(TfToken(c1).Hash() == TfToken(c2).Hash());

    TF_AXIOM(TfToken(a1).Hash() != TfToken(b1).Hash());

    TF_AXIOM(TfToken(a1) != TfToken(b1));
    TF_AXIOM(TfToken(a1) != TfToken(c1));
    TF_AXIOM(TfToken(b1) != TfToken(c1));

    TF_AXIOM(TfToken(b1) > TfToken(a1));
    TfToken t1(a1);
    TfToken t2(t1);  // Copy construct
    
    TF_AXIOM(t1 == t2);
    
    t1 = TfToken(b1);

    TF_AXIOM(t1 != t2);

    t2 = TfToken(b2);

    TF_AXIOM(t1 == t2);
    TF_AXIOM(t1 == TfToken("barnacle"));

    std::vector<std::string> strVec;

    strVec.push_back("string1");
    strVec.push_back("string2");
    strVec.push_back("string3");

    std::vector<TfToken> tokVec = TfToTokenVector(strVec);

    TF_AXIOM(TfToken(strVec[0]) == tokVec[0]);
    TF_AXIOM(TfToken(strVec[1]) == tokVec[1]);
    TF_AXIOM(TfToken(strVec[2]) == tokVec[2]);

    std::vector<std::string> strVec2 = TfToStringVector(tokVec);
    TF_AXIOM(TfToken(strVec2[0]) == tokVec[0]);
    TF_AXIOM(TfToken(strVec2[1]) == tokVec[1]);
    TF_AXIOM(TfToken(strVec2[2]) == tokVec[2]);

    return true;
}

TF_ADD_REGTEST(TfToken);
