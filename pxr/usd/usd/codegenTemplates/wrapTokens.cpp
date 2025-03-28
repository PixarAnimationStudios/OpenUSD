//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "{{ libraryPath }}/tokens.h"

{% if useExportAPI %}
{{ namespaceUsing }}

{% endif %}
#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return {{ tokensPrefix }}Tokens->name.GetString(); });

void wrap{{ tokensPrefix }}Tokens()
{
    pxr_boost::python::class_<{{ tokensPrefix }}TokensType, pxr_boost::python::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
{% for token in tokens %}
    _ADD_TOKEN(cls, {{ token.id }});
{% endfor %}
}
