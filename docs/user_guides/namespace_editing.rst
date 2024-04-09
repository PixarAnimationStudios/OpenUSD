.. include:: ../rolesAndUtils.rst

.. _namespace_editing:

#################
Namespace Editing
#################

A namespace edit is an operation that removes or changes the namespace path of 
a **composed** prim or property on a stage. Edit operations currently include 
deleting and moving (renaming and/or reparenting) a composed prim or property.

While it is possible to do some of these operations using Sdf or UsdStage APIs, 
these APIs can only edit prims or properties on a single layer. Namespace 
editing handles edits for prims and properties that are composed from multiple 
layers or other composition arcs. Using namespace editing lets you robustly
delete and move prims that have opinions throughout a LayerStack that you have
the authority to edit. Namespace editing also allows "non-destructive" 
deletion and moving of prims defined in LayerStacks that you don't have the 
authority to edit.

As a simple example, if you had :filename:`model.usda` with various prims:

.. code-block:: usda
    :caption: model.usda

    #usda 1.0 
    (
    )

    def Scope "modelScope" (
    )
    {
        def "model1" (
        )
        {
            custom string department = "department A"
        }
    }

And :filename:`main.usda`, which has :filename:`model.usda` as a sublayer and
overs for :sdfpath:`/modelScope` and :sdfpath:`/modelScope/model1`:

.. code-block:: usda
    :caption: main.usda

    #usda 1.0 
    (
        subLayers = [
            @model.usda@
        ]
    )

    over "modelScope" ()
    {
        over "model1" ()
        {
            custom string department = "department N"
        }
    }

After loading :filename:`main.usda` into your stage, if you wanted to fully 
remove :sdfpath:`/modelScope/model1` from your composed stage, using UsdStage 
APIs, you'd have to delete it twice: once in :filename:`main.usda`, and then, 
after changing edit targets, again in the :filename:`model.usda` layer:

.. code-block:: python

    stage = Usd.Stage.Open("main.usda")

    # This only removes model1 in model.usda, not
    # model1 in the model.usda layer
    removeResult = stage.RemovePrim("/modelScope/model1")  

    # After the RemovePrim() call, we still have /modelScope/model1
    # from model.usda in our composed stage, and would have to 
    # set the edit target to the model.usda layer and call RemovePrim() again

Whereas using namespace editing will properly handle removing the prim from
both :filename:`main.usda` and :filename:`model.usda`, handling edit target
changes for you:

.. code-block:: python

    editor = Usd.NamespaceEditor(stage)
    removeResult = editor.DeletePrimAtPath("/modelScope/model1")
    editor.ApplyEdits()

Namespace editing also handles issues such as making sure that paths to 
and overrides of edited objects are still valid after renaming or reparenting 
prims. Namespace editing tries to fix any existing composition arcs, 
relationship targets, attribute connections, and overrides that used paths to 
renamed or reparented prims to use the new paths.

Namespace editing in a future update will also use **relocates** for more 
complex edit scenarios. Relocates are another composition arc that maps a prim 
path in a namespace to a new path location. For example, if you were referencing 
a prim from :filename:`refModel.usda`:

.. code-block:: usda
    :caption: refModel.usda

    def "modelA" ()
    {
        def "modelAChild" ()
        {
        }
    }

