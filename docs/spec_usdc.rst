==============================
USDC File Format Specification
==============================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

Copyright |copy| 2022, Pixar Animation Studios,  *version 0.9.0*

.. contents:: :local:

Introduction
=============

The **USD Crate** format is binary encoding of the USD scene graph.
This document aims to document the layout of the this format.

.. warning::
    This is a Work in Progress documentation of the USD Crate format. It is not guaranteed to
    be an exact description of the format right now. Contributions to improve accuracy are welcome.

    If areas are unspecified, or if a discrepancy occurs, the implementation in the official USD library
    should always take precedence as being the canonical implementation of the format.

    It is **NOT** yet recommended to make your own implementation based on this document
    , and we encourage developers to use the official implementation.

    This document does not describe the composition elements of USD. Implementing this document
    will only allow for reading a single USD layer.


Versions
---------

This document only aims to document version `0.9.0` of the Crate format and higher.
For older versions of the Crate format, please refer to the USD code implementations.

These older crate files are exceedingly rare outside of Pixar and very early USD adopters.

Version History
^^^^^^^^^^^^^^^

Even though the spec only starts with 0.9.0, it is useful to understand the high level history of versions


* 0.9.0: Added support for the timecode and timecode[] value types.
* 0.8.0: Added support for SdfPayloadListOp values and SdfPayload values with layer offsets.
* 0.7.0: Array sizes written as 64 bit ints.
* 0.6.0: Compressed (scalar) floating point arrays that are either all ints or can be represented efficiently with a lookup table.
* 0.5.0: Compressed (u)int & (u)int64 arrays, arrays no longer store '1' rank.
* 0.4.0: Compressed structural sections.
* 0.3.0: (broken, unused)
* 0.2.0: Added support for prepend and append fields of SdfListOp.
* 0.1.0: Fixed structure layout issue encountered in Windows port.
* 0.0.1: Initial release.


Order of Reads
--------------

The Crate format is designed for minimal parsing on file load.

To achieve this, the general structure of the file is set up in the preamble section below that must be read first.

Value types are parsed on demand from there on out.

Integers
--------

Integers are stored with the least significant bit first to allow for fast loading on `Little Endian <https://en.wikipedia.org/wiki/Endianness>`_ systems.

Index
-----

Many fields reference into an another section to get their value. USD uses unsigned 32 bit integers as an index unless
otherwise specified.


Compression
===========

LZ4 Compression
----------------

Various section and data blocks use `LZ4 Compression <https://github.com/lz4/lz4/blob/dev/doc/lz4_Frame_format.md>`_.

USD vendors the LZ4 library with modifications and can be found `here <https://github.com/PixarAnimationStudios/USD/tree/release/pxr/base/tf/pxrLZ4>`_.
It is currently using `version 1.9.2 <https://github.com/lz4/lz4/releases/tag/v1.9.2>`_ of the library.

.. usdcppdoc:: pxr/base/tf/fastCompression.cpp lz4

Integer Compression
-------------------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp compressed_int


Preamble
========

The Crate format has a preamble that must be read prior to parsing the rest of the file.

This contains all the structural information that is required to identify Prim specifications and
their resulting attributes.


Bootstrap
---------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp bootstrap

Table of Contents
-----------------

The Table of Contents are found at the byte offset given by the bootstrap above.

It consist of named sections , as documented below.

Sections
--------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp section

There are several sections with their own specific internal structures.

Sections should be parsed in the order below since many sections depend on their predecessors.

Tokens
^^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp tokens_section

Strings
^^^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp strings_section

Fields
^^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp fields_section

Field Sets
^^^^^^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp field_sets_section

Paths
^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp paths_section


.. usdcppdoc:: pxr/usd/usd/crateFile.cpp build_compressed_paths

Specs
^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp specs_section

Spec Types are integer backed enums.

Certain types are marked as being **Internal to Pixar**. These are implementations
of Pixar's internal software and are therefore reserved for use. Though they are documented
below for completeness, they should not be used for other purposes.

