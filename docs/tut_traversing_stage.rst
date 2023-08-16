.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst             

==================
Traversing a Stage
==================

This tutorial covers API for traversing the composed prims on a
:usdcpp:`UsdStage`. This is the typical pattern for implementing imaging clients,
USD importers to other DCC applications, etc. We will use the layers produced in
:doc:`tut_referencing_layers`. You will need the referenced layer,
:filename:`HelloWorld.usda`, and the referencing layer,
:filename:`RefExample.usda`, which are in the
:filename:`USD/extras/usd/tutorials/traversingStage` folder. Please copy that
folder to a working directory and make its contents writable.


#. Open RefExample.usda in usdview and bring up the interpreter by hitting
   :kbd:`i` or the :menuselection:`Window --> Interpreter` menu item.

#. The interpreter has a built-in :python:`usdviewApi` object that contains a
   number of convenient variables and functions for working with usdview and the
   :usdcpp:`UsdStage` it has open.

   The simplest place to start is with :python:`Usd.Stage.Traverse()`; a
   generator which yields prims from the stage in depth-first-traversal order.

   .. code-block:: pycon 

      >>> [x for x in usdviewApi.stage.Traverse()]
      [Usd.Prim(</refSphere>), Usd.Prim(</refSphere/world>),
       Usd.Prim(</refSphere2>), Usd.Prim(</refSphere2/world>)]

   You can filter as you would any other Python generator:
   
   .. code-block:: pycon

      >>> [x for x in usdviewApi.stage.Traverse() if UsdGeom.Sphere(x)]
      [Usd.Prim(</refSphere/world>), Usd.Prim(</refSphere2/world>)]

#. For more involved traversals, the :python:`Usd.PrimRange()` exposes pre- and
   post-order prim visitations.

   .. code-block:: pycon

      >>> primIter = iter(Usd.PrimRange.PreAndPostVisit(usdviewApi.stage.GetPseudoRoot()))
      >>> for x in primIter: print(x, primIter.IsPostVisit())
      Usd.Prim(</>) False
      Usd.Prim(</refSphere>) False
      Usd.Prim(</refSphere/world>) False
      Usd.Prim(</refSphere/world>) True
      Usd.Prim(</refSphere>) True
      Usd.Prim(</refSphere2>) False
      Usd.Prim(</refSphere2/world>) False
      Usd.Prim(</refSphere2/world>) True
      Usd.Prim(</refSphere2>) True
      Usd.Prim(</>) True

#. :python:`Usd.PrimRange` also makes prim-flag predicates available. In fact,
   :python:`Usd.Stage.Traverse()` is really a convenience method that performs
   pre-order visitations on all the prims in the composed scenegraph that are
   active, defined, loaded, and concrete.

   In :doc:`tut_referencing_layers`, we touched upon what it means for a prim to
   be defined, i.e., it is backed by a def rather than an over. The concepts of
   being loaded and concrete respectively correlate to payloads (composition
   boundaries across which scene description does not get composed unless
   explicitly requested) and classes (abstract prims whose opinions apply to all
   prims which inherit from them). We will discuss these operators more in depth
   in a future tutorial.  Here we consider activation/deactivation semantics
   with usdview.

   Select refSphere2 in the tree view on the left and choose
   :menuselection:`Edit --> Deactivate` from the menu. The result should look
   like this.

   .. image:: http://openusd.org/images/tut_traversing_stage_refexample.png

   You can inspect the contents of the 
   `session layer <#usdglossary-sessionlayer>`_ in the interpreter to see
   the authored deactivation opinion.

   .. code-block:: pycon

      >>> print(usdviewApi.stage.GetSessionLayer().ExportToString())
      #usda 1.0

      over "refSphere2" (
          active = false
      )
      {
      }

   In USD, a session layer holds transient opinions for the current working
   session that a user would not want to commit to the asset and thereby affect
   downstream departments.

   Now :code:`Traverse()` will not visit refSphere2 or any of its namespace
   children, and renderers do not draw it.

   .. code-block:: pycon

      >>> [x for x in usdviewApi.stage.Traverse()]
      [Usd.Prim(</refSphere>), Usd.Prim(</refSphere/world>)]

   While we can still see refSphere2 as an inactive prim on the stage, its
   children no longer have any presence in the composed scenegraph. We can use
   :code:`TraverseAll()` to get an iterator with no predicates applied to
   verify this.

   .. code-block:: pycon

      >>> [x for x in usdviewApi.stage.TraverseAll()]
      [Usd.Prim(</refSphere>), Usd.Prim(</refSphere/world>),
       Usd.Prim(</refSphere2>)]

       

