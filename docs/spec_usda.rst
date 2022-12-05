==============================
USDA File Format Specification
==============================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

Copyright |copy| 2022, Pixar Animation Studios,  *version 0.1.0*

.. contents:: :local:

Introduction
=============

The **USDA** file format is a textual representation of a USD scene.

The file format is a UTF-8 encoded format today, though historically (as the name suggests), it was an ASCII format and
retains some ASCII specific limitations. Please refer to the **Notes** section for documentation on limits.

.. warning::
    This is a Work in Progress documentation of the USDA format. It is not guaranteed to
    be an exact description of the format right now. Contributions to improve accuracy are welcome.

    If areas are unspecified, or if a discrepancy occurs, the implementation in the official USD library
    should always take precedence as being the canonical implementation of the format.

    It is **NOT** yet recommended to make your own implementation based on this document
    , and we encourage developers to use the official implementation.

    This document does not describe the composition elements of USD. Implementing this document
    will only allow for reading a single USD layer.

Grammar
=======

USD uses `Yacc <https://www.cs.utexas.edu/users/novak/yaccpaper.htm>`_ and `Bison <https://www.gnu.org/software/bison/>`_
to read USD files.

However for the purposes of this specification, we describe the grammar in a
`Parsing Expression Grammar (PEG) <https://en.wikipedia.org/wiki/Parsing_expression_grammar>`_.

Since it is not the same parser used by USD, there may be slight differences that will be documented in the
**Notes** section.


The grammar is included here in its entirety. Please make sure to read the explanations below for further clarifications
on elements.

.. literalinclude:: _ext/usda.peg
   :language: peg


Layer
-----

Header
^^^^^^

Each USDA layer starts with a header of `#usda 1.0` to specify the file type and version.
Most files are going to be `usda` but this repo also contains a few with `sdf` as the filetype.

This must be the opening characters of the file.

Contents
^^^^^^^^

Layers have an optional top level metadata block to specify layer metadata, and can have multiple top level prim specs
at this level.

A layer is allowed to be empty as long as it contains the header.

Base Components
---------------

Spaces
^^^^^^

USD allows for multiple spaces between components and can be represented as either a single space or a tab.

WhiteSpace and Newlines
^^^^^^^^^^^^^^^^^^^^^^^

In the PEG grammar, there are two types of whitespace definitions.

**whitespace_newline** is meant to represent that there is a hard expectation of a newline.

Newlines are required between metadata items, prim specs and properties since USD has no semicolon style line ending.


Identifier
^^^^^^^^^^

USDA uses ASCII identifiers. These conform to the valid identifier specification for C and Python.

ASCII identifiers may only contain letters, numbers and underscores. They cannot start with a number.

A Work in Progress unicode identifier specification is also provided, awaiting Unicode identifier support in USD.
These must conform to the `unicode identifier standard <https://unicode.org/reports/tr31/>`_.


Values
^^^^^^

Valid value types for metadata and properties are:

* Strings
* Numbers
* Booleans
* References
* Dictionaries
* TimeSamples
* Tuples
* Arrays


Strings
^^^^^^^

Strings may be single quoted, double quoted or triple quoted.

Numbers
^^^^^^^

Numerical values are represented as floats in this grammar.

They may start with an optional negative sign. They cannot start with a positive sign.

Both the left and right hand side of the decimal point are optional as long as at least one is specified.
You may also specify an exponent.

Booleans
^^^^^^^^

Booleans are represented as `true` and `false`

References
^^^^^^^^^^

References are stored as two optional components, though at least one must be provided.

First is the external reference path, which is a path that will be resolved by the AssetResolver. This is stored between
two **@** symbols.

Second is the stage path specifier, that is enclosed in **<>** to represent the path to reference.

Dictionaries
^^^^^^^^^^^^

Dictionaries are stored as properties within curly braces.

TimeSamples
^^^^^^^^^^^

TimeSamples store animation data for a property.

They are stored as comma separated key value pairs in the form **time: value**, where the value is any other value type.


Tuples
^^^^^^
Tuples are fixed size elements to represent a group of values that must be read together, such as a double3 or position value.

Array
^^^^^

Arrays are meant for variable length values and can contain any other value type within them.

Arrays can have several operations done to them, specified by keywords on the metadata or property that holds this array.
Those operations are:

* prepend
* append
* add
* delete

Properties
----------

Properties are children of prim specs that store the following information:

**Specifier Type[] Namespace:Name = Value**

Property definitions require the type and name be provided, while the other elements are optional.

The specifier lets you know if the property is custom and/or uniform.

The type can be singular or specify **[]** if it's an array of that type.

Namespaces are colon separated identifiers for this property.

Metadata
--------

Metadata are elements stored on the layer, prim specs or properties within **( )**

Unlike properties they do not store type information, but are otherwise similar in definition.

Prim Specs
----------

Prim Specs allow for defining a Prim.

Prims are defined as

**Specifier Type "Name" Metadata PrimBlock**

Specifiers can be:
* def
* over
* class

The type is optional and is the schema type that this prim conforms to.

The name is a valid USD identifier.

Metadata is optional, and the prim block holds any of the following:

* Properties
* Other Prim Specs
* Variant Sets

Variant Sets
------------

Variant Sets are defined with the **variantSet** keyword

**variantSet "Name" { ... }**

The Name has to be a valid identifier.

The curly braces contain the variants of this variant set

Variants
^^^^^^^^

Variants are defined as

**"Name" PrimBlock**

Where name can be any valid identifier, and the prim block is the same as the prim block definition for the Prim Spec


Notes
============

As with many text formats, there are specific issues one must be aware of when parsing or generating the file.
While some of these may be resolved in future versions of the format, they're mentioned here for interoperability with
older versions of USD.

PEG Notes
---------

The specific PEG variant used here is compatible with the `Parsimonious <https://github.com/erikrose/parsimonious>`_
Python library, but should be adaptable to PEG libraries in other languages with minor modifications.

This version of the PEG grammar was picked because it was very close to the original PEG grammar specification.

Each parser library might have specific limitations or strengths, and outputs should be tested against the canonical
USD library to verify compatibility.


Type Validation
---------------

The PEG grammar doesn't attempt to try and validate values against the type expected by metadata or properties.
This is left to the runtime to verify that the types are valid, and within range.



Block Opens and Close
---------------------

The opening and closing of each block for Metadata, PrimSpecs or other containers should not share the same line as
the opening or close of another block.

For example you should not close two prim specs on the same line and `}}` will be seen as invalid.