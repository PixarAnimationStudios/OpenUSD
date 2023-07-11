.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst   

=============================
Generating New Schema Classes
=============================

.. note:: Configure Environment
   
   There are a couple of prerequisites to dealing with schema generation in 
   USD.
   
       * usdGenSchema, our tool for generating C++ classes from a schema.usda
         file requires the `jinja2 template substitution module
         <https://github.com/mitsuhiko/jinja2>`_ and argparse modules be
         installed and available in your python syspath.
   
       * Ensure that you have setup your python environment properly, so it can
         find USD python modules. We'll refer to the install location of your 
         USD build with **USD_INSTALL_ROOT** , this is determined in the build 
         with the cmake flag, **-DCMAKE_INSTALL_PREFIX** .  

         .. list-table::
            :header-rows: 1

            * - Environment Variable
              - Meaning
              - Value
            * - PYTHONPATH
              - This is a path list which Python uses to find modules.
              - $PYTHONPATH:**USD_INSTALL_ROOT/lib/python/**

   For more information see our page on `Advanced Build Configuration <https://github.com/PixarAnimationStudios/USD/blob/release/BUILDING.md>`_ .

The files used in this tutorial are available in
USD/extras/usd/examples/usdSchemaExamples/. The objective of this tutorial is 
to introduce the user to various types of schema classes and to provide
instructions for generating, compiling and using them. For a detailed
description of schema classes and the various options that usdGenSchema 
provides for customizing them, see `API documentation for generating schemas
<api/_usd__page__generating_schemas.html>`_

What is a Schema Class?
#######################

A schema class is simply a container of a UsdPrim that provides a layer of
specific, named API atop the underlying scene graph. USD provides a code
generator script called 'usdGenSchema' for creating new schema classes. For 
more info on the script and all of the options it provides see the references 
section below.

UsdModelAPI, UsdGeomImageable, UsdGeomMesh etc. are examples of schema classes
generated using the script. There are other schemas in USD/pxr/usd/lib/usd 
under usdGeom, usdRi, usdShade and more!

Types of Schema Classes
#######################

*copied from the* `API documentation 
<api/_usd__page__generating_schemas.html>`_...

Schema classes are classified into the following two types:

    * **IsA schema**

      An IsA schema can impart a typeName to a prim in
      addition to providing an interface to a prim's qualities. Every IsA
      schema must derive from the core class UsdTyped, which is the base class
      for all typed schemas. Furthermore, an IsA schema can be concrete or
      non-concrete. An IsA schema will be concrete (or instantiable) if its
      schema declaration provides both a name for the schema (in quotes) and a
      typeName in the schema.usda file in which it is defined. A non-concrete
      (abstract) IsA schema provides only a name for the schema, and hence
      cannot be instantiated; non-concrete schemas exist to serve as
      super-classes for related sets of concrete IsA schemas. UsdGeomImageable
      is an example of a non-concrete IsA schema. UsdGeomScope is an example of
      a concrete, typed IsA schema.

    * **API schema** 

      An API schema provides an interface to a prim's 
      qualities, but does not specify a typeName for the underlying prim. 
      The prim's qualities include its inheritance structure, attributes, 
      relationships etc. Since it cannot provide a typeName, an API schema is 
      considered to be non-concrete. As a convention, the C++/python class 
      name for an API schema must end with "API".  In core USD, UsdModelAPI is 
      an example of an API schema; UsdRiMaterialAPI is an example from our 
      RenderMan schema module, which adds/extracts RenderMan-specific shading 
      information from a generic UsdShadeMaterial-typed prim.  Also by 
      convention (with which usdGenSchema can help), the properties that 
      "belong" to an API schema are typically namespaced with the base-name of 
      the schema, camelCased. For example, 
      UsdRiMaterialAPI::CreateSurfaceAttr() will create an attribute named 
      outputs:ri:surface. API schemas are classified into the following two 
      sub-types: 

        * **Non-applied API schemas** 

          If an API schema only provides an 
          interface to certain core bits of metadata (like UsdModelAPI, which 
          sets model kind and UsdClipsAPI, which sets value-clips related 
          metadata) or if there is no use of recording the application of the 
          API schema on a prim (for the purpose of interchange), we make it a 
          non-applied API schema. Examples of non-applied API schemas include 
          UsdModelAPI, UsdClipsAPI, UsdShadeConnectableAPI and 
          UsdGeomPrimvarsAPI. Typically, non-applied API schemas can apply to 
          any prim-type (eg, UsdClipsAPI) or to a known fixed set of prim 
          types, like in the case of UsdShadeConnectableAPI which is only 
          applicable to types that implement connectability behaviors such as 
          shaders and lights.

        * **Applied API Schemas** 

          If there is a need to record and discover 
          whether an API schema has been applied to a prim, we make it an 
          applied API schema. An applied schema will impart its properties as 
          additional built-in properties on the prim. A public Apply() method 
          is auto-generated for applied API schemas. Once an API schema has 
          been applied to a prim, prim.HasAPI<APISchemaType>() will return 
          true. An applied API schema must be applied to a prim via a call to 
          the Apply() method, for the schema object to evaluate to true when 
          converted to a bool using the explicit bool conversion operator.
          Examples of applied API schemas include UsdCollectionAPI, 
          UsdGeomModelAPI and UsdGeomMotionAPI. All API schemas must inherit 
          from UsdAPISchemaBase directly and cannot inherit from other 
          schemas. Applied API schemas are further classified into the 
          following two categories:

            * **Single-Apply API Schemas** 

              Applied API schemas that can only be applied as a single 
              instance to a prim.  Even though API schemas cannot 
              inherit from other API schemas, single-apply API schemas can 
              include other API schemas as "built-ins". Examples of 
              single-apply API schemas include UsdGeomModelAPI and 
              UsdGeomMotionAPI. 

            * **Multiple-Apply API Schemas** 

              Applied API schemas that can 
              be applied multiple times on the same prim with different 
              *instance names*. For example, UsdCollectionAPI, which must be 
              applied once per collection owned by a prim. Properties 
              instantiated by this API schema are prefixed with the namespace 
              prefix of the schema followed by the instance name. There is 
              specific metadata one authors in schema.usda to identify the 
              prefix for multi-apply schemas, and the properties for a 
              multi-apply schema should be listed without any prefix.

The definition of an IsA schema is published, at runtime, into an 
introspectable "schema definition registry", which is consulted by core Usd 
when performing property value resolution (i.e. retrieving a property's value 
at a given UsdTime). This allows IsA schemas to provide *fallback values* for 
their properties, i.e., a value that the property will possess, even when none 
has been authored.

The author of an API schema must decide on the type of API schema at the time 
of its creation by setting token-valued customData entry 'apiSchemaType' in the
schema definition. It can be set to one of 'nonApplied', 'singleApply' or
'multipleApply'. When unspecified, the fallback *apiSchemaType* for an API
schema is 'singleApply'. An API schema can only inherit from another compatible
API schema with matching *customData["apiSchemaType"]* or from "
*/APISchemaBase"* directly. This is enforced by the schema code generation
script 'usdGenSchema'. API schemas that are *singleApply* or *multipleApply*
also populate the UsdSchemaRegistry at runtime, and when properly applied to a
prim, are taken together with the prim's IsA type to produce the prim's
:usdcpp:`UsdPrimTypeInfo` which is a complete "type signature", used as a key
to retrieve the prim's :usdcpp:`UsdPrimDefinition`, which is what USD actually
uses to discover fallback values (among other things) during property value
resolution, thus by applying an API schema, one is able to add builtin
properties-with-fallbacks to any prim.

