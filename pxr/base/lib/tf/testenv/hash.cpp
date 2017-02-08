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
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/weakPtr.h"
#include <boost/functional/hash.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

class Dolly : public TfRefBase, public TfWeakBase {
public:
    typedef TfRefPtr<Dolly> DollyRefPtr;
    typedef TfWeakPtr<Dolly> DollyPtr;
    static DollyRefPtr New() {
       // warning: return new Dolly directly will leak memory!
       return TfCreateRefPtr(new Dolly);
    }

    ~Dolly() {};

 private:
    Dolly() {};
    Dolly(bool) {};
};


static bool
Test_TfHash()
{
    Dolly::DollyRefPtr ref = Dolly::New();

    size_t hash = hash_value(ref);
    printf("hash(TfRefPtr): %zu\n", hash);

    Dolly::DollyPtr weak(ref);
    hash = hash_value(weak);
    printf("hash(TfWeakPtr): %zu\n", hash);

    bool status = true;
    return status;
}

TF_ADD_REGTEST(TfHash);

