.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst

===================================
Inspecting and Authoring Properties
===================================

In this tutorial we look at properties containing geometric data on the prims we
created in the :doc:`Hello World <tut_helloworld>` tutorial. The starting layer for
this exercise is
:filename:`extras/usd/tutorials/authoringProperties/HelloWorld.usda`.

This tutorial is available in
:filename:`extras/usd/tutorials/authoringProperties/authorProperties.py`. You
can follow along with this script in Python.

Tutorial
********

#. Open the stage and get the prims defined on the stage.

   .. code-block:: python

      from pxr import Usd, Vt
      stage = Usd.Stage.Open('HelloWorld.usda')
      xform = stage.GetPrimAtPath('/hello')
      sphere = stage.GetPrimAtPath('/hello/world')

#. List the available property names on each prim.

   .. code-block:: pycon

      >>> xform.GetPropertyNames()
      ['proxyPrim', 'purpose', 'visibility', 'xformOpOrder']
      >>> sphere.GetPropertyNames()
      ['doubleSided', 'extent', 'orientation', 'primvars:displayColor',
      'primvars:displayOpacity', 'proxyPrim', 'purpose', 'radius',
      'visibility', 'xformOpOrder']
   
#. Read the extent attribute on the sphere prim.

   .. code-block:: pycon

      >>> extentAttr = sphere.GetAttribute('extent')
      >>> extentAttr.Get()
      Vt.Vec3fArray(2, (Gf.Vec3f(-1.0, -1.0, -1.0), Gf.Vec3f(1.0, 1.0, 1.0)))

   This returns a two-by-three array containing the endpoints of the sphere's
   axis-aligned, object-space extent, as expected for the *fallback* value of
   this attribute.  An attribute has a fallback value when no opinions about its
   value are authored in the scene description.  This fallback value for
   :usda:`extent` is sensible given that the fallback value for a sphere's
   :usda:`radius` is 1.0.

   A call to :python:`Usd.Attribute.Get()` with no arguments returns the
   attribute's value at the :usdcpp:`Default time <UsdTimeCode::Default>`.
   Attributes can also have time-sampled values.  For example, we can create an
   animated deforming object by providing time sample values for a
   :python:`UsdGeom.Mesh`'s :usda:`points` attribute. (See
   :doc:`tut_end_to_end` for a mock-up of this pipeline in action).
      
   .. admonition:: API Tip

      :usdcpp:`UsdPrim::GetPropertyNames`
      demonstrates that we can fetch properties by name. In practice, to iterate
      over a prim's properties it is generally more convenient to use one of
      :usdcpp:`UsdPrim::GetProperties`, :usdcpp:`UsdPrim::GetAttributes`,
      or :usdcpp:`UsdPrim::GetRelationships`.
      These return :usdcpp:`UsdProperty`, :usdcpp:`UsdAttribute`, and
      :usdcpp:`UsdRelationship` objects that one can operate on directly.

#. Set the sphere's radius to 2.

   Since geometric extents are not automatically recomputed, we must also update
   the sphere's extent to reflect its new size.  See `more details here
   <api/class_usd_geom_boundable.html#details>`_.

   .. code-block:: pycon

      >>> radiusAttr = sphere.GetAttribute('radius')
      >>> radiusAttr.Set(2)
      True
      >>> extentAttr.Set(extentAttr.Get() * 2)
      True

   Like :code:`Get()`, a call to :code:`Set()` with a value argument and no time
   argument authors the value at the Default time. The resulting scene
   description is:

   .. code-block:: pycon

      >>> print(stage.GetRootLayer().ExportToString())
      #usda 1.0
      
      def Xform "hello"
      {
          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              double radius = 2
          }
      }

#. Author a displayColor on the sphere.

   As with :doc:`tut_helloworld_redux`, we can do so using the :code:`UsdGeom`
   schema API. One benefit to this approach is that the first-class API hides
   the fact that the attribute's raw name is :usda:`primvars:displayColor`.
   This frees client code from having to know that detail.

   .. code-block:: pycon

      >>> from pxr import UsdGeom
      >>> sphereSchema = UsdGeom.Sphere(sphere)
      >>> color = sphereSchema.GetDisplayColorAttr()
      >>> color.Set([(0,0,1)])
      True

   Note that the color value is a vector of triples. This is because the
   primvars:displayColor attribute can represent either a single color for the
   whole prim, or a color per prim element (e.g. mesh face). The resulting scene
   description is:

   .. code-block:: pycon

      >>> print(stage.GetRootLayer().ExportToString())
      #usda 1.0

      def Xform "hello"
      {
          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor = [(0, 0, 1)]
              double radius = 2
          }
      }

#. Save your edits.
   
   .. code-block:: pycon

       >>> stage.GetRootLayer().Save()

#. Here is the result in usdview.

   .. image:: http://openusd.org/images/tut_inspect_and_author_props_usdview.png
   
   The camera automatically frames the geometry, but we can see that the sphere
   is larger than in the last tutorial by inspecting its attributes in the
   Attribute browser.

.. admonition:: Further Reading
   
   * :usdcpp:`UsdProperty` provides access to authoring and interrogating
     properties and their common metadata
   
   * :usdcpp:`UsdAttribute` refines UsdProperty with specific API for
     time-sampled access to typed attribute data
   
   * :usdcpp:`UsdRelationship` refines UsdProperty with API to target other
     prims and properties, and resolve those targets robustly, and through
     chains of relationships.
   
   * Properties are ordered in dictionary order, by default, but one can
     explicitly order properties using :usdcpp:`UsdPrim::SetPropertyOrder`.
   

  