Although we will not cover it in this tutorial, you can add (and we have for
many of the core schemas) custom methods to any generated IsA or API schema 
that **will be preserved** if/when you need to re-run usdGenSchema. Custom 
methods are handy for providing computations, or authoring operations that 
require coordinated authoring of more than a single value on a property at 
once.

Schema Generation Prerequisites
###############################

The schema generation script 'usdGenSchema' is driven by a USD layer (typically
named schema.usda). Every schema.usda layer must meet the following 
requirements in order for generated code to compile and work with USD core 
successfully.

    * Must specify the libraryName as layer metadata.

    * usd/schema.usda must exist in the layer stack, not necessarily as a 
      direct subLayer.

    * Schema typenames must be unique across all libraries.

    * Attribute names and tokens must be camelCased valid identifiers. Refer to
      detailed documentation for generating schemas for exceptions to this rule
      and usage of "useLiteralIdentifier" metadata.

In our examples, we'll use the following as the base layer (or starting point)
for creating new schema classes in order to satisfy the first two requirements
above.

.. code-block:: usda

    #usda 1.0
    (
        """ This file describes an example schema for code generation using
            usdGenSchema.
        """
        subLayers = [
            # To refer to schema types defined in schema.usda files from other 
            # libraries, simply add comma-separated lines of the form 
            # @<library name>/schema.usda@. In this example, we're referring 
            # to schema types from 'usd'. If you were adding sub-classes of 
            # UsdGeom schema types, you would use usdGeom/schema.usda instead.
            @usd/schema.usda@
        ]
    )
   
    over "GLOBAL" (
        customData = {
            string libraryName       = "usdSchemaExamples"
            string libraryPath       = "./"
            string libraryPrefix     = "UsdSchemaExamples"
        }
    ) { 
    }