==================   ===  ========================
Spec Type            Key  Description
==================   ===  ========================
Unknown               0    An unknown type
Attribute             1    Attributes under a Prim Spec
Connection            2    Connections between rel attributes
Expression            3    Scripted Expressions or named plugin expressions (Internal to Pixar)
Mapper                4    Used to modify the value flowing through a connection (Internal to Pixar)
MapperArg             5    Arguments for a mapper (Internal to Pixar)
Prim                  6    A Prim specifier
PseudoRoot            7    The Pseudo Root that exists for all Sdf Layers
Relationship          8    A relationship description
RelationshipTarget    9    The Relationship target
Variant               10   A definition of a specific variant
VariantSet            11   A group of variants]
NumSpecTypes          12   A sentinel to represent the number of types
==================   ===  ========================

Constructing the Layer
======================

Once you have read the Preamble, you can then start constructing the prim hierarchy of this
individual USD Crate file that can act as a layer within a USD composition.

.. warning::

    As noted above, implementation of this document only allows for reading a single USD
    file layer. It does not describe composition however as that is a higher level functionality
    of the USD library.

You should now have a mapping of:

* Prim and Property Paths from the Paths section
* Fields consisting of their associated Token and Value Representation.
* Field Sets consisting of groups of Fields
* Specs that associate paths with Field Sets and a Spec Type

Therefore each node consists of:

* A Path
* A SpecType
* A Field Set

Fields are made up of Value Representations that are deferred for parsing.
See the section below on how to parse the range of Value Representations.

Layer Metadata
--------------

Layer Metadata is derived from the Field Sets on the PseudoRoot.

Common metadata in layers are, but not limited to:

+--------------------+------------+----------------------------------------------------+
| Name               | Type       | Description                                        |
+====================+============+====================================================+
| comment            | string     | Top level comment for this layer                   |
+--------------------+------------+----------------------------------------------------+
| customLayerData    | dictionary | Custom user specified data                         |
+--------------------+------------+----------------------------------------------------+
| defaultPrim        | token      | The prim to use when this layer is referenced      |
+--------------------+------------+----------------------------------------------------+
| documentation      | string     | Top level documentation for this layer             |
+--------------------+------------+----------------------------------------------------+
| metersPerUnit      | double     | The scale unit for this layer                      |
+--------------------+------------+----------------------------------------------------+
| primChildren       | token[]    | A list of top level children to limit the layer to |
+--------------------+------------+----------------------------------------------------+
| timeCodesPerSecond | double     | The time unit used                                 |
+--------------------+------------+----------------------------------------------------+
| upAxis             | token      | The Up Axis of the scene                           |
+--------------------+------------+----------------------------------------------------+

Prims
-----

Prims have the spec type of Prim.

Prims are defined by the field set as below. However prims may have many more fields than this, especially
depending on the schema type of the prim.

+------------------+---------------------+----------------------------------------------------------+
| Name             | Type                | Description                                              |
+==================+=====================+==========================================================+
| specifier        | SdfSpecifier        | Whether this is a def, over or class                     |
+------------------+---------------------+----------------------------------------------------------+
| typeName         | token               | The optional prim Type                                   |
+------------------+---------------------+----------------------------------------------------------+
| kind             | token               | The kind of this prim (component, assembly etc)          |
+------------------+---------------------+----------------------------------------------------------+
| active           | bool                | Whether this prim is active or not                       |
+------------------+---------------------+----------------------------------------------------------+
| apiSchems        | TokenListOp         | The API schemas to apply to this prim                    |
+------------------+---------------------+----------------------------------------------------------+
| variantSelection | VariantSelectionMap | The variants selected for this prim spec                 |
+------------------+---------------------+----------------------------------------------------------+
| references       | ReferencesListOp    | A list of references this prim should use in composition |
+------------------+---------------------+----------------------------------------------------------+
| inherits         | PathListOp          | A list of prims this inherits from                       |
+------------------+---------------------+----------------------------------------------------------+
| properties       | token[]             | List of name of prim properties                          |
+------------------+---------------------+----------------------------------------------------------+
| primChildren     | token[]             | List of child prims to limit to                          |
+------------------+---------------------+----------------------------------------------------------+
| comment          | string              | The comment for this prim                                |
+------------------+---------------------+----------------------------------------------------------+
| documentation    | string              | The documentation for this prim                          |
+------------------+---------------------+----------------------------------------------------------+


Variant Sets
------------

Variant Sets have the VariantSet spec type, and the following fields

+-----------------+---------+-------------------------------------------------+
| Name            | Type    | Description                                     |
+=================+=========+=================================================+
| variantChildren | token[] | A list of the names of the variants in this set |
+-----------------+---------+-------------------------------------------------+

Variants
--------

