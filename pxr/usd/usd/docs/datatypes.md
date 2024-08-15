# Basic Datatypes for Scene Description Provided by Sdf {#Usd_Page_Datatypes}

## Attribute value types {#Usd_Datatypes}

USD supports a variety of data types for attribute values. 
These types are encoded in an SdfValueTypeName object that corresponds to an 
underlying C++ type and is what a user would receive from a call to 
UsdAttribute::GetTypeName. Methods for looking up these objects are provided 
by \c SdfSchema and they're enumerated in the 
object \c SdfValueTypeNames -- \sa SdfSchema

For example, to create a custom, Matrix4d-valued attribute on prim \em foo:
\code
if (UsdAttribute mat = foo.CreateAttribute(TfToken("myMatrix"), SdfValueTypeNames->Matrix4d)){
   mat.Set(GfMatrix4d(1));  // Assign identity matrix
} else {
   // error creating the attribute
}
\endcode
In python:
\code{.py}
mat = foo.CreateAttribute("myMatrix", Sdf.ValueTypeNames.Matrix4d)
if mat:
   mat.Set(Gf.Matrix4d(1))  # Assign identity matrix
else:
   # error creating the attribute
\endcode

## Basic data types {#Usd_Basic_Datatypes}

This table lists the fundamental data types supported by USD.

| Value type token | C++ type      | Description                                      |
|:-----------------|:--------------|:-------------------------------------------------|
| bool             | bool          |                                                  |
| uchar            | uint8_t       | 8  bit unsigned integer                          |
| int              | int32_t       | 32 bit signed   integer                          |
| uint             | uint32_t      | 32 bit unsigned integer                          |
| int64            | int64_t       | 64 bit signed   integer                          |
| uint64           | uint64_t      | 64 bit unsigned integer                          |
| half             | GfHalf        | 16 bit floating point                            |
| float            | float         | 32 bit floating point                            |
| double           | double        | 64 bit floating point                            |
| timecode         | SdfTimeCode   | double representing a resolvable time            |
| string           | std::string   | stl string                                       |
| token            | TfToken       | interned string with fast comparison and hashing |
| asset            | SdfAssetPath  | represents a resolvable path to another asset    |
| opaque           | SdfOpaqueValue| represents a value that can't be serialized      |
| matrix2d         | GfMatrix2d    | 2x2 matrix of doubles                            |
| matrix3d         | GfMatrix3d    | 3x3 matrix of doubles                            |
| matrix4d         | GfMatrix4d    | 4x4 matrix of doubles                            |
| quatd            | GfQuatd       | double-precision quaternion                      |
| quatf            | GfQuatf       | single-precision quaternion                      |
| quath            | GfQuath       | half-precision quaternion                        |
| double2          | GfVec2d       | vector of 2 doubles                              |
| float2           | GfVec2f       | vector of 2 floats                               |
| half2            | GfVec2h       | vector of 2 half's                               |
| int2             | GfVec2i       | vector of 2 ints                                 |
| double3          | GfVec3d       | vector of 3 doubles                              |
| float3           | GfVec3f       | vector of 3 floats                               |
| half3            | GfVec3h       | vector of 3 half's                               |
| int3             | GfVec3i       | vector of 3 ints                                 |
| double4          | GfVec4d       | vector of 4 doubles                              |
| float4           | GfVec4f       | vector of 4 floats                               |
| half4            | GfVec4h       | vector of 4 half's                               |
| int4             | GfVec4i       | vector of 4 ints                                 |

Note that opaque-valued attributes cannot be authored, and are used for cases
where the value can't be represented in USD, such as shader outputs. 

## Roles {#Usd_Roles}

Value types may also be specified by one of a set of special type names. 
These names correspond to the basic data types listed above but provide 
extra semantics about how the data should be interpreted. For instance, 
a value of type "frame4d" is a matrix4d, but represents a coordinate frame.

These type names are grouped, in several instances, by a "role" name, such that 
all types with the same role have the same semantics. All of these "semantic 
types" are also provided in SdfValueTypeNames. To get the "semantic type" role 
name from the type token, use SdfGetRoleNameForValueTypeName().

