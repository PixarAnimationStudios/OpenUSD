.. include:: ../rolesAndUtils.rst

.. _collections_and_patterns:

########################
Collections and Patterns
########################

Use collections to describe a group of heterogeneous objects within the scene.
Collections can directly identify prims and properties belonging to prims, and 
can also identify other collections, whose entire contents will then be included 
in the collection as well.

Collections are used to facilitate workflows that need a flexible representation
of sets of objects in a scene. Example use-cases for collections in USD
include features such as light-linking, where you specify a collection of 
objects that a given light illuminates, and binding materials to collections, 
where you can bind a material to a complex collection of logical "parts" of a 
model. You can even aggregate collections in your workflow by including
collections in other collections. 

A collection's membership is specified in one of two ways. The first way uses
target-based rules to specify which objects are included (and excluded) from
the collection. Collections configured this way are considered to be in 
**relationship-mode**. A second way of specifying membership uses an 
expression-based syntax to match objects to be included in the collection. 
Collections configured this way are **pattern-based collections** and the 
collection is considered to be in **expression-mode**. Note that you cannot mix 
collection configurations. Collections can be configured to work in 
relationship-mode or expression-mode, but not a mix of the two.

Collections are defined in a prim with the CollectionAPI schema applied. The
CollectionAPI schema is a multiple-apply schema, so it can be applied to a prim 
multiple times with different instance names to define several collections on a 
single prim.

The following example shows a single prim with two collections. The 
``relCollection`` collection is a relationship-mode collection that includes all 
objects at the :sdfpath:`/World/Clothing/Shirts` and :sdfpath:`/World/Clothing/Pants`
paths. The ``expCollection`` collection is a pattern-based collection that 
matches all objects at the :sdfpath:`/World/Clothing/Shirts` path that start 
with "Red", and any descendants of those objects.

.. code-block:: usda

    def "CollectionPrim" (
        prepend apiSchemas = ["CollectionAPI:relCollection", "CollectionAPI:expCollection"]
    )
    {
        # Specify collection membership using "includes"
        rel collection:relCollection:includes = [
            </World/Clothing/Shirts>,
            </World/Clothing/Pants>,
        ]

        # Specify collection membership using a path expression
        pathExpression collection:expCollection:membershipExpression = "/World/Clothing/Shirts/Red*//"
    }

.. contents:: Table of Contents
    :local:
    :depth: 3

.. _basic_usage:

***********
Basic Usage
***********

To create and configure a collection, use the following steps:

1. Apply the CollectionAPI schema to the prim that will contain the collection,
   either in your USD data or programmatically.

   .. code-block:: usda

       def "CollectionPrim" (
           prepend apiSchemas = ["CollectionAPI:myCollection"]
       )
       {
           ...
       }

   .. code-block:: python

       collectionPrim = stage.DefinePrim("/CollectionPrim")
       myCollection = Usd.CollectionAPI.Apply(collectionPrim, "myCollection")

2. Configure what objects are in the collection by setting properties for
   a relationship-mode collection or a pattern-based collection, described in
   :ref:`relationship_mode_collections` and :ref:`pattern_based_collections` 
   below.

To query a configured collection for objects or paths from the collection, use 
the following steps:

1. Compute the fully resolved set of objects based on the collection 
   configuration by calling its ``ComputeMembershipQuery()`` method. This 
   returns a  ``CollectionMembershipQuery`` object used for querying the 
   collection. Note that you need to call ``ComputeMembershipQuery()`` whenever 
   the collection configuration changes.

   .. code-block:: python

       query = myCollection.ComputeMembershipQuery()

