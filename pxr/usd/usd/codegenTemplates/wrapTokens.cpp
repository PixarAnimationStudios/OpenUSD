//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pySafePython.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include "{{ libraryPath }}/tokens.h"

namespace nb = nanobind;

{% if useExportAPI %}
{{ namespaceUsing }}

{% endif %}
#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, [](nb::handle) { return {{ tokensPrefix }}Tokens->name.GetString(); });

void wrap{{ tokensPrefix }}Tokens(nb::module_ &m)
{
    nb::class_<{{ tokensPrefix }}TokensType> cls(m, "Tokens");
{% for token in tokens %}
    _ADD_TOKEN(cls, {{ token.id }});
{% endfor %}
}
