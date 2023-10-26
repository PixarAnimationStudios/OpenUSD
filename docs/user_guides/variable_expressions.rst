.. include:: ../rolesAndUtils.rst

.. _variable_expressions:

########################
USD Variable Expressions
########################

Variable expressions are specially formatted strings that contain expressions 
that USD will evaluate at runtime. The expressions can contain variable data 
and logic used to dynamically create or change values in your USD scene. Use
variable expressions to easily configure your scene without having to make
extensive scene data modifications. 

Variable expressions use named expression variables that can be easily 
defined and changed in layer metadata. Expression variables are defined in a 
"expressionVariables" dictionary metadata field in a layer. 

Variable expressions can currently be used in:

- Asset paths (including sublayer, reference, and payload asset paths as well as 
  asset-valued attributes and metadata). 
- Variant selections.

The following simple example demonstrates using variable expressions 
to specify a reference path and a variant selection. An ASSET_PATH expression 
variable is set to a directory, and then used in a reference variable 
expression. Similarly, a VARIANT_CHOICE expression variable is defined and 
then used in a variable expression for selecting a variant. 

.. code-block:: usda

    #usda 1.0
    (
        expressionVariables = {
            # Define path to reference that can be changed as needed
            string ASSET_PATH = "/finalAssets/SetA"
            # Define the expression variable that can be changed as needed
            string VARIANT_CHOICE = "variantA"
        }
    )
    def "asset0" (
        # Use a variable expression to complete the reference asset path
        references = @`"${ASSET_PATH}/extraAssets.usda"`@</asset0>
    )
    {
    }

    def Xform "asset1" (
        variants = {
            # Use a variable expression for specifying the variant
            string displayVariantSet = "`${VARIANT_CHOICE}`"
        }
        prepend variantSets = "displayVariantSet"
    )
    {
        ...displayVariantSet definition omitted...
    }

.. _defining_expression_variables:

****************************************
Defining Expression Variables in a Layer
****************************************

Add your named expression variables to the "expressionVariables" dictionary 
metadata field in your layer. Expression variables must be authored on either 
the root or session layer of a stage or in the root layer of a referenced 
:ref:`layer stack <usdglossary-layerstack>`.

.. code-block:: usda

    #usda 1.0
    (
        expressionVariables = {
            string VARIANT_CHOICE = "variantA"
            bool UseProxyModel = false
            int64 IDENTIFIER = 3254    
            string[] renderPassList = ["foreground", "background", "FX"]        
        }
    )

Expression variables can currently be set to the following types.

:usda:`string`
==============

