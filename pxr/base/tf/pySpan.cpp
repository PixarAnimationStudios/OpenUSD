//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pySpan.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/arch/demangle.h"

#include <algorithm>
#include <cstring>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _Layout {
    size_t elemSize   = 0;
    char   scalarKind = 0;
    size_t scalarSize = 0;
    std::vector<Py_ssize_t> elemShape;
};

struct _Entry {
    std::vector<Tf_PySpanSourceFn> sources;
    _Layout layout;
};

// Return the kind ('b'ool, 'i'nt, 'u'nsigned, 'f'loat) of a struct-module
// format character, or 0 if unsupported.
char
_GetScalarKind(char fmt)
{
    switch (fmt) {
    case '?':
        return 'b';
    case 'b': case 'h': case 'i': case 'l': case 'q':
        return 'i';
    case 'B': case 'H': case 'I': case 'L': case 'Q':
        return 'u';
    case 'e': case 'f': case 'd':
        return 'f';
    }
    return 0;
}

// The GIL guards the registry.  Registration acquires it, and lookups occur
// during boost.python argument conversion, which holds it.
using _Registry = std::unordered_map<std::type_index, _Entry>;

// Builtin scalar layouts need only a kind, since the itemsize check covers size
// differences.
template <class T>
constexpr char
_GetBuiltinKind()
{
    return std::is_same_v<T, bool>  ? 'b' :
        std::is_floating_point_v<T> ? 'f' :
        std::is_signed_v<T>         ? 'i' : 'u';
}

template <class ... Ts>
void
_AddBuiltinLayouts(_Registry *reg)
{
    ((void)((*reg)[typeid(Ts)].layout =
            _Layout { sizeof(Ts), _GetBuiltinKind<Ts>(), sizeof(Ts), {} }),
     ...);
}

_Registry &
_GetRegistry()
{
    static _Registry *registry = [] {
        _Registry *reg = new _Registry;
        _AddBuiltinLayouts<
            bool, char, signed char, unsigned char,
            short, unsigned short, int, unsigned int,
            long, unsigned long, long long, unsigned long long,
            float, double>(reg);
        return reg;
    }();
    return *registry;
}

_Entry const *
_Find(std::type_info const &elemType)
{
    _Registry const &reg = _GetRegistry();
    auto iter = reg.find(elemType);
    return iter == reg.end() ? nullptr : &iter->second;
}

} // anon

void
Tf_PyRegisterSpanSource(std::type_info const &elemType, Tf_PySpanSourceFn fn)
{
    TfPyLock lock;
    _GetRegistry()[elemType].sources.push_back(fn);
}

void
Tf_PyRegisterSpanBufferLayout(std::type_info const &elemType, size_t elemSize,
                              char scalarFormat,
                              Py_ssize_t const *elemShape, int elemNdim)
{
    size_t numScalars = 1;
    for (int i = 0; i != elemNdim; ++i) {
        numScalars *= elemShape[i];
    }
    if (!numScalars || elemSize % numScalars ||
        !_GetScalarKind(scalarFormat)) {
        TF_CODING_ERROR("Invalid span buffer layout for '%s'",
                        ArchGetDemangled(elemType).c_str());
        return;
    }
    TfPyLock lock;
    _Layout &layout   = _GetRegistry()[elemType].layout;
    layout.elemSize   = elemSize;
    layout.scalarKind = _GetScalarKind(scalarFormat);
    layout.scalarSize = elemSize / numScalars;
    layout.elemShape.assign(elemShape, elemShape + elemNdim);
}

bool
Tf_PyGetSpanFromSources(PyObject *obj, std::type_info const &elemType,
                        bool wantMutable, void **data, size_t *size)
{
    if (_Entry const *entry = _Find(elemType)) {
        for (Tf_PySpanSourceFn fn: entry->sources) {
            if (fn(obj, wantMutable, data, size)) {
                return true;
            }
        }
    }
    return false;
}

bool
Tf_PyGetSpanFromBuffer(PyObject *obj, std::type_info const &elemType,
                       bool wantMutable, Py_buffer *view,
                       void **data, size_t *size)
{
    _Entry const *entry = _Find(elemType);
    if (!entry || !entry->layout.scalarKind || !PyObject_CheckBuffer(obj)) {
        return false;
    }
    _Layout const &layout = entry->layout;

    int flags = PyBUF_FORMAT | PyBUF_C_CONTIGUOUS;
    if (wantMutable) {
        flags |= PyBUF_WRITABLE;
    }
    if (PyObject_GetBuffer(obj, view, flags) != 0) {
        PyErr_Clear();
        return false;
    }

    // Accept native byte order, or explicit little-endian on little-endian
    // hosts; the itemsize check below covers size differences.
    char const *fmt = view->format ? view->format : "B";
    if (*fmt == '@' || *fmt == '=' || (*fmt == '<' && PY_LITTLE_ENDIAN)) {
        ++fmt;
    }

    bool ok =
        fmt[0] && !fmt[1] &&
        _GetScalarKind(fmt[0]) == layout.scalarKind &&
        static_cast<size_t>(view->itemsize) == layout.scalarSize &&
        static_cast<size_t>(view->ndim) == layout.elemShape.size() + 1 &&
        std::equal(layout.elemShape.begin(), layout.elemShape.end(),
                   view->shape + 1);
    if (!ok) {
        PyBuffer_Release(view);
        return false;
    }
    *data = view->buf;
    *size = static_cast<size_t>(view->shape[0]);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
