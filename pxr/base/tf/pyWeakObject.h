//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_WEAK_OBJECT_H
#define PXR_BASE_TF_PY_WEAK_OBJECT_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pyIdentity.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"

#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>

#include "pxr/base/tf/hashmap.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef TfWeakPtr<struct Tf_PyWeakObject> Tf_PyWeakObjectPtr;

// A weak pointable weak reference to a python object.
struct Tf_PyWeakObject : public TfWeakBase
{
public:
    typedef Tf_PyWeakObject This;

    static Tf_PyWeakObjectPtr GetOrCreate(boost::python::object const &obj);
    boost::python::object GetObject() const;
    void Delete();
    
private:
    explicit Tf_PyWeakObject(boost::python::object const &obj);
    
    boost::python::handle<> _weakRef;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_WEAK_OBJECT_H