2. Use the query object along with the ``ComputeIncludedObjects()`` or
   ``ComputeIncludedPaths()`` APIs to get the set of matching objects or paths 
   (respectively) for the collection, queried against a specific stage.

   .. code-block:: python

       matchedObjects = Usd.CollectionAPI.ComputeIncludedObjects(query, stage)
       for obj in matchedObjects:
           # ... do something with each matching object ...

       matchedPaths = Usd.CollectionAPI.ComputeIncludedPaths(query, stage)
       for path in matchedPaths:
           # ... do something with each matching path ...

   Note that ``ComputeIncludedObjects()`` and ``ComputeIncludedPaths()`` take 
   an optional ``PrimFlagsPredicate`` argument that defaults to 
   ``Usd.PrimDefaultPredicate``. If you need to match abstract (class) prims, 
   you'll need to use a traversal predicate that includes abstract prims, such 
   as ``PrimAllPrimsPredicate``.

   .. code-block:: python

       # Get all prims including abstract prims
       matchedObjects = Usd.CollectionAPI.ComputeIncludedObjects(query, stage, Usd.PrimAllPrimsPredicate)

3. If you already are traversing objects in a scene and want to include 
   checking whether objects are in a collection as part of the traversal, 
   the query object provides the ``IsPathIncluded()`` method to directly check
   if a path is in a collection. 

   .. code-block:: python

       # Check to see if /MyPath/PrimA is in myCollection
       if query.IsPathIncluded("/MyPath/PrimA"):
           # ...PrimA is in collection, process accordingly...

.. _relationship_mode_collections:

*****************************
Relationship-Mode Collections
*****************************

A relationship-mode collection uses "include/exclude" rules to define 
membership in the collection. These rules are defined via ``includes`` and 
``excludes`` relationships, and an ``includeRoot`` property, on the 
collection prim. Additionally, the ``expansionRule`` property controls how
the ``includes`` and ``excludes`` rules are expanded.

.. _configuring_relationship_mode_collections:

Configuring Relationship-Mode Collections
=========================================

Use the following built-in properties from the CollectionAPI schema to configure
membership in a collection in relationship-mode, where ``instanceName`` is the 
user-provided applied API schema instance name:

* ``rel collection:instanceName:includes``: Specifies a list of targets that are 
  included in the collection. This can target prims or properties directly. A 
  collection can insert the rules of another collection by making its includes 
  relationship target the ``collection:otherInstanceName`` property from the 
  collection to be included. Note that including another collection does not 
  guarantee the contents of that collection will be in the final collection; 
  instead, the rules are merged. This means, for example, an exclude entry may 
  exclude a portion of the included collection. When a collection includes one 
  or more collections, the order in which targets are added to the ``includes`` 
  relationship may become significant, if there are conflicting opinions about 
  the same path. Targets that are added later are considered to be stronger than 
  earlier targets for the same path.

* ``rel collection:instanceName:excludes``: Specifies a list of targets that are 
  excluded below the included paths in this collection. This can target prims or 
  properties directly, but cannot target another collection. This is to keep the 
  membership determining logic simple, efficient and easier to reason about. 
  It is invalid for a collection to exclude paths that are not included in it. 
  The presence of such "orphaned" excluded paths will not affect the set of 
  paths included in the collection, but may affect the performance of querying 
  membership of a path in the collection or of enumerating the objects belonging 
  to the collection.

  In some scenarios it is useful to express a collection that includes
  everything except certain paths. To support this, a relationship-mode
  collection that has an exclude that is not descendant to any include will
  include the pseudo-root path :sdfpath:`/`.

* ``bool collection:instanceName:includeRoot``: Indicates whether the 
  pseudo-root path :sdfpath:`/` should be counted as one of the included target 
  paths. This separate attribute is required because relationships cannot 
  directly target the root. The fallback value is ``false``. 

The following properties provide additional configuration aspects for 
collections: 

* ``uniform token collection:instanceName:expansionRule``: Specifies how to 
  expand the includes and excludes relationship targets to determine the 
  collection's members. Possible values include:

  * ``expandPrims``: All the prims descendent to the ``includes`` relationship 
    targets (and not descendent to ``excludes`` relationship targets) belong to 
    the collection. Any includes-targeted property paths also belong to the 
    collection. This is the default behavior. 
  * ``expandPrimsAndProperties``: Like ``expandPrims``, but all properties on 
    all included prims also belong to the collection. 
  * ``explicitOnly``: Only paths in the ``includes`` relationship targets and 
    not those in the ``excludes`` relationship targets belong to the collection.

