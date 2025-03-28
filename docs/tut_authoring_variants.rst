.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst             

==================
Authoring Variants
==================

This tutorial walks through authoring a variant set on the HelloWorld layer from
:doc:`tut_inspect_and_author_props`. We begin with the layer
:filename:`USD/extras/usd/tutorials/authoringVariants/HelloWorld.usda` and the
Python code :filename:`authorVariants.py` in the same directory.  Please copy
these files to a working directory and make them writable.

#. Open the stage and clear the :usda:`Sphere`'s authored :usda:`displayColor`
   opinion. In USD *local* opinions are stronger than *variant* opinions.  So if
   we do not clear this opinion it will win over those we author within variants
   below.

   To see this in action, comment out the :python:`Clear()` and run the
   tutorial.  The steps below still author the same variant opinions, but the
   composed result shows the locally-authored blue color.  That stronger local
   opinion overrides the opinions from variants.

   See :ref:`usdglossary-livrpsstrengthordering` for more details on
   strength order in USD.

   .. code-block:: python

      from pxr import Usd, UsdGeom
      stage = Usd.Stage.Open('HelloWorld.usda')
      colorAttr = UsdGeom.Gprim.Get(stage, '/hello/world').GetDisplayColorAttr()
      colorAttr.Clear()
      print(stage.GetRootLayer().ExportToString())

   The :usda:`</hello/world>` prim is a :usda:`Sphere`; a schema subclass of
   :usda:`Gprim`.  So we can use the :python:`UsdGeom.Gprim` API to work with
   it.  In USD, :usda:`Gprim` defines :usda:`displayColor` so all gprim
   subclasses have it.

   .. code-block:: usd
      :emphasize-lines: 14

      #usda 1.0
      (
          defaultPrim = "hello"
      )

      def Xform "hello"
      {
          custom double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = ["xformOp:translate"]

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor
              double radius = 2
          }
      }

   The resulting scene description still has the declaration for the
   displayColor attribute; a future iteration on the :python:`Usd.Attribute` API
   may clean this up.

#. Create a shading variant on the root prim called :usda:`shadingVariant`

   .. code-block:: python

      rootPrim = stage.GetPrimAtPath('/hello')
      vset = rootPrim.GetVariantSets().AddVariantSet('shadingVariant')
      print(stage.GetRootLayer().ExportToString())

   produces

   .. code-block:: usd
      :emphasize-lines: 7

      #usda 1.0
      (
          defaultPrim = "hello"
      )

      def Xform "hello" (
          prepend variantSets = "shadingVariant"
      )
      {
          custom double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = ["xformOp:translate"]

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor
              double radius = 2
          }
      }

#. Create variants within 'shadingVariant' to contain the opinions we will
   author.

   .. code-block:: python

      vset.AddVariant('red')
      vset.AddVariant('blue')
      vset.AddVariant('green')
      print(stage.GetRootLayer().ExportToString())

   produces

   .. code-block:: usd
      :emphasize-lines: 20-29

      #usda 1.0
      (
          defaultPrim = "hello"
      )

      def Xform "hello" (
          prepend variantSets = "shadingVariant"
      )
      {
          custom double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = ["xformOp:translate"]

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor
              double radius = 2
          }

          variantSet "shadingVariant" = {
              "blue" {
              }

              "green" {
              }

              "red" {
              }
          }
      }

#. Author a red color to the sphere prim :usda:`</hello/world>` in the
   :usda:`red` variant.

   To author opinions inside the :usda:`red` variant we first select the
   variant, then use a :python:`Usd.EditContext`.  This directs editing into
   the variant.
   
   .. code-block:: python

      vset.SetVariantSelection('red')
      with vset.GetVariantEditContext():
          colorAttr.Set([(1,0,0)])

      print(stage.GetRootLayer().ExportToString())

   If we just invoked :python:`colorAttr.Set()` without using the variant edit
   context, we would write a *local* opinion; specifically the same opinion we
   cleared at the beginning.
   
   .. code-block:: usd
      :emphasize-lines: 7-9, 30-35

      #usda 1.0
      (
          defaultPrim = "hello"
      )

      def Xform "hello" (
          variants = {
              string shadingVariant = "red"
          }
          prepend variantSets = "shadingVariant"
      )
      {
          custom double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = ["xformOp:translate"]

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor
              double radius = 2
          }

          variantSet "shadingVariant" = {
              "blue" {
              }

              "green" {
              }

              "red" {
                  over "world"
                  {
                      color3f[] primvars:displayColor = [(1, 0, 0)]
                  }
              }
          }
      }

   Now :usda:`hello` has a :usda:`shadingVariant` with a default variant
   selection of :usda:`red`, and the :usda:`red` variant has an opinion for
   :usda:`displayColor`.

