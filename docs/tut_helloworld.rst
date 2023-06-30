.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst             

===========================================
Hello World - Creating Your First USD Stage
===========================================

This tutorial walks through creating a simple USD stage containing a transform
and a sphere.

Python for USD
##############

#. Open :filename:`USD/extras/usd/tutorials/helloWorld/helloWorld.py` to see the
   Python script that creates and exports the stage. It should look like the
   following.

   .. code-block:: python

      from pxr import Usd, UsdGeom
      stage = Usd.Stage.CreateNew('HelloWorld.usda')
      xformPrim = UsdGeom.Xform.Define(stage, '/hello')
      spherePrim = UsdGeom.Sphere.Define(stage, '/hello/world')
      stage.GetRootLayer().Save()

#. Execute the Python script to create :filename:`HelloWorld.usda`.

   .. code-block:: sh

      $ python extras/usd/tutorials/helloWorld/helloWorld.py

       

Visualizing the stage
#####################

Use :ref:`toolset:usdview` to visualize and inspect the stage.

#. Open the stage in usdview:

   .. code-block:: sh

      $ usdview HelloWorld.usda

   .. image:: http://openusd.org/images/tut_helloworld_sphere_1.png

#. You can refine the geometry with the :menuselection:`View --> Complexity`
   menu item or use the hotkeys :kbd:`Ctrl-+` and :kbd:`Ctrl--` to increase or
   decrease the refinement.

   .. image:: http://openusd.org/images/tut_helloworld_sphere_2.png

#. You can also bring up an embedded Python interpreter by pressing :kbd:`i` or
   using the :menuselection:`Window --> Interpreter` menu item. This interpreter
   has a built-in API object, :python:`usdviewApi`, that contains some useful
   variables. One is :python:`usdviewApi.prim`, which refers to the first prim,
   hierarchically, in the set of currently selected prims.

   Select the sphere either by clicking it in the viewport or by clicking its
   name, :usda:`world`, in the tree view on the left.  Then try the following
   commands:

   >>> usdviewApi.prim
   Usd.Prim(</hello/world>)
   >>> usdviewApi.prim.GetTypeName()
   'Sphere'
   >>> usdviewApi.prim.GetAttribute('radius').Get()
   1.0

Viewing and editing USD file contents
#####################################

The exported file is human-readable via :ref:`toolset:usdcat` and text-editable
via :ref:`toolset:usdedit` (both available in :filename:`USD_INSTALL_ROOT/bin`
in the default installation).  The :ref:`toolset:usdedit` program will bring up
any .usd file as plain text in your :envvar:`EDITOR` regardless of its
underlying format, and save it back out in its original format when editing is
complete.  See :ref:`toolset:usdedit` for more details.

This particular example can be edited in a text editor directly since we created
a text-based USD file with the :filename:`.usda` extension.  If we had created a
binary usd file with the :filename:`.usdc` extension instead, both
:ref:`toolset:usdcat` and :ref:`toolset:usdedit` would work just the same on it.

.. code-block:: usda

   #usda 1.0
   
   def Xform "hello"
   {
       def Sphere "world"
       {
       }
   }

.. admonition:: Further Reading
   
   * :usdcpp:`UsdStage` is the outermost container for scene description, which
     owns and presents composed prims as a scenegraph.

   * :usdcpp:`UsdPrim` is the sole persistent scenegraph object on a
     :usdcpp:`UsdStage`.