The following example shows a relationship-mode collection configured to 
include all prims (but not properties) at 
:sdfpath:`/World/Lights` and prims from a separate collection at 
:sdfpath:`/ExtraLights.collection:allExtraLights`, but exclude prims at 
:sdfpath:`/World/Lights/TestLights`.

.. code-block:: usda

    def "CollectionPrim" (
        prepend apiSchemas = ["CollectionAPI:allProdLights"]
    )
    {
        # Set the collection expansion rule to specify how the paths for collection are expanded
        uniform token collection:myCollection:expansionRule = "expandPrims"

        # Set includes/excludes to include all lights except those under /World/Lights/TestLights
        rel collection:allProdLights:includes = [
            </World/Lights>,
            </ExtraLights.collection:allExtraLights>
        ]
        rel collection:allProdLights:excludes = [
            </World/Lights/TestLights>,
        ]
    }

.. _pattern_based_collections:

*************************
Pattern-Based Collections
*************************

Pattern-based collections provide an alternative way to define the set of 
objects in a USD collection. Rather than specifying rules for paths to include 
and exclude in the collection, you provide a **path expression** that the 
collection API uses to match against objects in your stage. 

Pattern-based collections are helpful in scenarios where the scene hierarchy and 
composition is complex, and it's not possible to set include and exclude rules 
that represent the collection you want. Pattern-based collections use 
expressions that can match path names, object characteristics, and other 
mechanisms that aren't strictly tied to scene hierarchy. You can also create 
expressions that reference other expressions for even greater flexibility.

One example use case for pattern-based collections is light-linking. When 
rendering your USD scene, you might need to control which geometry is 
illuminated by a particular light. To support this, USD provides light-linking 
which uses the ``collection:lightLink`` object collection (part of the LightAPI
schema) to specify which objects are illuminated by a light. However, the 
lights in your scene might be widely distributed across many layers with 
different hierarchies. It might be difficult to come up with a set of 
include/exclude rules that properly capture all the lights you want to add to 
your collection, and the rules might not work with future changes to lights in 
the scene. With pattern-based collections, you can use name and path wildcards, 
expression logic, and other qualifiers, like requiring a prim have the LightAPI 
schema applied, to assemble your collection.

Pattern-based collections are defined in a prim with the CollectionAPI 
schema applied and the ``membershipExpression`` attribute set to a valid
path expression. 

.. note:: 
    Collections cannot mix includes/excludes and expressions. You can't set both 
    includes/excludes rules and an expression on a single collection. If a
    collection has ``includes`` or ``excludes`` rules set, or has its 
    ``includeRoot`` attribute set to ``true``, the collection is in 
    relationship-mode and any expression authored on the 
    ``membershipExpression`` attribute is ignored.

.. _path_expressions:

Path Expressions
================

A path expression contains zero or more patterns, operators, and references to 
other expressions, used to capture the set of objects you want to match. You'll 
typically author your path expressions as strings, and use these strings to set 
pathExpression attributes on prims with the CollectionAPI schema applied 
(see :ref:`configuring_pattern_based_collections` below for more details).

.. _path_patterns:

Path Patterns
-------------

A pattern is a sequence of characters that follow the syntax rules for a 
``Sdf.Path``, combined with optional glob-style wildcard characters and optional 
predicate functions. Paths in a pattern can be absolute, or relative.

.. _glob_style_wildcards:

Glob-Style Wildcards
^^^^^^^^^^^^^^^^^^^^

The supported glob-style wildcard characters are:

* ``*``: Match zero or more characters, but not path or property separators. 
  Example: ``/char*`` can match :sdfpath:`/char`, :sdfpath:`/character`, 
  but not :sdfpath:`/char/arm` or :sdfpath:`/char.myAttribute`.
* ``?``: Match any single character, but not a path or property separator. 
  Example: ``/appl?`` can match :sdfpath:`/apple` or :sdfpath:`/apply` but not 
  :sdfpath:`/applesauce` or :sdfpath:`/appl.price`.