Example Typed, Non-Concrete, IsA Schema
#######################################

A simple non-concrete IsA schema prim with one attribute and one relationship
would look as follows. The schema *is* Typed, but a "SimplePrim" cannot be
instantiated on a UsdStage - it is likely a base-class for other, concrete
schemas.

.. code-block:: usda

    class "SimplePrim" (
        doc = """An example of an untyped schema prim. Note that it does not 
                 specify a typeName"""
        # IsA schemas should derive from </Typed>, which is defined in the 
        # sublayer usd/schema.usda.
        inherits = </Typed>
        customData = {
            # Provide a different class name for the C++ and python schema 
            # classes. This will be prefixed with libraryPrefix.
            # In this case, the class name becomes UsdSchemaExamplesSimple.
            string className = "Simple"
        }
    )  {
        int intAttr = 0 (
            doc = "An integer attribute with fallback value of 0."
        )
        rel target (
            doc = """A relationship called target that could point to another 
                     prim or a property"""
        )
    }

Example Concrete IsA Schema
###########################

The following is an example of a typed IsA schema class. It derives from the
non-concrete </SimplePrim> defined above, specifies a concrete typeName
'ComplexPrim' and adds a string attribute with a fallback value.

.. code-block:: usda

    class ComplexPrim "ComplexPrim" (
        doc = """An example of a untyped IsA schema prim"""
        # Inherits from </SimplePrim> defined in simple.usda.
        inherits = </SimplePrim>
        customData = {
            string className = "Complex"
        }
    )  {
        string complexString = "somethingComplex"
    }

Example Applied API Schema
##########################

The following is a simple example of an API schema that provides API for
manipulating three custom double valued attributes. Note also that API
schemas are distinguished from Typed schemas in that they **must** derive from
*APISchemaBase* .

.. code-block:: usda

    # API schemas only provide an interface to the prim's qualities.
    # They are not allowed to specify a typeName.
    class "ParamsAPI" (
        # IsA schemas should derive from </APISchemaBase>, which is defined in 
        # the sublayer usd/schema.usda.
        inherits = </APISchemaBase>
        customData = {
            token apiSchemaType = "singleApply"
        }
    )
    {
        double params:mass (
            # Informs schema generator to create GetMassAttr() instead of 
            # GetParamsMassAttr() method
            customData = {
                string apiName = "mass"
            }
            doc = "Double value denoting mass"
        )
        double params:velocity (
            customData = {
                string apiName = "velocity"
            }
            doc = "Double value denoting velocity"
        )
        double params:volume (
            customData = {
                string apiName = "volume"
            }
            doc = "Double value denoting volume"
        )
    }

All of the above schema classes are available in the schema.usda file
in extras/usd/examples/usdSchemaExamples/. You can run usdGenSchema to generate
all the necessary files. See below for the list of files generated (or edited 
if they already exist).

.. code-block:: console
   :caption: usdGenSchema output

   $ usdGenSchema schema.usda .

   Processing schema classes:
   SimplePrim, ComplexPrim, ParamsAPI
   Loading Templates
   Writing Schema Tokens:
           unchanged extras/usd/examples/usdSchemaExamples/tokens.h
           unchanged extras/usd/examples/usdSchemaExamples/tokens.cpp
           unchanged extras/usd/examples/usdSchemaExamples/wrapTokens.cpp
   Generating Classes:
           unchanged extras/usd/examples/usdSchemaExamples/simple.h
           unchanged extras/usd/examples/usdSchemaExamples/simple.cpp
           unchanged extras/usd/examples/usdSchemaExamples/wrapSimple.cpp
           unchanged extras/usd/examples/usdSchemaExamples/complex.h
           unchanged extras/usd/examples/usdSchemaExamples/complex.cpp
           unchanged extras/usd/examples/usdSchemaExamples/wrapComplex.cpp
           unchanged extras/usd/examples/usdSchemaExamples/paramsAPI.h
           unchanged extras/usd/examples/usdSchemaExamples/paramsAPI.cpp
           unchanged extras/usd/examples/usdSchemaExamples/wrapParamsAPI.cpp
           unchanged extras/usd/examples/usdSchemaExamples/plugInfo.json
   Generating Schematics:
           unchanged extras/usd/examples/usdSchemaExamples/generatedSchema.usda

