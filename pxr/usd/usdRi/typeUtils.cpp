//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "typeUtils.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE


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
    return SdfSchema::GetInstance().FindOrCreateType(TfToken(riType));
}

PXR_NAMESPACE_CLOSE_SCOPE

