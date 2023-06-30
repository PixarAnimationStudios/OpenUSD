.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst

==================
Referencing Layers
==================

This tutorial walks you through referencing the stage that we created in the
previous tutorials into a new stage. The :filename:`HelloWorld.usda` that we
will use as our starting point and all the code for this exercise is in the
:filename:`extras/usd/tutorials/referencingLayers` folder.

#. Author the :usda:`defaultPrim` metadata on the layer that you want to
   reference. This is the name of the root prim that will be referenced. If it
   is not authored here, then the referencing client must specify the root prim
   path it wants from the referenced layer. We also want to author a
   transformation on the default root prim, so that we can override it later; to
   do this we use the :usdcpp:`UsdGeomXformCommonAPI schema <UsdGeomXformCommonAPI>`,
   which we will discuss further, later.

   .. code-block:: python

      from pxr import Usd, UsdGeom
      stage = Usd.Stage.Open('HelloWorld.usda')
      hello = stage.GetPrimAtPath('/hello')
      stage.SetDefaultPrim(hello)
      UsdGeom.XformCommonAPI(hello).SetTranslate((4, 5, 6))
      print(stage.GetRootLayer().ExportToString())
      stage.GetRootLayer().Save()

   produces

   .. code-block:: usd

      #usda 1.0
      (
          defaultPrim = "hello"
      )

      def Xform "hello"
      {
          double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = ["xformOp:translate"]

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor = [(0, 0, 1)]
              double radius = 2
          }
      }

#. Now let's create a new stage to reference in HelloWorld.usda and create an
   override prim to contain the reference.

   .. code-block:: python

      refStage = Usd.Stage.CreateNew('RefExample.usda')
      refSphere = refStage.OverridePrim('/refSphere')
      print(refStage.GetRootLayer().ExportToString())

   produces

   .. code-block:: usd

      #usda 1.0

      over "refSphere"
      {
      }

   All of the previous prims we had created are :usda:`def` s , which are
   concrete prims that appear in standard scenegraph traversals (i.e. by clients
   performing imaging, or importing the stage into another DCC application).  By
   contrast, an :usda:`over` can be thought of as containing a set of
   *speculative opinions* that are applied over any concrete prims that may be
   defined in other layers at the corresponding namespace location in a composed
   stage. Overs can contain opinions for any property, metadata, or prim
   composition operators. For example, an over can non-destructively express a
   different opinion for the transform and displayColor attributes above.

#. Let's reference in the stage from HelloWorld.

   .. code-block:: python

      refSphere.GetReferences().AddReference('./HelloWorld.usda')
      print(refStage.GetRootLayer().ExportToString())
      refStage.GetRootLayer().Save()

   produces

   .. code-block:: usd

      #usda 1.0

      over "refSphere" (
          prepend references = @./HelloWorld.usda@
      )
      {
      }

   .. admonition:: Asset Path Resolver and File Format Plugins

      In this example we use a filename to reference the layer.  In practice,
      the layer identifier passed to :python:`Usd.References.AddReference()` can
      be any string that a path resolver plugin can resolve and a scene
      description file format plugin would process to populate the actual scene
      description. USD supports user-implementable asset path resolver and file
      format plugins to allow site-specific customization and pipeline
      integration.  USD does not require that layers be files on disk.  See `the
      Ar library documentation
      <api/ar_page_front.html>`_ for more
      about asset resolvers, and see the code in
      :filename:`USD/extras/usd/examples/usdObj/` for an example file format
      plugin.

   Note that since we authored :usda:`defaultPrim` in
   :filename:`HelloWorld.usda`, we only need to specify the root layer we want
   to reference, and it is inferred that we will be bringing in the scenegraph
   contents rooted at :sdfpath:`/hello` into our :sdfpath:`/refSphere`.

   Running usdview on the exported :filename:`RefExample.usda` shows the
   composed result.

   .. image:: http://openusd.org/images/tut_referencing_layers_refexample.png

   If it were unselected in the namespace browser, usdview would show
   :sdfpath:`/refSphere` in orange to indicate that it is a referencing point on
   our stage. Our screenshot shows refSphere's row selected, however, to show
   both what our overridden transformation looks like as attributes, and to note
   in the **Meta Data** inspector that the reference and its target are listed.
   Note also that there is no prim named "hello" since it has been referenced
   into :sdfpath:`/refSphere`.
   
