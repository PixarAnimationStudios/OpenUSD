//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyNoticeWrapper.h"

using std::map;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_boost::python;

TfPyNoticeWrapperBase::~TfPyNoticeWrapperBase()
{
}



Tf_PyNoticeObjectGenerator::MakeObjectFunc
Tf_PyNoticeObjectGenerator::_Lookup(TfNotice const &n)
{
    if (!_generators.IsInitialized())
        return 0;
    string typeName = typeid(n).name();
    map<string, MakeObjectFunc>::iterator i = _generators->find(typeName);
    return i == _generators->end() ? 0 : i->second;
}


object
Tf_PyNoticeObjectGenerator::Invoke(TfNotice const &n)
{
    TfPyLock lock;  // Take python lock since generators call to python.
    MakeObjectFunc func = _Lookup(n);
    return func ? func(n) : object();
}

TfStaticData<map<string, Tf_PyNoticeObjectGenerator::MakeObjectFunc> >
    Tf_PyNoticeObjectGenerator::_generators;

PXR_NAMESPACE_CLOSE_SCOPE