A string, which in USD's utf-8 text format, :filename:`usda`, is delimited by 
double or single quotes. Quote characters can be escaped using 
Python-style escaping ("\"). A string value itself can contain a variable 
expression contained in backticks ("`"), that will be evaluated when the 
variable is used in a variable expression.

Examples:

.. code-block:: 

    string exampleStr = "I am a string"
    string exampleStrEscaped = "A \"quote escaped\" string"
    string exampleStrEscaped2 = """Another "escaped" string"""
    string exampleStrBoolExpression = "`True`"
    string exampleStrIfExpression = '`if(${USE_RED}, "red", "blue")`'
    string exampleStrExpanded = "`charA_${MODEL_CATEGORY}_${MODEL_VERSION}`"

:usda:`bool`
============

A boolean value. Accepted values are :usda:`True`, :usda:`true`, :usda:`False`, 
or :usda:`false`. 

Examples:

.. code-block:: usda

    bool exampleBool = False 
    bool exampleBool2 = true

:usda:`int64`
=============

A 64-bit integer. 

Examples: 

.. code-block:: usda

    int64 exampleInt = 43 
    int64 exampleIntNeg = -500

:usda:`<type>[]`
================

A list of one of the previously mentioned types. The list can only contain 
values of one type. Lists with mixed types will result in an error.

Examples: 

.. code-block:: usda

    string[] exampleStringList = ["red", "green", "blue"]
    int64[] exampleIntList = [4, 5, 6]
    bool[] exampleBoolList = [True, False, False, True]

.. _expression_variable_none_value:

:usda:`None`
============

:usda:`None` is a special case value. Currently, you can't set a variable 
directly to :usda:`None`. The :usda:`None` value can only be used within an 
expression, and is useful for expression functions, like the :usda:`if()` 
function, detailed in 
:ref:`Expression Function Reference <expression_function_reference>`. 

:usda:`None` is also not allowed in a list within an expression. The following
examples will result in an error when evaluated:

.. code-block::

    `[1,2,3,None]` # Lists with None values are not allowed
    `[if(False, 1)]` # The if() function will evaluate to None, so this is also not allowed in a list

Expression Variables and Composition
====================================

Like all other stage metadata, expression variables do not compose across 
sublayers in a layer stack except for the session and root layers of a stage. 
This avoids possible circular or order-dependent evaluations that would result 
in inconsistent behavior.

Expression variables *will* compose across references and payloads. If a 
referenced layer stack contains expressions that refer to expression variables, 
the expression variables from the chain of referencing layer stacks will be 
composed together and used to evaluate those expressions. However, note that 
the sublayer restriction from above still applies, so only expression variables 
authored on the root layer of the layer stacks will be considered. 

Composing across references and payloads ensures that assets that internally use 
expressions referring to variables can be referenced by another asset and still 
work as expected. This also gives the referencing asset the ability to override 
the expression variables used by the referenced asset. For example, if you had 
the following defined in :filename:`model.usda`:

.. code-block:: usda

    #usda 1.0
    (
        expressionVariables = {
            string MODEL_VARIANT = "blue"
        }
        defaultPrim = "Model_1"
    )
     
    def Xform "Model_1" (
        variants = {
            string modelVariant = "`${MODEL_VARIANT}`"
        }
        prepend variantSets = "modelVariant"
    )
    {
        ...modelVariant definitions omitted...
    }

And then referenced :usda:`model.usda</Model_1>` in your stage:

.. code-block:: usda

    #usda 1.0
    (
        expressionVariables = {
            string MODEL_VARIANT = "red"
        }
        defaultPrim = "Model_1"
    )
     
    over Xform "Model_1" (
        # reference model layer, but apply stage-level
        # MODEL_VARIANT variable value
        references = @model.usda@</Model_1> 
    )
    {
    }

The stage's root layer MODEL_VARIANT variable will be applied to the referenced 
object, and the "red" variant will be used. If you omitted MODEL_VARIANT in your 
stage, the :filename:`model.usda` root layer's MODEL_VARIANT would be applied 
instead, and the "blue" variant would be used.

******************************
Authoring Variable Expressions
******************************

Expressions are represented in USD scene data as strings that are surrounded 
with backticks ("`"). Variables used in an expression must be included using the 
${variableName} format. 

Expressions can use expression variables to produce dynamic values when 
evaluated. The following example uses two variables in an expression to produce 
a reference path.

.. code-block:: usda

    references = @`${ASSET_DIRECTORY}/${CHARACTERS_DIRECTORY}/CharSetA.usda`@

When mixing expression variables and string content, you might need to escape 
special characters. The following examples show different ways of escaping 
characters.

.. code-block:: 

    # The following expression evaluates to "C:\USD\test.usd" or "D:\USD\test.usd" 
    # (depending on if COND evaluates to true or false) due to escaped backslashes
    `if(${COND}, "C:\\USD\\test.usd", "D:\\USD\test.usd")`  

    # The following evaluates to "escaped_var_${X}". No substitutions occur due 
    # to the escaped "$".
    `"escaped_var_\${X}"` 

Expressions can also use expression functions to add conditional logic. The 
following example uses the :usda:`if()` function to conditionally pick between 
two different reference paths, based on whether USE_LOW_POLY_ASSETS evaluates to 
:usda:`true` or :usda:`false`.

.. code-block:: usda

    references = @`if(${USE_LOW_POLY_ASSETS}, "assetsLow/extraAsset.usda", "assetsNormal/extraAsset.usda")`@

The full list of expression functions is described in 
:ref:`Expression Function Reference <expression_function_reference>`.

.. _expression_function_reference:

*****************************
Expression Function Reference
*****************************

The following functions are supported for expressions. For all functions, the 
function arguments can be values (of a type listed in 
:ref:`Defining Expression Variables in a Layer <defining_expression_variables>`), 
variables, or functions. 

:usda:`defined(<variable name>, ...)`
=====================================

If <variable name> is defined in the composed set of expression 
variables visible to the layer where the expression is authored, return 
:usda:`true`, otherwise return :usda:`false`. This function can take multiple 
variable names. All listed variable names must be defined for this function to 
return true. Note that variable names must be provided as simple strings -- you 
cannot use a string that contains a variable expression with this function.

Examples:

.. code-block::

    defined("VARIABLE1", "VARIABLE2") # returns true if VARIABLE1 and VARIABLE2 are both defined 
    if( defined("VARIABLE1"), ${VARIABLE1}, None) # if VARIABLE1 is defined, returns whatever VARIABLE1 evaluates to, otherwise None

:usda:`if(<condition>, <true-value>, <false-value>)`
====================================================

If <condition> evaluates to :usda:`true`, returns <true-value>, 
otherwise returns <false-value>. An error results if <condition>
does not evaluate to a boolean value. Note that <true-value> and
<false-value> must evaluate to the same type, or None.

Examples: 

.. code-block:: 

    # If the USE_OLD_ASSETS_BOOLEAN_VAR variable is set to true, return "/archived/Assets", 
    # otherwise return whatever string value CURRENT_ASSETS_PATH evaluates to.
    if(${USE_OLD_ASSETS_BOOLEAN_VAR}, "/archived/Assets", ${CURRENT_ASSETS_PATH})  

:usda:`if(<condition>, <true-value>)`
=====================================

If <condition> evaluates to :usda:`true`, returns <true-value>, 
otherwise returns usda:`None`. An error results if <condition> does not 
evaluate to a boolean value. 

Examples: 

.. code-block:: 

    if(${USE_OLD_ASSETS_BOOLEAN_VAR}, "/archived/Assets")

:usda:`and(<x>, <y>, ...)`
==========================

Returns the result of the logical :usda:`and` operation of all arguments. If all 
arguments evaluate to :usda:`true`, this will return :usda:`true`, otherwise 
this returns :usda:`false`. An error results if any argument does not evaluate 
to a boolean value. 

Examples:

.. code-block:: 

    and( ${BOOL_VAR}, True )  # if BOOL_VAR evaluates to true, this will return true, otherwise false

:usda:`or(<x>, <y>, ...)`
=========================

Returns the result of the logical or operation of all arguments. If any argument 
evaluates to :usda:`true`, this will return :usda:`true`. If all arguments 
evaluate to :usda:`false`, this will return :usda:`false`. An error results if 
any argument does not evaluate to a boolean value.

Examples:

.. code-block:: usda

    or( false, true, false )  # returns true

:usda:`not(<x>)`
================

Returns the opposite boolean value that <x> evaluates to. An error 
results if <x> does not evaluate to a boolean value.

:usda:`eq(<x>, <y>)`
====================

Compares the evaluated values of <x> and <y>. Returns 
:usda:`true` if values are equal, :usda:`false` otherwise. An error results if 
<x> and <y> evaluate to different types. For strings, comparison 
is a lexicographic comparison. 

Examples: 

.. code-block:: 

    eq(12, ${INT_VAR})  # returns true if INT_VAR is defined, is a int64 variable, and evaluates to 12
    eq("hello", "HELLO") # returns false

:usda:`neq(<x>, <y>)`
=====================

Compares the evaluated values of <x> and <y>. Returns 
:usda:`true` if values are not equal, :usda:`false` otherwise. An error results 
if <x> and <y> evaluate to different types. For strings, 
comparison is a lexicographic comparison. 

:usda:`lt(<x>, <y>)`
====================
:usda:`leq(<x>, <y>)`
=====================
:usda:`gt(<x>, <y>)`
====================
:usda:`geq(<x>, <y>)`
=====================

Compares the evaluated values of <x> and <y>. Returns 
:usda:`true` if <x> is less than (lt), less than or equal (leq), greater 
than (gt), or greater than or equal (geq) to <y>. An error results if 
<x> and <y> evaluate to different types. For strings, comparison 
is a lexicographic comparison. For booleans, :usda:`true` is considered greater 
than :usda:`false`.

Examples:

.. code-block:: usda

    lt(10,12) # returns true
    gt("Abc", "abc") # returns false, capital letters are 'earlier' lexicographically
    gt("xyz", "abc") # returns true, xyz is 'later' than abc    
    gt("applecart", "apple") # returns true
    gt(true, false) # returns true

:usda:`contains(<list_or_string>, <value>)`
===========================================

Returns :usda:`true` if <list_or_string> contains <value>, 
:usda:`false` otherwise. This function works on both lists and strings. If 
<list_or_string> is a list, this function checks if <value> is 
an element in the list. If <list_or_string> is a string, this function 
checks if <value> is a substring in that string.

An error results if <list_or_string> does not evaluate to a list or string. If 
<list_or_string> is a list, and <value> is a different type than the list 
element type, this results in an error. Similarly, if <list_or_string> is a 
string and <value> is not a string, this results in an error.

Note that lists cannot contain the :usda:`None` value, as described in 
:ref:`Defining Expression Variables in a Layer:None <expression_variable_none_value>`.

Examples:

.. code-block:: 

    # defined in layer expressionVariables: 
    #   string[] assetList = ["assetsLow/extraAsset.usda", "assetsMedium/extraAsset.usda", "assetsHigh/extraAsset.usda"]

    contains(${assetList}, "assetsExtraHigh/extraAsset.usda") # returns false
    contains("12345678", "45") # returns true

:usda:`at(<list_or_string>, <index>)`
=====================================

Returns the value at the (0-based) <index> in <list_or_string>. 
This function works on both lists and strings. If <list_or_string> is a 
list, this function returns the list element at index <index>. If 
<list_or_string> is a string, this function returns a string that 
contains the character at index <index> in the string. 

Note that negative index values are supported. Negative index values behave like 
negative indices in Python, accessing elements by counting from the end of the 
list or string rather than from the beginning.

An error results if:

- <list_or_string> does not evaluate to a list or string.
- <index> does not evaluate to an int64.
- <index> is not a valid index into <list_or_string>.

Examples:

.. code-block:: 

    # defined in layer expressionVariables: 
    #   string[] assetList = ["assetsLow/extraAsset.usda", "assetsMedium/extraAsset.usda", "assetsHigh/extraAsset.usda"]
    #   int64 newIndex = 1

    at(${assetList}, 0) # returns "assetsLow/extraAsset.usda"
    at(${assetList}, ${newIndex}) # returns "assetsMedium/extraAsset.usda"
    at(${assetList}, -1) # returns "assetsHigh/extraAsset.usda"
    at(${assetList}, 15) # results in Index of of Range error
    at("123456",-2) # returns "5"

:usda:`len(<list_or_string>)`
=============================

Returns the number of elements in <list_or_string>. An error results if 
<list_or_string> doesn't evaluate to a list or string. Can be used to sanity 
check that lists or strings are not empty before accessing them with :usda:`at()` 
or :usda:`contains()`. Depending on how lists are used in your scene, checking 
the length of a list could also provide more insight into asset structure 
(e.g., a "versions" list with length greater than 1 could imply additional 
versions of an asset exist).

Examples:

.. code-block:: 

    # returns number of elements in assetList, or error if assetList undefined or not a list or string
    len(${assetList})  

    # returns 5
    len("hello")

    # sanity check assetList has at least one element, and if so, return the 0th index element 
    # otherwise return a default path string
    if( gt(len(${assetList}), 0), at(${assetList},0), "defaultAssetPath") 

********
Examples
********

This section provides additional examples using variable expressions for various 
use-cases.

Flexible Variant Selections
===========================

Variable expressions can provide flexible ways of specifying variations 
for logical groups of prims that are controlled via a single expression variable.

For example, consider an asset with a large crowd of models, each with variants 
controlling some aspect of their look, like a color palette. A crowds artist 
could select models that should have the same color palette and set their 
variant selections to corresponding variables, like:

:filename:`Crowd.usda`

.. code-block:: usda

    #usda 1.0
    (
        expressionVariables = {
            string COLOR_GROUP_1 = "regular"
            string COLOR_GROUP_2 = "regular"
        }
    )

    def "Crowd"
    {
        def "Model_1" (
            variantSelection = {
                string palette = "`${COLOR_GROUP_1}`"
            }
        )
        {
        }

        def "Model_2" (
            variantSelection = {
                string palette = "`${COLOR_GROUP_2}`"
            }
        )
        {
        }

        # etc., etc. ... 
    }

When another artist references this crowd asset into their own stage, they can 
change the color palettes for all of the prims in the groups determined by the 
original crowd artist just by overriding the expression variables:

:filename:`Shot.usda`

.. code-block:: usda

    #usda 1.0
    (
        expressionVariables = {
            string COLOR_GROUP_1 = "rainbow"
            string COLOR_GROUP_2 = "monochrome"
        }
    )

    def "Crowd" (references = @Crowd.usd@</Crowd>)
    {
    }        

The artist can still change the palette variant on individual models if they 
desire simply by overriding the variant selection on the specific prim.

Asset-valued Texture File Attribute
===================================

Different renderers may require different texture file formats. Rather than 
using alternate layers that specify different texture files for each renderer, 
you can use variable expressions to conditionally pick the desired texture file. 
The following example demonstrates this use-case by using variable expressions 
to configure the :usda:`inputs:file` asset-valued attribute on a UsdUVTexture 
Shader. 

.. code-block:: usda

    #usda 1.0
    (
        expressionVariables = {
            string TEXTURE_DIR = "textures"
            bool USING_PRMAN_RENDERER = true
            string DEFAULT_TEXTURE_FILE = "pattern.png"
            string PRMAN_TEXTURE_FILE = "pattern.tex"
        }
    )

    ...

    def Material "boardMat"
    {
        def Shader "diffuseTexture"
        {
            uniform token info:id = "UsdUVTexture"
            asset inputs:file = @`if( ${USING_PRMAN_RENDERER}, "${TEXTURE_DIR}/${PRMAN_TEXTURE_FILE}", "${TEXTURE_DIR}/${DEFAULT_TEXTURE_FILE}")`@
            float2 inputs:st.connect = </boardMat/stReader.outputs:result>
            float3 outputs:rgb
        }

        ... other USD Preview Material Shaders omitted...
    }

Conditionally Include Sublayers
===============================

If you use multiple render passes when rendering your scene, you may need to use 
different sublayers for specific render passes. For example, you might have a 
render pass for FX that includes a sublayer that deactivates non-effects-related 
geometry to speed up rendering. You can use variable expressions to 
conditionally specify which sublayers to include for a given render pass. This 
might look something like:

.. code-block:: usda

    #usda 1.0
    (
        expressionVariables = {
            string RENDER_PASS = "fx"
        }

        subLayers = [
            @`"render_pass_${RENDER_PASS}.usd"`@,
            ...
        ]
    )