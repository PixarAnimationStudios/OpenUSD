//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/staticKeyData.h"

#include "pxr/pxr.h"
#include "pxr/base/arch/function.h"

#include <cstring>

PXR_NAMESPACE_OPEN_SCOPE

static bool
_StrEqual(const char* a, const char* b)
{
    if (a == b) {
        return true;
    } else {
        if (a && b) {
            return std::strcmp(a, b) == 0;
        } else {
            return false;
        }
    }
}

bool 
TraceStaticKeyData::operator == (const TraceStaticKeyData& other) const
{
    return _StrEqual(_funcName, other._funcName)
        && _StrEqual(_prettyFuncName, other._prettyFuncName)
        && _StrEqual(_name, other._name);
}

std::string
TraceStaticKeyData::GetString() const
{
    std::string s;
    if (_funcName && _prettyFuncName) {
        if (_name) {
            s = ArchGetPrettierFunctionName(_funcName, _prettyFuncName)
                + " ("+_name+")";
        } else {
            s = ArchGetPrettierFunctionName(_funcName, _prettyFuncName);
        }
    } else {
        s =  _name;
    }
    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