Variants have the Variant Spec Type. They have the following fields.

Variants encapsulate their own hierarchy for use in composition later.

Properties (Attribute)
----------------------

Properties are of the Attribute SpecType.
They are the individual data members of each composed prim.

The fields for properties are, but are not limited to:

+-----------------+-------------+------------------------------------------+
| Name            | Type        | Description                              |
+=================+=============+==========================================+
| typeName        | token       | The name of the type                     |
+-----------------+-------------+------------------------------------------+
| custom          | bool        | Whether or not this is a custom property |
+-----------------+-------------+------------------------------------------+
| variability     | variability | See the Variability section below        |
+-----------------+-------------+------------------------------------------+
| default         | Value       | The static value                         |
+-----------------+-------------+------------------------------------------+
| timeSamples     | TimeSamples | The TimeSampled animated values          |
+-----------------+-------------+------------------------------------------+
| connectionPaths | PathListOp  | The list of paths this is connected to   |
+-----------------+-------------+------------------------------------------+







Value Representations
=====================

.. usdcppdoc:: pxr/usd/usd/crateFile.h value_rep

Refer to the **Value Types** section below for documentation on different value types.

Inlined Types
-------------

Inlined types are stored directly in the payload of the value representation.

They cannot have the compressed or array bits set.

For types of a single dimension, you can simply cast the data bytes to the given type.

For single dimensioned types like **Vec2i** , elements are stored as signed 8 bit integers and cast to the native type
of the container.

For multidimensional type like **Matrix2D**, the data is stored as the diagonal signed 8 bit integers and then cast to
the native type of the type.

E.g a **Matrix3D** is stored as ::

    1 - -
    - 1 -
    - - 1

Singular Offset Types
---------------------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp unpack_value

Array Offset Types
------------------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp unpack_array_value


Value Types
===========

The following types are valid Value Representation types in USD.

Type Table
----------

The following section enumerates the USD type table used to identify value types.

All type values are documented below the table.

.. usdcratetypes::

Base Types
----------

The following types are foundational types that are used to compose other types.

Bool
^^^^

The **Bool** type is a basic Binary boolean that can be checked by taking any non-zero number as True.

UChar
^^^^^

An unsigned character bit , represented by a single unsigned 8-bit integer.

Int
^^^

A signed 32-bit integer

Uint
^^^^

An unsigned 32-bit integer

Int64
^^^^^

A signed 64-bit integer

UInt64
^^^^^^

An unsigned 64-bit integer

Half
^^^^

A 16-bit floating point data type

Float
^^^^^

A 32-bit floating point data type

Double
^^^^^^

A 64-bit floating point data type

String
^^^^^^

Strings are stored in the file as an index to a token in the Token section.

Token
^^^^^

Tokens are stored in the file as an index to a token in the Token section.

AssetPath
^^^^^^^^^

AssetPaths are stored in the file as an index to a token in the Token section.

Payload
^^^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp payload

ValueBlock
^^^^^^^^^^

A special value type that can be used to explicitly author an
opinion for an attribute's default value or time sample value
that represents having no value. Note that this is different
from not having a value authored.

Value
^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp vtvalue

UnregisteredValue
^^^^^^^^^^^^^^^^^

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp unregistered_value

TimeCode
^^^^^^^^

A single Double that represents an SdfTimeCode


Other Types
===========

When traversing a USD crate file, other data types used by SDF are involved, and described
here. These are not directly referencable as Value Representations but are used by other
container types below.


References
----------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp reference_type

Layer Offset
------------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp layer_offset

Quaternions
-----------

Quaternions are represented by four contiguous elements of a given type.

The first three make up the imaginary coefficient.
The last the element is the real coefficient.

QuatD
^^^^^

A Quaternion using doubles as the core type.

QuatF
^^^^^

A Quaternion using floats as the core type.

QuatH
^^^^^

A Quaternion using halfs as the core type.

Vectors (Mathematical)
----------------------

Vectors are fixed length contiguous arrays of a given type.

Vec2d
^^^^^

A vector with 2 doubles.

Vec2f
^^^^^

A vector with 2 floats.

Vec2h
^^^^^

A vector with 2 halfs.

Vec2i
^^^^^

A vector with 2 32-bit integers.

Vec3d
^^^^^

A vector with 3 doubles.

Vec3f
^^^^^

A vector with 2 floats.

Vec3h
^^^^^