#. Author the blue and green variants similarly.

   .. code-block:: python

      vset.SetVariantSelection('blue')
      with vset.GetVariantEditContext():
          colorAttr.Set([(0,0,1)])

      vset.SetVariantSelection('green')
      with vset.GetVariantEditContext():
          colorAttr.Set([(0,1,0)])

      print(stage.GetRootLayer().ExportToString())

   produces

   .. code-block:: usd 
      :emphasize-lines: 24-36

      #usda 1.0
      (
          defaultPrim = "hello"
      )

      def Xform "hello" (
          variants = {
              string shadingVariant = "green"
          }
          prepend variantSets = "shadingVariant"
      )
      {
          custom double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = ["xformOp:translate"]

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor
              double radius = 2
          }

          variantSet "shadingVariant" = {
              "blue" {
                  over "world"
                  {
                      color3f[] primvars:displayColor = [(0, 0, 1)]
                  }
              }

              "green" {
                  over "world"
                  {
                      color3f[] primvars:displayColor = [(0, 1, 0)]
                  }
              }

              "red" {
                  over "world"
                  {
                      color3f[] primvars:displayColor = [(1, 0, 0)]
                  }
              }
          }
      }

   As mentioned at the beginning, it was important to first :code:`Clear()` the
   *local* opinion for :usda:`primvars:displayColor` because **local opinions
   are strongest**. Had we not cleared the local opinion, the sphere would
   remain blue regardless of the selected :usda:`shadingVariant` for this
   reason.

   .. admonition:: Opinion Strength Order

      :ref:`Strength order <usdglossary-livrpsstrengthordering>` is a fundamental
      part of USD.

#. Examine the composed result.

   .. code-block:: python

      print(stage.ExportToString(addSourceFileComment=False))

   shows

   .. code-block:: usd

      #usda 1.0
      (
          defaultPrim = "hello"
      )

      def Xform "hello"
      {
          custom double3 xformOp:translate = (4, 5, 6)
          uniform token[] xformOpOrder = ["xformOp:translate"]

          def Sphere "world"
          {
              float3[] extent = [(-2, -2, -2), (2, 2, 2)]
              color3f[] primvars:displayColor = [(0, 1, 0)]
              double radius = 2
          }

      }
      
   What happened to the variants? :usdcpp:`Exporting a UsdStage
   <UsdStage::Export>` writes the composed scene description with all the
   composition operators (like variants) fully evaluated.  We call this
   :ref:`flattening <glossary:Flatten>`.  :ref:`Flattening <glossary:Flatten>`
   the composition applies the opinions from the currently selected variants so
   we only get the green color due to the last call to
   :usdcpp:`SetVariantSelection() <UsdVariantSet::SetVariantSelection>`.

#. Save the edited authoring layer to a new file, :filename:`HelloWorldWithVariants.usda`

   In contrast to exporting a :usdcpp:`UsdStage`, :usdcpp:`exporting
   an individual layer <SdfLayer::Export>` writes its contents as-is to a
   different file.  There is no flattening.

   .. code-block:: python

      stage.GetRootLayer().Export('HelloWorldWithVariants.usda')

   We could also call :code:`Save()` to update the original file with the new
   content.

#. Run :ref:`toolset:usdview` on :filename:`HelloWorldWithVariants.usda`.

   .. image:: http://openusd.org/images/tut_authoring_variants_helloworld1.png

   Click the :usda:`hello` prim in the tree view on the left.  Then click the
   :guilabel:`Meta Data` tab in the lower right.  You will see the authored
   variant set with a combo box that can toggle the variant selection between
   red, blue, and green.

   .. image:: http://openusd.org/images/tut_authoring_variants_helloworld2.png
                
#. In the interpreter you can see the variant selections that usdview authors to
   the :ref:`session layer <usdglossary-sessionlayer>`. This is the same sparse 
   override you would see if you referenced this layer into another one and 
   authored the variant selection in the referencing layer.

   .. code-block:: python

      print(usdviewApi.stage.GetSessionLayer().ExportToString())

   shows

   .. code-block:: python

      #usda 1.0

      over "hello" (
          variants = {
              string shadingVariant = "red"
          }
      )
      {
      }