#. Let's reset the transform on our over to the identity.

   .. code-block:: python

      refXform = UsdGeom.Xformable(refSphere)
      refXform.SetXformOpOrder([])
      print(refStage.GetRootLayer().ExportToString())

   produces

   .. code-block:: usd

      #usda 1.0

      over "refSphere" (
          prepend references = @./HelloWorld.usda@
      )
      {
          uniform token[] xformOpOrder = []
      }

   What Just Happened?

   The `UsdGeomXformable
   <api/class_usd_geom_xformable.html#details>`_
   schema is component-based, allowing you to specify a single 4x4 matrix, or an
   unlimited sequence of translate, rotate, scale, matrix, and (quaternion)
   orientation "ops". Each xformable prim has a builtin :usda:`xformOpOrder`
   attribute that specifies the order in which the ops are applied. By
   explicitly setting the ordering to an empty list as in:

   :python:`refXform.SetXformOpOrder([])`
   
   we are telling the schema to ignore any ops, even if authored, effectively
   setting the transformation to the identity. We could also have explicitly
   authored an identity matrix, or set all existing, composed op attributes to
   their identity values. For a complete explanation of :code:`Xformable` and
   :code:`XformOps`, please see the `API documentation for UsdGeomXformable
   <api/class_usd_geom_xformable.html#details>`_

#. Reference in another HelloWorld.

   .. code-block:: python

      refSphere2 = refStage.OverridePrim('/refSphere2')
      refSphere2.GetReferences().AddReference('./HelloWorld.usda')
      print(refStage.GetRootLayer().ExportToString())
      refStage.GetRootLayer().Save()

   produces

   .. code-block:: usd

      #usda 1.0

      over "refSphere" (
          prepend references = @./HelloWorld.usda@
      )
      {
          uniform token[] xformOpOrder = []
      }

      over "refSphere2" (
          prepend references = @./HelloWorld.usda@
      )
      {
      }

   .. image:: http://openusd.org/images/tut_referencing_layers_xform.png

   We can see that our over has been applied to move the first sphere to the
   origin, while the second sphere is still translated by :code:`(4, 5, 6)`.

#. Of course, overs can be authored for the actual sphere prims underneath the
   reference as well. Let's color our second sphere red.

   .. code-block:: python

      overSphere = UsdGeom.Sphere.Get(refStage, '/refSphere2/world')
      overSphere.GetDisplayColorAttr().Set( [(1, 0, 0)] )
      print(refStage.GetRootLayer().ExportToString())
      refStage.GetRootLayer().Save()

   Note that we don't need to call OverridePrim again because
   :usda:`/refSphere2/world` already has a presence in the composed scenegraph.
   USD automatically creates an :usda:`over` when :python:`Usd.Attribute.Set()`
   is called if one doesn't already exist for that prim in the current authoring
   layer.

   .. code-block:: usd

      #usda 1.0

      over "refSphere" (
          prepend references = @./HelloWorld.usda@
      )
      {
          uniform token[] xformOpOrder = []
      }

      over "refSphere2" (
          prepend references = @./HelloWorld.usda@
      )
      {
          over "world"
          {
              color3f[] primvars:displayColor = [(1, 0, 0)]
          }
      }

   .. image:: http://openusd.org/images/tut_referencing_layers_color.png

#. We can also flatten the composed results.  All of the scene description
   listings above were of the stage's root layer, where we performed our
   authoring. Calling :usdcpp:`ExportToString() <UsdStage::ExportToString>` or
   :usdcpp:`Export() <UsdStage::Export>` on the :usdcpp:`UsdStage` itself, will
   print or save out flattened scene description, respectively.
   
   flattening
      The term *flattening* generally means producing a single Layer of scene
      description that contains the final "composed data" from a set of composed
      layers, **and retains no composition operators** such as references,
      payloads, inherits, variants, sublayers, and activations (except
      references generated in order to preserve :ref:`scene graph instancing
      <glossary:Instancing>`). :usdcpp:`UsdStage::Flatten`
      flattens an entire stage and is used by :code:`Export()` and
      :code:`ExportToString()`.  USD also supports flattening individual
      :ref:`layer stacks <glossary:LayerStack>`.  See 
      :usdcpp:`UsdFlattenLayerStack`.

   .. code-block:: python

      print(refStage.ExportToString())

   .. code-block:: usd
                       
      #usda 1.0
      (
          doc = """Generated from Composed Stage of root layer RefExample.usda
      """
      )

      def Xform "refSphere"
      {
          double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = []

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor = [(0, 0, 1)]
              double radius = 2
          }
      }

      def Xform "refSphere2"
      {
          double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = ["xformOp:translate"]

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor = [(1, 0, 0)]
              double radius = 2
          }
      }

       

