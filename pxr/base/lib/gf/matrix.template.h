{#
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
#}
#ifndef GF_{{ UPPER(MAT)[2:] }}_H
#define GF_{{ UPPER(MAT)[2:] }}_H

#include "pxr/base/gf/matrixData.h"
#include "pxr/base/gf/vec{{ SUFFIX }}.h"
#include "pxr/base/gf/traits.h"
{% block includes %}
{% endblock %}

#include <boost/functional/hash.hpp>

#include <iosfwd>
#include <vector>

/// \file matrix{{ SUFFIX }}.h
/// \ingroup group_gf_LinearAlgebra
///

template <>
struct GfIsGfMatrix<class {{ MAT }}> { static const bool value = true; };

{% for S in SCALARS %}
class GfMatrix{{ DIM }}{{ S[0] }};
{% endfor %}
{% block forwardDeclarations %}
{% endblock %}

/// \class {{ MAT }} matrix{{ SUFFIX }}.h "pxr/base/gf/matrix{{ SUFFIX }}.h"
/// \ingroup group_gf_LinearAlgebra
/// \brief Stores a {{ DIM }}x{{ DIM }} matrix of \c {{ SCL }} elements. A basic type.
///
/// Matrices are defined to be in row-major order, so <c>matrix[i][j]</c> 
/// indexes the element in the \e i th row and the \e j th column.
///
{% block classDocs %}{% endblock %}
class {{ MAT }}
{
public:
    typedef {{ SCL }} ScalarType;

    static const size_t numRows = {{ DIM }};
    static const size_t numColumns = {{ DIM }};

    /// Default constructor. Leaves the matrix component values
    /// undefined.
    {{ MAT }}() {}

    /// Constructor. Initializes the matrix from {{ DIM*DIM }} independent
    /// \c {{ SCL }} values, specified in row-major order. For example,
    /// parameter \e m10 specifies the value in row 1 and column 0.
    {{ MAT }}({{ MATRIX("%(SCL)s m%%(i)s%%(j)s" % { 'SCL':SCL }, indent=15) }}) {
        Set({{ MATRIX("m%(i)s%(j)s", indent=12) }});
    }

    /// Constructor. Initializes the matrix from a {{ DIM }}x{{ DIM }} array of
    /// \c {{ SCL }} values, specified in row-major order.
    {{ MAT }}(const {{ SCL }} m[{{ DIM }}][{{ DIM }}]) {
        Set(m);
    }

    /// Constructor. Explicitly initializes the matrix to \e s times
    /// the identity matrix.
    explicit {{ MAT }}({{ SCL }} s) {
        SetDiagonal(s);
    }
{% block customDiagonalConstructors %}

{% endblock customDiagonalConstructors %}
    /// Constructor. Explicitly initializes the matrix to diagonal form,
    /// with the \e i th element on the diagonal set to <c>v[i]</c>.
    explicit {{ MAT }}(const GfVec{{ SUFFIX }}& v) {
        SetDiagonal(v);
    }

{% for S in SCALARS %}
    /// Constructor.  Initialize the matrix from a vector of vectors of {{ S }}.
    /// The vector is expected to be {{ DIM }}x{{ DIM }}.  If it is too big, only the first
    /// {{ DIM }} rows and/or columns will be used.  If it is too small, uninitialized
    /// elements will be filled in with the corresponding elements from an
    /// identity matrix.
    ///
    explicit {{ MAT }}(const std::vector< std::vector<{{ S }}> >& v);

{% endfor %}
{% block customConstructors %}
{% endblock %}
{% if SCL == 'double' %}
    //!
    // This explicit constructor converts a "float" matrix to a "double" matrix.
    explicit {{ MAT }}(const class GfMatrix{{ DIM }}f& m);

{% endif %}
{% if SCL == 'float' %}
    //!
    // This explicit constructor converts a "double" matrix to a "float" matrix.
    explicit {{ MAT }}(const class GfMatrix{{ DIM }}d& m);

{% endif %}
    /// Sets a row of the matrix from a Vec{{ DIM }}.
    void SetRow(int i, const GfVec{{ SUFFIX }} & v) {
        {{ LIST("_mtx[i][%(i)s] = v[%(i)s];", sep='\n        ') }}
    }

    /// Sets a column of the matrix from a Vec{{ DIM }}.
    void SetColumn(int i, const GfVec{{ SUFFIX }} & v) {
        {{ LIST("_mtx[%(i)s][i] = v[%(i)s];", sep='\n        ') }}
    }

    /// Gets a row of the matrix as a Vec{{ DIM }}.
    GfVec{{ SUFFIX }} GetRow(int i) const {
        return GfVec{{ SUFFIX }}({{ LIST("_mtx[i][%(i)s]") }});
    }

    /// Gets a column of the matrix as a Vec{{ DIM }}.
    GfVec{{ SUFFIX }} GetColumn(int i) const {
        return GfVec{{ SUFFIX }}({{ LIST("_mtx[%(i)s][i]") }});
    }

    /// \brief Sets the matrix from {{ DIM*DIM }} independent \c {{ SCL }} values, specified
    /// in row-major order. For example, parameter \e m10 specifies the
    /// value in row 1 and column 0.
    {{ MAT }}& Set({{ MATRIX("%(SCL)s m%%(i)s%%(j)s" % { 'SCL':SCL }, indent=20) }}) {
        {{ MATRIX("_mtx[%(i)s][%(j)s] = m%(i)s%(j)s;", sep=" ", indent=8) }}
        return *this;
    }

    /// \brief Sets the matrix from a {{ DIM }}x{{ DIM }} array of \c {{ SCL }} values, specified
    /// in row-major order.
    {{ MAT }}& Set(const {{ SCL }} m[{{ DIM }}][{{ DIM }}]) {
        {{ MATRIX("_mtx[%(i)s][%(j)s] = m[%(i)s][%(j)s];", sep="\n        ") }}
        return *this;
    }

    /// Sets the matrix to the identity matrix.
    {{ MAT }}& SetIdentity() {
        return SetDiagonal(1);
    }

    /// Sets the matrix to zero.
    {{ MAT }}& SetZero() {
        return SetDiagonal(0);
    }

    /// Sets the matrix to \e s times the identity matrix.
    {{ MAT }}& SetDiagonal({{ SCL }} s);

    /// Sets the matrix to have diagonal (<c>{{ LIST("v[%(i)s]") }}</c>).
    {{ MAT }}& SetDiagonal(const GfVec{{ SUFFIX }}&);

    /// Fills a {{ DIM }}x{{ DIM }} array of \c {{ SCL }} values with the values in
    /// the matrix, specified in row-major order.
    {{ SCL }}* Get({{ SCL }} m[{{ DIM }}][{{ DIM }}]);

    /// Returns vector components as an array of \c {{ SCL }} values.
    {{ SCL }}* GetArray()  {
        return _mtx.GetData();
    }

    /// Returns vector components as a const array of \c {{ SCL }} values.
    const {{ SCL }}* GetArray() const {
        return _mtx.GetData();
    }
    
    /// Accesses an indexed row \e i of the matrix as an array of {{ DIM }} \c {{ SCL }}
    /// values so that standard indexing (such as <c>m[0][1]</c>) works
    /// correctly.
    {{ SCL }}* operator [](int i) { return _mtx[i]; }

    /// Accesses an indexed row \e i of the matrix as an array of {{ DIM }} \c {{ SCL }}
    /// values so that standard indexing (such as <c>m[0][1]</c>) works
    /// correctly.
    const {{ SCL }}* operator [](int i) const { return _mtx[i]; }

    /// Hash.
    friend inline size_t hash_value({{ MAT }} const &m) {
        int nElems = {{ DIM }} * {{ DIM }};
        size_t h = 0;
        const {{ SCL }} *p = m.GetArray();
        while (nElems--)
            boost::hash_combine(h, *p++);
        return h;
    }        

    /// Tests for element-wise matrix equality. All elements must match
    /// exactly for matrices to be considered equal.
    bool operator ==(const GfMatrix{{ DIM }}d& m) const;

    /// Tests for element-wise matrix equality. All elements must match
    /// exactly for matrices to be considered equal.
    bool operator ==(const GfMatrix{{ DIM }}f& m) const;

    /// Tests for element-wise matrix inequality. All elements must match
    /// exactly for matrices to be considered equal.
    bool operator !=(const GfMatrix{{ DIM }}d& m) const {
        return !(*this == m);
    }

    /// Tests for element-wise matrix inequality. All elements must match
    /// exactly for matrices to be considered equal.
    bool operator !=(const GfMatrix{{ DIM }}f& m) const {
        return !(*this == m);
    }

    /// Returns the transpose of the matrix.
    {{ MAT }} GetTranspose() const;

    /// Returns the inverse of the matrix, or FLT_MAX * SetIdentity() if the
    /// matrix is singular. (FLT_MAX is the largest value a \c float can have, 
    /// as defined by the system.) The matrix is considered singular if the 
    /// determinant is less than or equal to the optional parameter \e eps.
    /// If \e det is non-null, <c>*det</c> is set to the determinant.
    {{ MAT }} GetInverse(double* det = NULL, double eps = 0) const;

    /// Returns the determinant of the matrix.
    double GetDeterminant() const;

{% block customFunctions %}
{% endblock customFunctions %}

    /// Post-multiplies matrix \e m into this matrix.
    {{ MAT }}& operator *=(const {{ MAT }}& m);

    /// Multiplies the matrix by a {{ SCL }}.
    {{ MAT }}& operator *=(double);

    ///
    // Returns the product of a matrix and a {{ SCL }}.
    friend {{ MAT }} operator *(const {{ MAT }}& m1, double d)
    {
	{{ MAT }} m = m1;
	return m *= d;
    }

    ///
    // Returns the product of a matrix and a {{ SCL }}.
    friend {{ MAT }} operator *(double d, const {{ MAT }}& m)
    {
        return m * d;
    }

    /// Adds matrix \e m to this matrix.
    {{ MAT }}& operator +=(const {{ MAT }}& m);

    /// Subtracts matrix \e m from this matrix.
    {{ MAT }}& operator -=(const {{ MAT }}& m);

    /// Returns the unary negation of matrix \e m.
    friend {{ MAT }} operator -(const {{ MAT }}& m);

    /// Adds matrix \e m2 to \e m1
    friend {{ MAT }} operator +(const {{ MAT }}& m1, const {{ MAT }}& m2)
    {
        {{ MAT }} tmp(m1);
        tmp += m2;
        return tmp;
    }

    /// Subtracts matrix \e m2 from \e m1
    friend {{ MAT }} operator -(const {{ MAT }}& m1, const {{ MAT }}& m2)
    {
        {{ MAT }} tmp(m1);
        tmp -= m2;
        return tmp;
    }
    
    /// Multiplies matrix \e m1 by \e m2
    friend {{ MAT }} operator *(const {{ MAT }}& m1, const {{ MAT }}& m2)
    {
        {{ MAT }} tmp(m1);
        tmp *= m2;
        return tmp;
    }

    /// Divides matrix \e m1 by \e m2 (that is, <c>m1 * inv(m2)</c>).
    friend {{ MAT }} operator /(const {{ MAT }}& m1, const {{ MAT }}& m2)
    {
        return(m1 * m2.GetInverse());
    }

    /// Returns the product of a matrix \e m and a column vector \e vec.
    friend inline GfVec{{ SUFFIX }} operator *(const {{ MAT }}& m, const GfVec{{ SUFFIX }}& vec) {
        return GfVec{{ SUFFIX }}(
{%- for ROW in range(DIM) -%}
            {{ LIST("vec[%%(i)s] * m._mtx[%(ROW)s][%%(i)s]" % {'ROW':ROW}, sep=" + ") }}
            {%- if not loop.last %}{{ ",\n                       " }}{% endif %}
{% endfor %});
    }

    /// Returns the product of row vector \e vec and a matrix \e m.
    friend inline GfVec{{ SUFFIX }} operator *(const GfVec{{ SUFFIX }} &vec, const {{ MAT }}& m) {
        return GfVec{{ SUFFIX }}(
{%- for COL in range(DIM) -%}
            {{ LIST("vec[%%(i)s] * m._mtx[%%(i)s][%(COL)s]" % {'COL':COL}, sep=" + ") }}
            {%- if not loop.last %}{{ ",\n                       " }}{% endif %}
{% endfor %});
    }

{% if SCL == 'double' %}
    /// Returns the product of a matrix \e m and a column vector \e vec.
    /// Note that the return type is a \c GfVec{{ DIM }}f.
    friend GfVec{{ DIM }}f operator *(const {{ MAT }}& m, const GfVec{{ DIM }}f& vec);

    /// Returns the product of row vector \e vec and a matrix \e m.
    /// Note that the return type is a \c GfVec{{ DIM }}f.
    friend GfVec{{ DIM }}f operator *(const GfVec{{ DIM }}f &vec, const {{ MAT }}& m);

{% endif %}
{% block customXformFunctions %}
{% endblock customXformFunctions %}

private:
    /// Matrix storage, in row-major order.
    GfMatrixData<{{ SCL }}, {{ DIM }}, {{ DIM }}> _mtx;

    // Friend declarations
{% if SCL == 'float' %}
    friend class GfMatrix{{ DIM }}d;
{% endif %}
{% if SCL == 'double' %}
    friend class GfMatrix{{ DIM }}f;
{% endif %}
};

/// Output a {{ MAT }}
/// \ingroup group_gf_DebuggingOutput
std::ostream& operator<<(std::ostream &, {{ MAT }} const &);

#endif // GF_{{ UPPER(MAT)[2:] }}_H