.. code-block:: usda
    :caption: main.usda

    def "mainModelA" (
        prepend references = @refModel.usda@</modelA>
    )
    {

You will be able to use namespace editing to move or rename 
:sdfpath:`/mainModelA/modelAChild` using relocates. 

.. code-block:: python

    # Rename /mainModelA/modelAChild to /mainModelA/renamedChild
    # Namespace editing will use relocates for this edit
    removeResult = editor.MovePrimAtPath("/mainModelA/modelAChild", "/mainModelA/renamedChild")
    editor.ApplyEdits()

************************
Using UsdNamespaceEditor
************************

To use namespace editing, first create a namespace editor instance for the stage 
you want to perform edit operations on:

.. code-block:: python

    stage = Usd.Stage.Open("main.usda")
    editor = Usd.NamespaceEditor(stage)

:usdcpp:`UsdNamespaceEditor` provides methods for deleting and moving prims or 
properties. Methods for deleting prims and properties can use paths, or 
:usdcpp:`UsdPrim`/:usdcpp:`UsdProperty` instances. Methods for moving prims only 
use paths, but :usdcpp:`UsdNamespaceEditor` also provides convenience methods to 
rename or reparent prims and properties using :usdcpp:`UsdPrim` and 
:usdcpp:`UsdProperty` instances. 

When you call a :usdcpp:`UsdNamespaceEditor` edit operation, the operation paths 
are validated (e.g. the paths passed to :code:`MovePrimAtPath()` are checked to 
make sure they are valid paths), and then the operation is queued (to support 
batches of operations, see :ref:`namespace_editing_batch_edits` below). To 
execute individual edit operations, as the following examples do, call 
:code:`ApplyEdits()` after each operation call.

.. code-block:: python

    # Delete a prim at a specific path
    editor.DeletePrimAtPath("/asset/child1")
    editor.ApplyEdits()

    # Delete a UsdProperty
    property = stage.GetPropertyAtPath("/model/charA.customProperty")
    editor.DeleteProperty(property)
    editor.ApplyEdits()

    # Move a prim, potentially to a different layer. This call assumes 
    # "/Materials/Material1" doesn't already exist in the composed stage
    # and also that "/Materials" does exist
    editor.MovePrimAtPath("/asset/Material1","/Materials/Material1")
    editor.ApplyEdits()

    # Use MovePropertyAtPath to rename a property. This call assumes that
    # "/model/charA.deprecatedProperty" doesn't already exist in the
    # composed stage
    editor.MovePropertyAtPath("/model/charA.extraProperty", "/model/charA.deprecatedProperty")
    editor.ApplyEdits()

As mentioned earlier, :usdcpp:`UsdNamespaceEditor` also provides convenience 
methods to rename and reparent :usdcpp:`UsdPrims <UsdPrim>` or 
:usdcpp:`UsdProperties <UsdProperty>`. 

.. code-block:: python

    # Renames "/model/extraChar" to "/model/deprecatedChar"
    renamedPrim = stage.GetPrimAtPath("/model/extraChar")
    editor.RenamePrim(renamedPrim, "deprecatedChar")
    editor.ApplyEdits()

    # Reparent "/model/deprecatedChar" to "/deprecated",
    # assumes "/deprecated" exists in the composed stage
    reparentedPrim = stage.GetPrimAtPath("/model/deprecatedChar")
    newParentPrim = stage.GetPrimAtPath("/deprecated")
    editor.ReparentPrim(reparentedPrim, newParentPrim)
    editor.ApplyEdits()

    # Reparent and rename "/model/charA.oldProperty" to
    # "/deprecated/deprecatedChar.deprecatedProperty", 
    # assumes "/deprecated/deprecatedChar" exists in the composed stage
    reparentedProp = stage.GetPropertyAtPath("/model/charA.oldProperty")
    newParentPrim = stage.GetPrimAtPath("/deprecated/deprecatedChar")
    editor.ReparentProperty(reparentedProp, newParentPrim, "deprecatedProperty")
    editor.ApplyEdits()

Note that after renaming or reparenting a :usdcpp:`UsdPrim` or 
:usdcpp:`UsdProperty`, the :usdcpp:`UsdPrim` or :usdcpp:`UsdProperty` reference 
used in the operation will no longer be valid, as the path has changed. 

Fixing Paths For Moved Objects
==============================

If a prim or property is moved/renamed/reparented, and other objects in the 
composed stage use paths to the original prim or property (for example, via a 
relationship path, or composition arc), namespace editing will attempt to 
correct those paths to use the new path for the edited prim or property.

For example, if you had a prim with a relationship to another prim:

.. code-block:: usda

    def "groupA" ()
    {
        custom rel charA = </asset/charA>
    }

And you moved :sdfpath:`/asset/charA` to :sdfpath:`/oldAssets/reviewed/charA`:

.. code-block:: python

    editor.MovePrimAtPath("/asset/charA","/oldAssets/reviewed/charA")

The relationship property in :usda:`primA` would be automatically updated in 
the composed stage to :sdfpath:`/oldAssets/reviewed/charA`:

.. code-block:: usda

    def "groupA" ()
    {
        custom rel charA = </oldAssets/reviewed/charA>
    }

Places where paths are used that will get fixed currently include:

- Relationship targets
- Attribute connections

Namespace editing will support fixing paths used in other areas (e.g. path 
expressions for pattern based collections) in future updates.

Note that fixes for edits that delete or move a prim or property that is 
introduced across a composition arc (like a reference) may have undesired 
consequences for other objects using an arc to the edited prim or property. 
For example, if you had a prim :sdfpath:`/asset` that was referenced by two 
other prims, :sdfpath:`/Shot1/shotAsset` and :sdfpath:`/Shot2/shotAsset`:

.. code-block:: usda

    def "asset" ()
    {
        def "assetChild" ()
        {
        }
    }

    def "Shot1" ()
    {
        def "shotAsset" (
            prepend references = </asset>
        )
        {
        }
    }

    def "Shot2" ()
    {
        def "shotAsset" (
            prepend references = </asset>
        )
        {
        }
    }

Any edits to :sdfpath:`/asset` will be reflected in both :sdfpath:`/Shot1/shotAsset` 
and :sdfpath:`/Shot2/shotAsset`:

.. code-block:: python

    editor.MovePrimAtPath("/asset/assetChild", "/asset/assetUpdatedForShot1")

With the flattened results looking like:

.. code-block:: usda

    def "asset" ()
    {
        def "assetUpdatedForShot1" ()
        {
        }
    }

    def "Shot1" ()
    {
        def "shotAsset" (
        )
        {
            def "assetUpdatedForShot1" () 
            {
            }
        }
    }

    def "Shot2" ()
    {
        def "shotAsset" (
        )
        {
            def "assetUpdatedForShot1" () 
            {
            }
        }
    }

.. note:: 
    Currently, direct edits across composition arcs, such as renaming just 
    :sdfpath:`/Shot1/shotAsset/assetChild` in the above example, is not supported 
    via namespace editing and will result in the following error (from 
    :code:`CanApplyEdits()` or :code:`ApplyEdits()`): "The prim to move requires 
    authoring relocates since it composes opinions introduced by ancestral 
    composition arcs; authoring relocates is not yet supported". Relocates will 
    be available in a future update.

.. _namespace_editing_batch_edits:

Batch Edits
===========

.. note:: 
    Currently batch editing is work-in-progress, so the following details are 
    not implemented, and the batch queue is effectively limited to a single edit 
    operation. This also means that any edit operation that is called without 
    :code:`ApplyEdits()` being called afterwards will currently get replaced by 
    the next edit operation call made in the current session.

:usdcpp:`UsdNamespaceEditor` provides the ability to create a sequence of edit 
operations and execute them as a batch. Each edit operation you call will be 
added to a queue and the queue will be executed (and cleared) when you call 
:python:`UsdNamespaceEditor.ApplyEdits()`. 

.. code-block:: python

    # Start a batch of edits to remove deprecated assets and mark other
    # assets as now deprecated
    editor.DeletePrimAtPath("/oldAssets/deprecated/protoModel")
    editor.MovePrimAtPath("/currentAssets/model1", "/oldAssets/model1")
    ...other additional related edits...
    batchApplyResult = editor.ApplyEdits()  # Apply this batch of edits

The batch is validated before applying, so if any of the edit operations would 
fail, none of the batch is actually applied, and :code:`ApplyEdits()` will 
return false. If you want to sanity-check whether your edits can successfully 
be applied to the stage, use :python:`UsdNamespaceEditor.CanApplyEdits()`. This 
will verify any queued edits and return true if the edits can be applied to 
the current stage, and false otherwise.

.. code-block:: python

    canApply = editor.CanApplyEdits()
    if canApply:
        editor.ApplyEdits()

Executing batches of edits will usually be more efficient than applying each 
edit individually. USD will process the list of edits in a batch to determine 
the most efficient way to apply them.

********************************
Namespace Editing Best Practices
********************************

The following are some general best practices and caveats for namespace editing.

Use CanApplyEdits() To Validate Edit Operations
===============================================

When you call edit operations, operation parameters like paths are 
sanity-checked, but the operation itself is not applied until you call 
:code:`ApplyEdits()`. If you want to confirm if your operation can be applied, 
use :code:`CanApplyEdits()` before calling :code:`ApplyEdits()`. 
:code:`CanApplyEdits()` will return an annotated bool that evaluates to 
:python:`True` if the edits can be applied, and :python:`False` if the edits 
cannot be applied, with the return result including a :code:`whyNot` attribute 
with further details.

For example, you might have a delete operation that uses a path to a prim that 
unbeknownst to you does not exist in the current stage. The 
:code:`DeletePrimAtPath` call will succeed as the provided path is a valid Sdf 
path, however both :code:`CanApplyEdits()` and :code:`ApplyEdits()` will fail 
indicating the path does not resolve to a valid prim.

.. code-block:: python

    editor.DeletePrimAtPath("/primDoesNotExist")
    # Check first if we can delete /primDoesNotExist
    canApplyResult = editor.CanApplyEdits()  
    if canApplyResult:
        editor.ApplyEdits()
    else:
        # Handle error, using canApplyResult.whyNot as needed, etc.

Built-In Properties From Schemas Are Not Editable
=================================================

The namespace editor does not allow deleting or moving built-in properties. For 
example, if you had a :usdcpp:`UsdGeomSphere` with an authored opinion for the 
:usda:`radius` attribute:

.. code-block:: usda

    def Sphere "testSphere" (
    )
    {
        custom string customProp = "custom property"

        # UsdGeomSphere schema radius attribute
        double radius = 2
    }

You could edit the :usda:`customProp` attribute, but editing :usda:`radius` 
would fail.

.. code-block:: python

    editor.DeletePropertyAtPath("/testSphere/customProp")  # This is allowed
    editor.DeletePropertyAtPath("/testSphere/radius")  # This is not allowed and will cause an error