The following table lists the type tokens for these roles, the corresponding
base type token, which can be matched to the C++ type in the table above, 
and the role name and meaning.

| Value type token  | Base type | Role name         | Role Meaning                                           |
|:------------------|:----------|:------------------|:-------------------------------------------------------|
| point3d           | double3   | Point             | transform as a position                                |
| point3f           | float3    | Point             | transform as a position                                |
| point3h           | half3     | Point             | transform as a position                                |
| normal3d          | double    | Normal            | transform as a normal                                  |
| normal3f          | float3    | Normal            | transform as a normal                                  |
| normal3h          | half3     | Normal            | transform as a normal                                  |
| vector3d          | double3   | Vector            | transform as a direction                               |
| vector3f          | float3    | Vector            | transform as a direction                               |
| vector3h          | half3     | Vector            | transform as a direction                               |
| color3d           | double3   | Color             | energy-linear RGB                                      |
| color3f           | float3    | Color             | energy-linear RGB                                      |
| color3h           | half3     | Color             | energy-linear RGB                                      |
| color4d           | double4   | Color             | energy-linear RGBA, <b>not</b> pre-alpha multiplied    |
| color4f           | float4    | Color             | energy-linear RGBA, <b>not</b> pre-alpha multiplied    |
| color4h           | half4     | Color             | energy-linear RGBA, <b>not</b> pre-alpha multiplied    |
| frame4d           | matrix4d  | Frame             | defines a coordinate frame                             |
| texCoord2d        | double2   | TextureCoordinate | 2D uv texture coordinate                               |
| texCoord2f        | float2    | TextureCoordinate | 2D uv texture coordinate                               |
| texCoord2h        | half2     | TextureCoordinate | 2D uv texture coordinate                               |
| texCoord3d        | double3   | TextureCoordinate | 3D uvw texture coordinate                              |
| texCoord3f        | float3    | TextureCoordinate | 3D uvw texture coordinate                              |
| texCoord3h        | half3     | TextureCoordinate | 3D uvw texture coordinate                              |
| group             | opaque    | Group             | used as a grouping mechanism for namespaced properties |

A Group attribute is an opaque attribute used to represent a group of other 
properties. It behaves like a connectable/targetable property namespace.

## Array data types {#Usd_Array_Datatypes}

USD also supports arrays of the above data types. The value type 
name for these arrays is simply the name of the underlying value type 
and "[]", e.g. "bool[]", "float[]", etc., and are also provided by 
SdfValueTypeNames with a "Array" suffix on the basic datatype 
(e.g. SdfValueTypeNames->FloatArray provides "float[]"). 
The corresponding C++ type is VtArray, e.g. VtArray<bool>, VtArray<float>, etc.

## Dictionary-valued Metadata {#Usd_Dictionary_Type}

Metadata in USD (See \ref Usd_OM_Metadata) can take on several other datatypes.  
Most of these are highly-specialized, pertaining to composition, 
like SdfListOp<SdfReference>, but we also provide one very versatile 
datatype <b>dictionary</b> <em>that can only be assumed by metadata</em>, 
such as \ref UsdObject::GetCustomData() "customData". A dictionary's entries 
are keyed by string, and can take on any other scene description value type, 
\em including dictionary, allowing for nested dictionaries.

Dictionaries have special value-resolution semantics, in that each field 
resolves independently of all others, allowing dictionaries to be sparsely 
described and overridden in many layers.  This behavior, which we have 
found highly desirable, also makes dictionaries more expensive to fully 
compose than all other value types (except those that 
are \ref Usd_OM_ListOps "list-edited").  It is for this reason that we 
disallow dictionary as a value type for attributes: it would eliminate 
the extremely important performance characteristic of attribute 
value resolution that "strongest opinion wins".

You can use the builtin customData for your truly ad-hoc user-data needs, 
but you can also add new dictionary-valued metadata by defining it in a 
module plugInfo.json file - 
see \ref sdf_plugin_metadata for details on how to do so.