Building the Schema
###################

To rebuild the plugin, simply go to the root of your build directory and run.

.. code-block:: console
   :caption: Build plugin using cmake

   cmake --build . --target install --config Release

Using the Schema Classes
########################
.. note:: Because this schema is an external plugin, the USD build must be told
   where to find it before it can be used. This can be done by either:
      
    * Setting the :filename:`PXR_PLUGINPATH_NAME` environment variable to 
      the location of the plugin's :filename:`resources` directory. For 
      example, if you are building the :filename:`usdSchemaExamples` plugin 
      out of the USD source tree, this location will be 
      :filename:`<prefix>/share/usd/examples/plugin/usdSchemaExamples/resources`
   
    * Copying :filename:`usdSchemaExamples.so` (on Windows, 
      :filename:`usdSchemaExamples.dll` and :filename:`.lib`) and the 
      :filename:`usdSchemaExamples` directory to :filename`<prefix>/plugin/usd`
   
Create a usd file named Test.usda with the following content:

.. code-block:: usda
   :caption: Test.usda

   #usda 1.0
   
    def ComplexPrim "Complex"
    {
        string complexString = "a really complex string"
   	    int intAttr = 10
   	    add rel target = </Object>
    }
   
    def Xform "Object" (
        prepend apiSchemas = ["ParamsAPI"]
    )
    {
        custom double params:mass = 1.0;
        custom double params:velocity = 10.0;
        custom double params:volume = 4.0;
    }

You should be able to load the above usda file in usdview without warnings or 
errors.

.. image:: http://openusd.org/images/tut_generating_new_schema_usdview.png

C++ Example
***********

The following C++ code loads the above test scene, constructs schema prims and
uses the API provided by schema code generation.

.. code-block:: cpp
   :caption: Schema Prim Usage C++ Example

    UsdStageRefPtr stage = UsdStage::Open("/path/to/testenv/Test.usda");
   
    UsdPrim cp = stage->GetPrimAtPath("/Complex");
   
    UsdSchemaExamplesSimple simple(cp);
    UsdRelationship target = simple.GetTargetRel();
    UsdAttribute intAttr = simple.GetIntAttrAttr();
   
    UsdSchemaExamplesComplex complex(cp);
    std::cout << complex.GetComplexStringAttr().Get<string>() << std::endl;
   
    UsdPrim obj = stage->GetPrimAtPath("/Object");
    UsdSchemaExamplesParamsAPI paramsAPI = UsdSchemaExamplesParamsAPI::Apply(obj);
    assert(obj.HasAPI<UsdSchemaExamplesParamsAPI>());
    std::cout << "mass: " << paramsAPI.GetMassAttr().Get<double>() << std::endl;
    std::cout << "velocity: " << paramsAPI.GetVelocityAttr().Get<double>() << std::endl;
    std::cout << "volume: " << paramsAPI.GetVolumeAttr().Get<double>() << std::endl;

Python Example
**************

The following python code loads the above test scene, constructs schema prims
and uses the API provided by schema code generation.

.. code-block:: python
   :caption: Python Example

   from pxr import Usd, UsdSchemaExamples
   stage = Usd.Stage.Open("Test.usda")
   cp = stage.GetPrimAtPath("/Complex")
   simple = UsdSchemaExamples.Simple(cp)
   target = simple.GetTargetRel()
   intAttr = simple.GetIntAttrAttr()
   complex = UsdSchemaExamples.Complex(cp)
   print('complexString: %s' % complex.GetComplexStringAttr().Get())
   obj = stage.GetPrimAtPath("/Object")
   paramsAPI = UsdSchemaExamples.ParamsAPI.Apply(obj)
   assert obj.HasAPI(UsdSchemaExamples.ParamsAPI)
   print('mass: %s' % paramsAPI.GetMassAttr().Get())
   print('velocity: %s' % paramsAPI.GetVelocityAttr().Get())
   print('volume: %s' % paramsAPI.GetVolumeAttr().Get())

Codeless Schemas
****************

Clients also have an option of not generating any code by setting the 
*skipCodeGeneration* metadata to True for a given schema, hence generating 
**only generatedSchema.usda and plugInfo.json** which are the only essential 
products for runtime schema registration.
Since codeless schemas do not provide any code, clients do not need to recompile
USD to use or update these schemas. This "dynamic" nature of codeless schemas 
is the primary motivation behind using codeless schemas.