A vector with 3 halfs.

Vec3i
^^^^^

A vector with 3 32-bit integers.

Vec4d
^^^^^

A vector with 4 doubles.

Vec4f
^^^^^

A vector with 4 floats.

Vec4h
^^^^^

A vector with 4 halfs.

Vec4i
^^^^^

A vector with 4 32-bit integers.


Matrix
------

Matrix types are NxN dimensional groups of Doubles. They are defined in a contiguous
row-major order so `matrix[i][j]` refers to row **i** and column **j**.


Identity Matrix values have all cells zeroed out, except for the diagonal from top left ( 0,0 )
to bottom right (N,N) which are set to 1.

Matrix2d
^^^^^^^^

A 2x2 matrix

Matrix3d
^^^^^^^^

A 3x3 matrix

Matrix4d
^^^^^^^^

A 4x4 matrix

Dictionary
----------

A Dictionary is a key:value map of data.

The Value Representation starts with an unsigned 64-bit integer representing the number of elements in the
dictionary.

Keys are strings, stored as an index to a token in the Token section.

Values are a 64-bit unsigned integer representing the offset from the start of the file to a value representation.

List Operations
---------------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp listop

TokenListOp
^^^^^^^^^^^

A ListOp made of Tokens, stored as an array of Indexes referencing the Tokens section.

StringListOp
^^^^^^^^^^^^

A ListOp made of Strings, stored as an array of Indexes referencing the Tokens section.

ReferenceListOp
^^^^^^^^^^^^^^^

A ListOp consisting of References. See the References section below.

IntListOp
^^^^^^^^^

A ListOp consisting of signed 32-bit Integers

Int64ListOp
^^^^^^^^^^^

A ListOp consisting of signed 64-bit Integers

UIntListOp
^^^^^^^^^^

A ListOp consisting of unsigned 32-bit Integers

UInt64ListOp
^^^^^^^^^^^^

A ListOp consisting of unsigned 64-bit Integers

PayloadListOp
^^^^^^^^^^^^^

A ListOp consisting of a Payload. See the Payload section above.

UnregisteredValueListOp
^^^^^^^^^^^^^^^^^^^^^^^

A ListOp consisting of Unregistered Values. See the UnregisteredValue section above.

Vectors (Arrays)
----------------

Vectors are arrays of a given type stored contiguously. These are different in intent from
the mathematical vectors listed above, and their naming reflects the container type in programming
languages like C++.

They always start with a 64-bit unsigned integer reflecting the number of elements,
which is followed by a contiguous array of the specific data type.

PathVector
^^^^^^^^^^

Consists of Index signed integers referencing the Paths section

TokenVector
^^^^^^^^^^^

Consists of Index signed integers referencing the Tokens section

DoubleVector
^^^^^^^^^^^^

Consists of Doubles

LayerOffsetVector
^^^^^^^^^^^^^^^^^

Consists of Layer Offsets

StringVector
^^^^^^^^^^^^

Consists of Index signed integers referencing the Tokens section


Specifier
---------

A 32-bit integer backed enum that defines the possible specifiers for a prim.
These are representations of SdfSpecifier.

==================   ===  ========================
Spec Type            Key  Description
==================   ===  ========================
Def                   0    Defines a concrete prim
Over                  1    Overrides an existing prim
Class                 2    Defines an abstract prim
NumSpecifiers         3    The number of possible specifiers
==================   ===  ========================

Permission
----------

A 32-bit integer backed enum that defines permissions.
Permissions control which layers may refer to or express opinions about a prim.

These are representations of SdfPermission.

==================   ===  ========================
Spec Type            Key  Description
==================   ===  ========================
Public                0    Public prims can be referred to by anything
Private               1    Private prims can only be referred within the local layer stack
NumPermissions        2    The number of possible permissions
==================   ===  ========================

Variability
-----------

A 32-bit integer backed enum that defines variability for an attribute.
Variability indicates whether an attribute is uniform or time varying.

These are representations of SdfVariability.

==================   ===  ========================
Spec Type            Key  Description
==================   ===  ========================
Varying               0    Can be animated
Uniform               1    Can only be static
NumVariability        2    The number of possible variability types
==================   ===  ========================

Variant Selection Map
---------------------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp variant_selection_map

TimeSamples
-----------

.. usdcppdoc:: pxr/usd/usd/crateFile.cpp timesamples
