//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/vt/valueTransform.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueRef.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/spinRWMutex.h"

#include <typeindex>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

using ErasedXformFnRef = TfFunctionRef<VtValue (VtValueRef)>;

using VoidFn = void (*)();
using TypedXfFn = VtValue (*)(VtValueRef obj, void const *, VoidFn);
using ErasedXfFn = VtValue (*)(VtValueRef obj, VoidFn, ErasedXformFnRef);

// A named pair of type_index, used to key transform registrations.
struct Types
{
    Types(std::type_index const &obj,
          std::type_index const &xf) : obj(obj), xf(xf) {}

    template <class HashState>
    friend void TfHashAppend(HashState &h, Types const &t) {
        h.Append(t.obj.hash_code(), t.xf.hash_code());
    }

    friend bool operator==(Types const &lhs, Types const &rhs) {
        return lhs.obj == rhs.obj && lhs.xf == rhs.xf;
    }
    
    std::type_index obj;
    std::type_index xf;
};

// A pair of function pointers, representing a typed transform registration.
// The function-call operator trampolines to the type-specific function.
struct TypedXformFunc
{
    TypedXformFunc(VoidFn vfn, TypedXfFn xfn) : voidFn(vfn), xFn(xfn) {}

    VtValue operator()(VtValueRef obj, void const *xfObj) const {
        return xFn(obj, xfObj, voidFn);
    }

    VoidFn voidFn;
    TypedXfFn xFn;
};

// A pair of function pointers, representing a type-erased transform
// registration.  The function-call operator trampolines to the type-specific
// function.
struct ErasedXformFunc
{
    ErasedXformFunc(VoidFn vfn, ErasedXfFn xfn) : voidFn(vfn), xFn(xfn) {}

    VtValue
    operator()(VtValueRef obj, ErasedXformFnRef xform) const {
        return xFn(obj, voidFn, xform);
    }

    VoidFn voidFn;
    ErasedXfFn xFn;
};

// The singleton registry of transform functions.
struct Vt_ValueXformRegistry
{
    // Look for a TypedXformFunc for objType by xfType and return a pointer to
    // it if found, else return nullptr.
    TypedXformFunc const *
    FindTyped(std::type_index const &objType,
              std::type_index const &xfType) const {
        TfSpinRWMutex::ScopedLock lock(_rwMutex, /*write=*/false);
        // It's safe to return the address of the value here since the container
        // is an unordered_map and we never erase registrations.
        auto iter = _typedXformFns.find({ objType, xfType });
        return iter == _typedXformFns.end() ? nullptr : &iter->second;
    }

    // Look for an ErasedXformFunc for objType by xfType and return a pointer to
    // it if found, else return nullptr.
    ErasedXformFunc const *
    FindErased(std::type_index const &objType) const{
        TfSpinRWMutex::ScopedLock lock(_rwMutex, /*write=*/false);
        // It's safe to return the address of the value here since the container
        // is an unordered_map and we never erase registrations.
        auto iter = _erasedXformFns.find(objType);
        return iter == _erasedXformFns.end() ? nullptr : &iter->second;
    }               

    // Attempt to register a typed transform function.  If one already exists,
    // ignore this registration and return false.
    bool
    RegisterTyped(std::type_index const &objType,
                  std::type_index const &xfType,
                  VoidFn vfn, TypedXfFn cfn) {
        TfSpinRWMutex::ScopedLock lock(_rwMutex);
        auto [iter, inserted] =
            _typedXformFns.emplace(Types { objType, xfType },
                                   TypedXformFunc { vfn, cfn });
        return inserted;
    }

    // Attempt to register a type-erased transform function.  If one already
    // exists, ignore this registration and return false.
    bool
    RegisterErased(std::type_index const &objType,
                   VoidFn vfn, ErasedXfFn cfn) {
        TfSpinRWMutex::ScopedLock lock(_rwMutex);
        auto [iter, inserted] =
            _erasedXformFns.emplace(objType, ErasedXformFunc { vfn, cfn });
        return inserted;
    }

    mutable TfSpinRWMutex _rwMutex;
    std::unordered_map<Types, TypedXformFunc, TfHash> _typedXformFns;
    std::unordered_map<std::type_index, ErasedXformFunc> _erasedXformFns;
};

static Vt_ValueXformRegistry &
_GetMutableXformRegistry()
{
    // Intentionally "leak" this singleton to avoid possible static destruction
    // issues.
    static Vt_ValueXformRegistry *registry = new Vt_ValueXformRegistry;
    return *registry;
}

static Vt_ValueXformRegistry const &
_GetXformRegistry()
{
    return _GetMutableXformRegistry();
}

} // anon

// Return true if there is either a typed or type-erased registration for
// transforming the value viewed by `obj` by `xfType`.
bool
Vt_TransformImpl::Can(VtValueRef obj, std::type_info const &xfType)
{
    auto const &reg = _GetXformRegistry();
    return reg.FindTyped(obj.GetTypeid(), xfType) ||
        reg.FindErased(obj.GetTypeid());
}

// Look up and invoke either a typed or type-erased registration for
// transforming the value viewed by `obj` by `xfType`, preferring a typed
// registration.
VtValue
Vt_TransformImpl::Try(
    VtValueRef obj,
    std::type_info const &xfType,
    void const *xfObj,
    TfFunctionRef<VtValue (VtValueRef)> erasedXfFn)
{
    auto const &reg = _GetXformRegistry();
    VtValue ret;
    if (TypedXformFunc const *fn = reg.FindTyped(obj.GetTypeid(), xfType)) {
        ret = (*fn)(obj, xfObj);
    }
    else if (ErasedXformFunc const *fn = reg.FindErased(obj.GetTypeid())) {
        ret = (*fn)(obj, erasedXfFn);
    }
    return ret;
}

void
Vt_RegisterTypedTransform(std::type_info const &objType,
                          std::type_info const &xfType,
                          VoidFn vfn, TypedXfFn xfn)
{
    if (!_GetMutableXformRegistry().RegisterTyped(objType, xfType, vfn, xfn)) {
        TF_CODING_ERROR("Duplicate typed transform function registered for "
                        "'%s' by '%s' - ignoring registration",
                        ArchGetDemangled(objType).c_str(),
                        ArchGetDemangled(xfType).c_str());
    }
}

void
Vt_RegisterErasedTransform(std::type_info const &objType,
                           VoidFn vfn, ErasedXfFn xfn)
{
    if (!_GetMutableXformRegistry().RegisterErased(objType, vfn, xfn)) {
        TF_CODING_ERROR("Duplicate type-erased transform function registered "
                        "for '%s' - ignoring registration",
                        ArchGetDemangled(objType).c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
