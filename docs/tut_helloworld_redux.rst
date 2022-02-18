.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst

=======================================
Hello World Redux - Using Generic Prims
=======================================

In this tutorial, we revisit the :doc:`Hello World! <tut_helloworld>` example,
but this example passes in hard-coded typenames :usda:`Xform` and
:usda:`Sphere`, which correspond to the typenames appearing after the
:usda:`def` specifiers in the :usda:`usda` scene description representation.

The :code:`UsdGeom` API used in the :doc:`previous tutorial <tut_helloworld>`,
is part of USD's built-in geometry schema.  These prim types and associated C++
and Python API have first-class support in USD and provide domain-specific
interfaces to create, manipulate, introspect and author properties upon them.

Note that :usdcpp:`UsdGeomXform::Define` returns a schema object on which the
`full schema-specific API <api/usd_geom_page_front.html>`_ is available, as
opposed to :usdcpp:`UsdStage::DefinePrim` which returns a generic
:usdcpp:`UsdPrim`.  In USD, all schema classes have a :code:`GetPrim()` member
function that returns its underlying :usdcpp:`UsdPrim`. Think of a
:usdcpp:`UsdPrim` as the object's generic, persistent presence in the
scenegraph, and the schema object as the first-class way to access its
domain-specific data and functionality.

.. code-block:: python

   from pxr import Usd
   stage = Usd.Stage.CreateNew('HelloWorldRedux.usda')
   xform = stage.DefinePrim('/hello', 'Xform')
   sphere = stage.DefinePrim('/hello/world', 'Sphere')
   stage.GetRootLayer().Save()

The code above produces identical scene description to the previous tutorial,
which you can see using :ref:`toolset:usdcat`:

.. code-block:: usda

   #usda 1.0

   def Xform "hello"
   {
       def Sphere "world"
       {
       }
   }

In the :doc:`next tutorial <tut_inspect_and_author_props>`, we'll look at
UsdGeom API in action on prim properties.