* ``//``: Match any path hierarchy, used to match any arbitrary length path 
  sequence. Example: ``/primA//leafB`` can match :sdfpath:`primA/leafB` 
  or :sdfpath:`/primA/child1/child2/leafB`.

.. _predicate_functions:

Predicate Functions
^^^^^^^^^^^^^^^^^^^

You can optionally include a predicate function in a pattern. Predicate 
functions let you add additional qualifiers for matching prims and properties. 
For example, you can specify that any objects that match a specific base pattern 
(e.g. ``/foo*``) also need to have a particular API schema applied to 
match, by adding a ``hasAPI`` predicate function:

.. code-block:: 

    /foo*{hasAPI:MaterialBindingAPI}

Predicate functions can take arguments, including named arguments, and may have 
default values for arguments. USD provides several built-in predicate functions 
(listed below). 

To use a predicate function with your pattern, specify the predicate function 
using the ``{predicate-name}`` format. The following example matches root-level 
prims starting with "char" that are abstract (classes):

.. code-block::

    /char*{abstract}

If a predicate function takes arguments, you can list them in-order using a 
format of ``{predicate-name:arg1, arg2, etc}`` or using a format of 
``{predicate-name(arg1, arg2, etc)}``. The following examples match root-level 
prims starting with "char" that are classes or overs:

.. code-block::

    /char*{specifier:class,over}
    /char*{specifier(class,over)}

If a predicate function takes named arguments, you can specify them by name, 
using a format of ``{predicate-name(arg1name=arg1value, arg3name = arg3value, arg2name=arg2value, etc)}``. 
The following example matches root-level prims starting with "char" that have a 
variant selection of ``blue`` for the variant set ``myVariantSet`` and a variant 
selection of ``red`` for the variant set ``myVariantSet2``.

.. code-block::

    /char*{variant(myVariantSet=blue, myVariantSet2=red)}

If a predicate function is not preceded by a pattern, ``*`` is implied to be
the desired pattern. Similarly, if an expression only consists of a predicate
function, it's assumed ``//*`` is the intended pattern.

.. code-block::

    /primA/{abstract} # equivalent to "/primA/*{abstract}", all objects under primA that are abstract
    {abstract}        # equivalent to "//*{abstract}", all objects in the scene that are abstract

Predicate functions can be combined using ``not``, ``and``, ``or`` boolean
operators, and grouped using ``( )``. The following matches prims that start
with ``standin`` that have the MaterialBindingAPI schema applied, and are
not Spheres or Cubes.

.. code-block::

    /standin*{hasAPI:MaterialBindingAPI and not (isa:Sphere or isa:Cube)}

CollectionAPI provides the following built-in predicate functions:

* ``abstract(bool=true)`` match prims that are or are not abstract.
* ``defined(bool=true)`` match prims that are or are not defined.
* ``model(bool=true)`` match prims that are or are not considered models.
* ``group(bool=true)`` match prims that are or are not considered groups.
* ``kind(kind1, ... kindN, strict=false)`` match prims of any of the given kinds. 
  If ``strict=true``, matching subkinds is not allowed, only exact matches pass.
* ``specifier(specifier1, ... specifierN)`` match prims with any of the given 
  specifiers.
* ``isa(typeName1, ... typeNameN, strict=false)`` match prims that are any typed 
  schema typeName1..N or subtypes. Disallow subtypes if ``strict=true``.
* ``hasAPI(typeName1, ... typeNameN, instanceName='')`` match prims that have any 
  of the applied API schemas 1..N. If ``instanceName`` is provided, the prim must 
  have an applied API schema with that instanceName.
* ``variant(setName1 = selGlob1, ... , setNameN = selGlobN)`` match prims that have 
  variant selections matching the literal names or glob patterns in 
  selGlob1..selGlobN for the variant sets setName1..N.

.. _logical_operators:

Logical Operators
^^^^^^^^^^^^^^^^^

Use operators to apply algebraic logic with patterns and expression references 
in order to refine your expression. Most operators are used between 
patterns or expression references (e.g. "leftPattern <operator> rightPattern").

