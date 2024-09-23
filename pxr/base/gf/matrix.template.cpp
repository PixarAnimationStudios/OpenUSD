{#
//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#}

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix{{ DIM }}d.h"
#include "pxr/base/gf/matrix{{ DIM }}f.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/tf/type.h"

{% block customIncludes %}
{% endblock customIncludes -%}

#include <float.h>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<{{ MAT }}>();
}

std::ostream&
operator<<(std::ostream& out, const {{ MAT }}& m)
{
    return out
        << "( ("
{% for ROW in range(DIM) %}
        {{ LIST("<< Gf_OstreamHelperP(m[%(ROW)s][%%(i)s])" % {'ROW':ROW}, 
                    sep=" << \", \"\n        ") }}
{% if not loop.last %}
        << "), ("
{% endif %}
{% endfor %}
        << ") )";
}

{% if SCL == 'double' %}
{{ MAT }}::{{ MAT }}(const GfMatrix{{ DIM }}f& m)
{
    Set({{ MATRIX("m[%(i)s][%(j)s]", indent=8) }});
}
{% elif SCL == 'float' %}
{{ MAT }}::{{ MAT }}(const GfMatrix{{ DIM }}d& m)
{
    Set({{ MATRIX("m[%(i)s][%(j)s]", indent=8) }});
}
{% endif %}

{% macro DIAGONAL(dim, i) %}
{{'{'}} 
{%- for ROW in range(dim) %}
    {%- if loop.first %}{{ '{' }}{% else %}{{ '{'|indent(i, true) }}{% endif %}
    {%- for COL in range(dim) %}
        {%- if ROW == COL -%}1.0{% else %}0.0{%- endif %}
        {%- if not loop.last %}, {% endif %}
    {%- endfor %}
    {%- if loop.last %}{{'}'}}{% else %}{{ '},\n'}}{% endif %}
{% endfor %}
{{'};'}}
{%- endmacro -%}

{% for S in SCALARS %}
{{ MAT }}::{{ MAT }}(const std::vector< std::vector<{{ S }}> >& v)
{
    {{ SCL }} m[{{ DIM }}][{{ DIM }}] = {{ DIAGONAL(DIM, 22) }}
    for(size_t row = 0; row < {{ DIM }} && row < v.size(); ++row) {
        for (size_t col = 0; col < {{ DIM }} && col < v[row].size(); ++col) {
            m[row][col] = v[row][col];
        }
    }
    Set(m);
}

{% endfor %}
{% block customConstructors %}
{% endblock customConstructors %}

{{ MAT }} &
{{ MAT }}::SetDiagonal({{ SCL }} s)
{
    {{ MATRIX(fmt = "_mtx[%(i)s][%(j)s] = 0.0;", 
              diagFmt = "_mtx[%(i)s][%(j)s] = s;", 
              sep = "\n    ", indent=4) }}
    return *this;
}

{{ MAT }} &
{{ MAT }}::SetDiagonal(const GfVec{{ SUFFIX }}& v)
{
    {{ MATRIX(fmt = "_mtx[%(i)s][%(j)s] = 0.0;",
              diagFmt = "_mtx[%(i)s][%(j)s] = v[%(i)s];",
              sep = " ", indent = 4) }}
    return *this;
}

{{ SCL }} *
{{ MAT }}::Get({{ SCL }} m[{{ DIM }}][{{ DIM }}]) const
{
    {{ MATRIX("m[%(i)s][%(j)s] = _mtx[%(i)s][%(j)s];", sep="\n    ",
              indent=4) }}
    return &m[0][0];
}

{% for S in SCALARS %}
bool
{{ MAT }}::operator ==(const GfMatrix{{ DIM }}{{ S[0] }} &m) const
{
    return ({{ MATRIX("_mtx[%(i)s][%(j)s] == m._mtx[%(i)s][%(j)s]",
                      sep=" &&\n            ", indent=12) }});
}

{% endfor %}

{{ MAT }}
{{ MAT }}::GetTranspose() const
{
    {{ MAT }} transpose;
    {{ MATRIX("transpose._mtx[%(j)s][%(i)s] = _mtx[%(i)s][%(j)s];",
                sep="\n    ", indent=4) }}

    return transpose;
}

{% block customFunctions %}
{% endblock customFunctions %}

/*
** Scaling
*/
{{ MAT }}&
{{ MAT }}::operator*=(double d)
{
    {{ MATRIX("_mtx[%(i)s][%(j)s] *= d;", sep=" ", indent=4) }}
    return *this;
}

/*
** Addition
*/
{{ MAT }} &
{{ MAT }}::operator+=(const {{ MAT }} &m)
{
    {{ MATRIX("_mtx[%(i)s][%(j)s] += m._mtx[%(i)s][%(j)s];", 
              sep="\n    ", indent=4) }}
    return *this;
}

/*
** Subtraction
*/
{{ MAT }} &
{{ MAT }}::operator-=(const {{ MAT }} &m)
{
    {{ MATRIX("_mtx[%(i)s][%(j)s] -= m._mtx[%(i)s][%(j)s];", 
              sep="\n    ", indent=4) }}
    return *this;
}

/*
** Negation
*/
{{ MAT }}
operator -(const {{ MAT }}& m)
{
    return
        {{ MAT }}({{ MATRIX("-m._mtx[%(i)s][%(j)s]", indent=19) }});
}

{{ MAT }} &
{{ MAT }}::operator*=(const {{ MAT }} &m)
{
    // Save current values before they are overwritten
    {{ MAT }} tmp = *this;

    {% for ROW in range(DIM) -%}
    {% for COL in range(DIM) -%}
    _mtx[{{ ROW }}][{{ COL }}] = {{ LIST("tmp._mtx[%(ROW)s][%%(i)s] * m._mtx[%%(i)s][%(COL)s]" % {"ROW":ROW,"COL":COL},
                sep=" +\n                 ")}};

    {% endfor -%}
    {% endfor -%}

    return *this;
}
{% block customXformFunctions %}
{% endblock customXformFunctions %}


bool
GfIsClose({{ MAT }} const &m1, {{ MAT }} const &m2, double tolerance)
{
    for(size_t row = 0; row < {{ DIM }}; ++row) {
        for(size_t col = 0; col < {{ DIM }}; ++col) {
            if(!GfIsClose(m1[row][col], m2[row][col], tolerance))
                return false;
        }
    }
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
