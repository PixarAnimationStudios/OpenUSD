/*!
\page sdf_page_front Sdf : Scene Description Foundations
\if ( PIXAR_MFB_BUILD )
\mainpage Sdf : Scene Description Foundations
\endif

Sdf provides the foundations for serializing scene description to a reference text format, or a multitude of plugin-defined formats.  It also provides the primitive abstractions for interacting with scene description, such as SdfPath, SdfLayer, SdfPrimSpec.

\section sdf_overview Overview

Implements scene description \em layers in USD.  In USD, a complete scene description is composed from partial scene description stored in SdfLayer objects.  The primary unit of scene description within a layer is a \em prim \em spec, represented by the SdfPrimSpec class.  A complete UsdPrim on a stage is a composition of the prim's built-in fallback values and all of the prim spec objects specified in Sdf layers.  (For an overview of prims and stages, see the \ref usd_page_front "Usd library overview".) 

Use methods on an SdfLayer object to export and save a layer to a file, or to load a file from disk.  Scene description files are stored in <c>.usd</c> format (one layer per file, text or binary).  Other features abstracted at the layer level include undo/redo functionality and logging, which can be customized by subclassing SdfLayerStateDelegateBase .

You should primarily work with scene description using the classes in the Usd library.  The UsdStage object not only represents a complete scene; it also knows how each of the partial scene descriptions were combined to form the complete scene.  For example, the UsdStage object has the context to know how the path of a UsdPrim object on the stage relates to the paths of each SdfPrimSpec object in each layer that contributes a partial description to the complete prim.  SdfLayer objects do not have the context to know how they relate to other layers.

\section sdf_layering Layering and Referencing

An SdfPrimSpec object is named and forms a namespace hierarchy with other prims.  Each layer contains one or more root prims, each of which may have a hierarchy of children.  The SdfPath class provides methods to manipulate paths for all of the objects that comprise a layer's scene description.  For example, SdfPath assigns unique paths for each of the objects in a namespace hierarchy; this includes paths to scene description that "lives inside of" particular variants of a VariantSet.

Layers can be combined in several ways.  An SdfLayer can have \e sublayers.  When layering, the SdfPrimSpec objects in an SdfLayer object merge over the prims at the same namespace path in the layer.  An SdfPrimSpec object can also reference another prim within its own layer or a prim from another layer.  When referencing, a prim and its name children merge over the other prim that it references.

Note that layering and referencing means that multiple SdfPrimSpec objects may contribute partial descriptions for the same logical prim.  The full description of the prim in a given scene comes only from combining or \em composing the contributions of each of the SdfPrimSpec objects. 

\section sdf_layers_opinions Layers and Opinions

You can think of the partial scene spec in an SdfLayer object as one \em opinion on an aspect of a complete scene.  Several properties at the layer level determine how or whether the opinion offered at a particular layer is considered when the system composes a complete prim on a stage.  For example, the SdfPrimSpec and SdfPropertySpec classes have an access permission property (SdfPermission) that you can use to specify whether a layer is public or private.

\section sdf_primSpec Prim Spec

There are many different kinds of prims, but at the level of scene description they are all represented by an SdfPrimSpec object.  An SdfPrimSpec object represents a partial description of an individual prim in a scene.  It does not require values for every property it contains.  In addition, the list of properties that an SdfPrimSpec object owns may be sparse.  An SdfPrimSpec object that describes a Cylinder may have a radius but no height, relying on either another SdfPrimSpec object or the prim's fallback definition to provide the height.  Similarly, the list of name children on an SdfPrimSpec object may be sparse.

SdfPrimSpec properties are represented by the SdfPropertySpec class. Property specs also represent partial scene description.  The SdfPropertySpec subclasses represent the basic types of properties that prims can have:

<ul>
	<li>
        <b>SdfAttributeSpec.</b> Represents values, which can be scalar or array-valued.  For example, the \c radius attribute of a Sphere gprim holds a scalar value; the \c points attribute of a Mesh gprim holds an array value. 
	</li>
	<li>
	    <b>SdfRelationshipSpec.</b> Represents a relationship to other prims, attributes, or relationships, such as the \c material:binding relationship that assigns Materials to Gprims.
	</li>
</ul>


\section sdf_plugin_metadata Plugin Metadata

Plugins can extend scene description to store additional plugin-specific metadata by registering custom metadata fields.  Consumers can query and author data for these fields in the same way as the built-in metadata fields in Sdf.  This data will be serialized to and read from layers just like all other scene description.

Plugin metadata fields must be defined in a dictionary called "SdfMetadata" in the "Info" section of the plugin's \c plugInfo.json file.  Each entry in the dictionary defines a single field and has the following syntax:

\code

"<field_name>" : {
    "appliesTo": "<Optional comma-separated list of spec types this field applies to>"
    "default": "<Optional default value for field>",
    "displayGroup": "<Optional name of associated display group>",
    "type": "<Required name indicating field type>",
}

\endcode

For example:

\code
# plugInfo.json
{
    "Plugins": [
        {
            ...
            "Info": {
                "SdfMetadata": {
                    "custom_double": {
                        "type": "double",
                        "appliesTo": "prims"
                    },
                    "custom_string": {
                        "type": "string",
                        "default": "default metadata value"
                    }
                }
            }
            ...
        }
    ]
}
\endcode

\note Plugin metadata in a layer will remain intact and will round-trip properly even if the definition for that metadata is unavailable when the layer is opened.  However, this metadata will not be inspectable using the normal Sdf API.

\subsection sdf_metadata_types Types

The "type" entry for a metadata field must be one of the types listed below.

<table border="0">
    <tr>
        <td valign="top">
            <table>
                <tr><th colspan="2">Scalar Types</th></tr>
                <tr><th>"type" value</th><th>C++ type</th></tr>
                <tr><td>asset</td><td>SdfAssetPath</td></tr>
                <tr><td>bool</td><td>bool</td></tr>
                <tr><td>double</td><td>double</td></tr>
                <tr><td>float</td><td>float</td></tr>
                <tr><td>half</td><td>GfHalf</td></tr>
                <tr><td>int</td><td>int</td></tr>
                <tr><td>int64</td><td>int64_t</td></tr>
                <tr><td>string</td><td>std::string</td></tr>
                <tr><td>timecode</td><td>SdfTimeCode</td></tr>
                <tr><td>token</td><td>TfToken</td></tr>
                <tr><td>uchar</td><td>unsigned char</td></tr>
                <tr><td>uint</td><td>unsigned int</td></tr>
                <tr><td>uint64</td><td>uint64_t</td></tr>
            </table>
        </td>
        <td valign="top">
            <table>
                <tr><th colspan="2">Dimensioned Types</th></tr>
                <tr><th>"type" value</th><th>C++ type</th></tr>
                <tr><td>double2</td><td>GfVec2d</td></tr>
                <tr><td>double3</td><td>GfVec3d</td></tr>
                <tr><td>double4</td><td>GfVec4d</td></tr>
                <tr><td>float2</td><td>GfVec2f</td></tr>
                <tr><td>float3</td><td>GfVec3f</td></tr>
                <tr><td>float4</td><td>GfVec4f</td></tr>
                <tr><td>half2</td><td>GfVec2h</td></tr>
                <tr><td>half3</td><td>GfVec3h</td></tr>
                <tr><td>half4</td><td>GfVec4h</td></tr>
                <tr><td>int2</td><td>GfVec2i</td></tr>
                <tr><td>int3</td><td>GfVec3i</td></tr>
                <tr><td>int4</td><td>GfVec4i</td></tr>
                <tr><td>matrix2d</td><td>GfMatrix2d</td></tr>
                <tr><td>matrix3d</td><td>GfMatrix3d</td></tr>
                <tr><td>matrix4d</td><td>GfMatrix4d</td></tr>
                <tr><td>quatd</td><td>GfQuatd</td></tr>
                <tr><td>quatf</td><td>GfQuatf</td></tr>
                <tr><td>quath</td><td>GfQuath</td></tr>
            </table>
        </td>
        <td valign="top">
            <table>
                <tr><th colspan="2">Array Types</th></tr>
                <tr><th>"type" value</th><th>C++ type</th></tr>
                <tr><td>Element type name + "[]"</td><td>VtArray<Element C++ type></td></tr>
                <tr><td>Any scalar type + "[]"</td><td>VtArray&lt;C++ type&gt; </td></tr>
                <tr><td>&nbsp;&nbsp;ex: string[]</td><td>&nbsp;&nbsp;ex: VtArray&lt;std::string&gt; </td></tr>
                <tr><td>Any dimensioned type + "[]"</td><td>VtArray&lt;C++ type&gt; </td></tr>
                <tr><td>&nbsp;&nbsp;ex: float2[]</td><td>&nbsp;&nbsp;ex: VtArray&lt;GfVec2f&gt; </td></tr>
                <tr><th colspan="2">Dictionary Types</th></tr>
                <tr><th>"type" value</th><th>C++ type</th></tr>
                <tr><td>dictionary</td><td>VtDictionary</td></tr>
                <tr><th colspan="2">List Operation Types</th></tr>
                <tr><th>"type" value</th><th>C++ type</th></tr>
                <tr><td>intlistop</td><td>SdfIntListOp</td></tr>
                <tr><td>int64listop</td><td>SdfInt64ListOp</td></tr>    
                <tr><td>uintlistop</td><td>SdfUIntListOp</td></tr>
                <tr><td>uint64listop</td><td>SdfUInt64ListOp</td></tr>    
                <tr><td>stringlistop</td><td>SdfStringListOp</td></tr>    
                <tr><td>tokenlistop</td><td>SdfTokenListOp</td></tr>    
            </table>
        </td>
    </tr>
</table>

\subsection sdf_metadata_defaults Default Values

If not specified, the default value for plugin metadata fields is the default value for the associated scene description type.  However, plugins may specify default values for each field.  The current implementation allows default values to be specified using a double, an int, a string, or a flat list of one of these types.  For example:

\code

"SdfMetadata": {
    # Defines a field named "double_single_default" with default value of 1.0
    "double_single_default": {
        "type": "double",
        "default": 1.0
    },
    # Defines a field named "double_shaped_default" with default value of VtArray<double>: {0.0, 1.0}
    "double_array_with_default": {
        "type": "double[]",
        "default": [0.0, 1.0]
    }
    # Defines a field named "double2_single_default" with default value of GfVec2d(0.0, 1.0)
    "double2_single_default": {
        "type": "double2",
        "default": [0.0, 1.0]
    }
    # Defines a field named "matrix4d_single_default" with default value of GfMatrix4d(0.5, 1.5, 2.5, 3.5, ...)
    "matrix4d_single_default": {
        "type": "matrix4d"
        "default": [0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5]
    }
}

\endcode

Default values may not be specified for list operation types and dictionary types.

Clients can query the default value for a plugin metadata field by calling SdfSchema::GetFallback.

\subsection sdf_metadata_applies Limiting to Specific Spec Types

By default, a plugin metadata field can be used with any spec type.  However, this can be limited by setting the "appliesTo" field to a comma-separated list of values from the table below:

"appliesTo" value | Spec type
------------------|-----------
layers            | SdfLayer (SdfPseudoRootSpec)
prims             | SdfPrimSpec, SdfVariantSpec
properties        | SdfPropertySpec
attributes        | SdfAttributeSpec
relationships     | SdfRelationshipSpec
variants          | SdfVariantSpec

Note that metadata that "appliesTo" prims also applies to variants, as variants can hold the same scene description as prims.

\subsection sdf_metadata_displayGroup Display Groups

A plugin metadata field may be associated with a display group by specifying a value for the "displayGroup" field.  This is purely an affordance to allow external applications to group metadata fields together for display purposes.  Sdf does not consume this data itself.

\section sdf_file_format_plugins File Format Plugins

See \subpage Sdf_Page_FileFormatPlugin for information about creating file format plugins..

\section sdf_variable_expressions Variable Expressions

See \subpage Sdf_Page_VariableExpressions for information about variable expressions.

*/