The supported operators are:

* ``+``: Union. Matches all objects matched by either left or right pattern 
  operands, or objects matched by both patterns. Example: ``/foo* + /*bar`` 
  can match :sdfpath:`/foofoo`, :sdfpath:`/barbar`, and :sdfpath:`/foobar`.
* ``(white space)``: Implied union. White space between patterns is 
  treated as an implied union, and behaves exactly like a union. 
  ``/foo* /*bar`` is the same as ``/foo* + /*bar``.
* ``&``: Intersection. Matches only objects matched by both left and right 
  pattern operands. For example ``/foo* & /*bar`` can match :sdfpath:`/foobar`, 
  but not :sdfpath:`/foofoo` or :sdfpath:`/barbar`.  
* ``-``: Difference. Matches objects matched by the left pattern operand, but 
  discards any matches that would also be matched by the right pattern operand. 
  For example, ``/foo* - /*bar`` can match :sdfpath:`/foo` and 
  :sdfpath:`/foofoo` but not :sdfpath:`/foobar`, nor :sdfpath:`/barbar`.
* ``~``: Complement. Matches everything not matched by the pattern operand. For 
  example, ``~/foo*`` matches everything except paths like 
  :sdfpath:`/foo`, :sdfpath:`/foofoo`, etc. 
* ``( )``: Group. Use parenthesis to group patterns-plus-operators into a 
  logical group to enforce evaluation order. 

.. _expression_references:

Expression References
^^^^^^^^^^^^^^^^^^^^^

Expressions can reference other expressions, which is useful for building 
complex expressions from sets of "base" expressions, or re-using expressions 
in your scene or in code.

An expression reference starts with ``%`` followed by a prim path, a ``:``, and 
a name. The name of the sub-expression can be defined in USD data or in code. 
Note that wildcards are not allowed in the names of expression references.

For example, you might have a "allFruit" expression that consists of 
``/Fruit/apple /Fruit/banana /Fruit/*berry``, and a "allFood" expression 
that references allFruit: ``%/myPrim:allFruit + /Cheese* + /Meat*``. When 
the "allFood" expression is resolved, the resolved expression for "allFruit" 
will be included where the allFruit expression is referenced. 

If you need to reference an expression from a sibling collection on the same
prim, you can omit the prim path. Using the previous example, if allFruit
and allFood happened to be expression attributes for two different collections
in the same prim, the allFood expression could reference allFruit as:
``%:allFruit + /Cheese* + /Meat*``

.. note::
    Note that you can't reference a relationship-mode collection (that uses 
    includes/excludes rules) as a sub-expression in a pattern-based collection. 
    You can only reference other pattern-based collection expressions as 
    sub-expressions. See :ref:`expression_for_relationship_mode_collection` for
    details on computing the equivalent expression for a relationship-mode 
    collection that you can then reference.

.. _resolving_expressions:

Resolving Expressions That Use Expression References
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When resolving named expression references in pattern-based collections, the 
expression name is the property name that contains the ``membershipExpression`` 
for the collection. Using the previous allFruit and allFood example, in your 
USD data you might have a prim with the CollectionAPI schema applied, that looks
something like:

.. code-block:: usda

    def "Food" 
    (
        prepend apiSchemas = ["CollectionAPI:allFruit", "CollectionAPI:allFood"]
    )
    {
        pathExpression collection:allFruit:membershipExpression = "/Fruit/apple /Fruit/banana /Fruit/*berry"

        pathExpression collection:allFood:membershipExpression = "%:allFruit + /Cheese* + /Meat*"
        ...
    }

USD will look for pathExpression properties with the referenced name when 
resolving the full expression. The expressions will automatically be resolved 
using the composed stage when the collections are queried. You can
also get the fully resolved expression by calling 
``ResolveCompleteMembershipExpression()``. Using the previous example, the 
following Python example gets the fully resolved expression from the allFood 
collection.

.. code-block:: python

    collectionPrim = stage.GetPrimAtPath("/Food")
    allFoodCollection = Usd.CollectionAPI(collectionPrim, "allFood")
    resolvedExp = allFoodCollection.ResolveCompleteMembershipExpression()

Any expression references that can't be resolved are replaced with the
empty/nothing expression.

.. note::
    Normally you should not need to resolve the membership expression yourself, 
    since the ``CollectionMembershipQuery`` used to
    :ref:`query which objects belong to the collection <basic_usage>` 
    will do it for you. But if you are working with path expressions directly, 
    you can use ``Sdf.PathExpression`` APIs such as 
    :usdcpp:`Sdf.PathExpression.ResolveReferences()<SdfPathExpression::ResolveReferences>` 
    and :usdcpp:`Sdf.PathExpression.ComposeOver()<SdfPathExpression::ComposeOver>` 
    to do your own custom expression reference resolving.

.. _weaker_expression_references:

Working With Weaker Expression References
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is an additional convenience "weaker" expression reference which 
uses the ``%_`` syntax to reference a weaker expression. A weaker 
expression is an expression property opinion from a weaker composition arc
for the expression property making the reference. For example, you might have 
the following collection prim "Animals" defined in a layer:

.. code-block:: usda

    def "World" ()
    {
        def "Animals" (
            prepend apiSchemas = ["CollectionAPI:expTest"]
        )
        {
            pathExpression collection:expTest:membershipExpression = "/World/Animals/Fish/Halibut"

            def "Fish"
            {
                def "Tuna"
                {
                }
                def "Salmon"
                {
                }
                def "Halibut"
                {                
                }
            }
        }
    }

In another layer, you reference the :sdfpath:`/World/Animals` prim, but 
override the expTest path expression property:

.. code-block:: usda

    def "World" ()
    {
        def "Animals" (
            prepend apiSchemas = ["CollectionAPI:expTest"]
            references = @weakerSubRef.usda@</World/Animals>
        )
        {
            pathExpression collection:expTest:membershipExpression = "/World/Animals/Fish/Goldfish"

            def "Fish"
            {
                def "Goldfish"
                {
                }
            }
        }
    }

Querying the expTest collection will only match :sdfpath:`/World/Animals/Fish/Goldfish`. 
If you wanted to also include matches from the expTest expression in the 
referenced prim, you would use the ``%_`` syntax:

.. code-block:: usda

    def "World" ()
    {
        def "Animals" (
            prepend apiSchemas = ["CollectionAPI:expTest"]
            references = @weakerSubRef.usda@</World/Animals>
        )
        {
            pathExpression collection:expTest:membershipExpression = "/World/Animals/Fish/Goldfish + %_"

            def "Fish"
            {
                def "Goldfish"
                {
                }
            }
        }
    }

The expTest collection would now include matches from the weaker :sdfpath:`/World/Animals` 
reference, and the collection would match :sdfpath:`/World/Animals/Fish/Goldfish` 
and :sdfpath:`/World/Animals/Fish/Halibut`.

If USD is unable to resolve the weaker expression from the composed scene, the
``%_`` reference will be replaced with the empty/nothing expression.

.. _additional_considerations:

Additional Expressions Considerations
-------------------------------------

USD will automatically simplify expressions when possible. Leading and trailing 
whitespace is ignored, and expression logic is simplified. For example, an 
expression of ``// - /foo`` gets simplified to ``~/foo``
automatically. If you need to know the simplified expression, use 
``PathExpression.GetText()``:

.. code-block:: python

    testExpressionString = "// - /foo"  # This should get simplified to "~/foo"
    testExpression = Sdf.PathExpression(testExpressionString)
    print("Simplified expression text: " + testExpression.GetText()) # Should return "~/foo"

USD provides several convenience expressions, such as "Nothing" and "Everything"

.. code-block:: python

    pathExp = Sdf.PathExpression()            # Empty expression, matches nothing
    pathExp = Sdf.PathExpression.Nothing()    # Same as empty expression, matches nothing
    pathExp = Sdf.PathExpression.Everything() # Matches everything, equivalent to "//"

.. _configuring_pattern_based_collections:

Configuring Pattern-Based Collections
=====================================

Use the following built-in properties from the CollectionAPI schema to configure
membership in a pattern-based collection, where ``instanceName`` is the 
user-provided applied API schema instance name:

* ``uniform pathExpression collection:instanceName:membershipExpression``: 
  Defines the path expression used to test objects in the collection for 
  membership.

The following properties provide additional configuration aspects for 
collections: 

* ``uniform token collection:instanceName:expansionRule``: Specifies 
  specifies how matching scene objects against the ``membershipExpression`` 
  proceeds. Possible values include:

  * ``expandPrims``: The ``ComputeIncludedObjectsFromCollection()`` 
    and ``ComputeIncludedPathsFromCollection()`` CollectionAPI APIs only test 
    prims against the ``membershipExpression`` to determine membership. This
    is the default behavior.
  * ``expandPrimsAndProperties``: Like ``expandPrims``, but all properties on 
    all included prims also belong to the collection. 
  * ``explicitOnly``: Only applies to relationship-mode collections. For 
    pattern-based collections, if ``expansionRule`` is set to ``explicitOnly``, 
    the ``ComputeIncludedObjectsFromCollection()`` 
    and ``ComputeIncludedPathsFromCollection()`` CollectionAPI APIs return no
    results.

The following example shows a pattern-based collection configured to 
match prims (but not properties) that have the LightAPI schema applied, and
match the "/World/Light*" path expression.

.. code-block:: usda

    def "CollectionPrim" (
        prepend apiSchemas = ["CollectionAPI:myCollection"]
    )
    {
        # Set the collection expansion rule to specify how the paths for collection are expanded
        token collection:myCollection:expansionRule = "expandPrims"

        # Set myCollection:membershipExpression with the path expression we want
        pathExpression collection:myCollection:membershipExpression = "/World/Light*{hasAPI:LightAPI}"
    }

You can also use CollectionAPI methods to programmatically set the attribute:

.. code-block:: python

    collectionPrim = stage.DefinePrim("/CollectionPrim")
    myCollection = Usd.CollectionAPI.Apply(collectionPrim, "myCollection")
    myCollection.CreateExpansionRuleAttr(Usd.Tokens.expandPrims)  
    pathExp = Sdf.PathExpression("/World/Light*{hasAPI:LightAPI}") 
    expressionAttr = myCollection.CreateMembershipExpressionAttr(pathExp)

.. _expression_for_relationship_mode_collection:

Getting the Expression for a Relationship-Mode Collection
=========================================================

As mentioned earlier, collections can work in relationship-mode, using
include/exclude rules, or expression-mode, using path expressions. A collection
can't work in both modes. Additionally, you can't reference a relationship-mode
collection in an expression-mode collection's path expression. However, you
can have USD compute the equivalent expression for a relationship-mode
collection, and then reference that expression if needed. 

To compute the equivalent expression, use 
``ComputePathExpressionFromCollectionMembershipQueryRuleMap()``. The 
following example shows how to get the equivalent expression from a 
relationship-mode collection. 

.. code-block:: python

    # Get an existing relationship-mode collection
    collectionPrim = stage.GetPrimAtPath("/World/MyCollection")
    relationshipModeCollection = Usd.CollectionAPI(collectionPrim, "relationshipCollection")    

    # First compute the membership query for the relationship-mode collection
    collectionQuery = relationshipModeCollection.ComputeMembershipQuery()

    # Get the equivalent expression
    equivalentExpression = Usd.ComputePathExpressionFromCollectionMembershipQueryRuleMap(
                collectionQuery.GetAsPathExpansionRuleMap())

    # ...use equivalentExpression as needed...

Once you have the equivalent expression, you can author that expression on the
``membershipExpression`` of a new or existing collection. You can use the 
equivalent expression to convert a relationship-mode collection to an equivalent 
expression-mode collection by authoring ``membershipExpression`` with the
equivalent expression **and** clearing out the ``includes``, ``excludes``, and 
``includeRoot`` relationship-mode properties.
