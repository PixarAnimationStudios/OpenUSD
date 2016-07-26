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
#include "typeUtils.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"

using std::string;

string 
UsdRi_GetRiType(const SdfValueTypeName &usdType)
{
    // TODO
    return "";
}

SdfValueTypeName 
UsdRi_GetUsdType(const string &riType)
{
    struct Entry { const char* riName; SdfValueTypeName usdType; };
    static Entry map[] = {
        { "color",  SdfValueTypeNames->Color3f  },
        { "vector", SdfValueTypeNames->Vector3d },
        { "normal", SdfValueTypeNames->Normal3d },
        { "point",  SdfValueTypeNames->Point3d  },
        { "matrix", SdfValueTypeNames->Matrix4d }
    };
    static const size_t mapLen = sizeof(map)/sizeof(map[0]);

    for (size_t i = 0; i != mapLen; ++i) {
        if (riType.find(map[i].riName) != string::npos)
            return map[i].usdType;
    }
    // XXX -- Really?
    return SdfSchema::GetInstance().FindOrCreateType(riType);
}
