.. include:: ../rolesAndUtils.rst

.. _namespace_editing:

#################
Namespace Editing
#################

A namespace edit is an operation that removes or changes the namespace path of 
a **composed** prim or property on a stage. Edit operations currently include 
deleting and moving (renaming and/or reparenting) a composed prim or property.

While it is possible to do some of these operations using Sdf or UsdStage APIs,
these APIs can only edit prims or properties on a single layer. Additionally, 
any paths targeting the moved or deleted objects will need to be manually fixed. 
Use namespace editing to easily and safely delete or move prims and properties
across the :ref:`LayerStack <usdglossary-layerstack>`. Namespace editing handles 
edits for objects composed from multiple layers and handles changing 
:ref:`EditTargets <usdglossary-edittarget>` or fixing paths targeting the 
edited objects, automatically.

Namespace also provides non-destructive editing of prims defined across 
composition arcs by adding the **relocates** composition arc if needed.
By using relocates, namespace editing ensures non-destructive edits by *not*
modifying the source of a composition arc. This is necessary for workflows where
you might not be able to make changes to the source of the composition 
arc directly (e.g. you're referencing prims from assets maintained by a 
different department that are also being referenced elsewhere).

As a simple example, if you had :filename:`model.usda` with various prims:

.. code-block:: usda

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
overrides for :sdfpath:`/modelScope` and :sdfpath:`/modelScope/model1`:

.. code-block:: usda

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
remove :sdfpath:`/modelScope/model1` from your composed stage using UsdStage 
APIs, you'd have to delete it twice: once in :filename:`main.usda`, and then, 
after changing :ref:`EditTargets <usdglossary-edittarget>`, again in the 
:filename:`model.usda` layer:

.. code-block:: python

    stage = Usd.Stage.Open("main.usda")

    # This only removes model1 in main.usda, not
    # model1 in the model.usda layer
    removeResult = stage.RemovePrim("/modelScope/model1")  

    # After the RemovePrim() call, we still have /modelScope/model1
    # from model.usda in our composed stage, and would have to 
    # set the edit target to the model.usda layer and call RemovePrim() again

Whereas using namespace editing will properly handle removing the prim from
both :filename:`main.usda` and :filename:`model.usda` in a single edit 
operation, handling edit target changes for you:

.. code-block:: python

    editor = Usd.NamespaceEditor(stage)
    removeResult = editor.DeletePrimAtPath("/modelScope/model1")
    editor.ApplyEdits()

Namespace editing handles issues such as making sure that paths targeting 
edited objects and overrides are still valid after renaming or reparenting 
prims. Namespace editing tries to fix any existing composition arcs, 
relationship targets, attribute connections, and overrides that used paths to 
renamed or reparented prims to use the new paths.

Namespace editing will also use **relocates** for more complex edit scenarios. 
Relocates are another composition arc that maps a prim path defined in a remote 
:ref:`LayerStack <usdglossary-layerstack>` (i.e. across a composition arc) to a 
new path location in the local namespace. For an example of how namespace 
editing uses relocates, if you had :filename:`refModel.usda`: 

.. code-block:: usda

    def "modelA" ()
    {
        def "modelAChild" ()
        {
        }
    }

Which is referenced in :filename:`main.usda`:

.. code-block:: usda

    def "mainModelA" (
        prepend references = @refModel.usda@</modelA>
    )
    {

You might want to use namespace editing to move or rename 
:sdfpath:`/mainModelA/modelAChild`. Because :sdfpath:`/mainModelA/modelAChild`
is composed across a reference composition arc, it can't be directly edited,
so namespace editing will use relocates to create the edit.

.. code-block:: python

    # Rename /mainModelA/modelAChild to /mainModelA/renamedChild
    # Namespace editing will use relocates for this edit
    removeResult = editor.MovePrimAtPath("/mainModelA/modelAChild", "/mainModelA/renamedChild")
    editor.ApplyEdits()

The resulting stage root layer for :file:`main.usda` will look like:

.. code-block:: usda

    #usda 1.0
    (
        relocates = {
            </mainModelA/modelAChild>: </mainModelA/renamedChild>
        }
    )

    def "mainModelA" (
        prepend references = @refModel.usda@</modelA>
    )
    {
    }

.. note::

    **relocates** is a new composition arc for USD that is a separate feature
    from namespace editing, and can be used independently of namespace editing.
    See :ref:`usdglossary-relocates` for more details.

.. _nsedit_using_usdnamespaceeditor:

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
batches of operations, see :ref:`nsedit_batch_edits` below). To 
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

.. _nsedit_setting_editor_options:

Setting Editor Options
======================

When you create a namespace editor, you can optionally provide edit options
that control editor behavior. The current set of options are:

* **allowRelocatesAuthoring**: If :code:`True` the namespace editor will use 
  relocates when needed to make edits. If :code:`False`, the namespace editor 
  will not use relocates and will issue errors when applying or validating edits 
  that require relocates. The default is :code:`True`.

The following example creates the :code:`noRelocatesEditOptions` edit options,
disables :code:`noRelocatesEditOptions.allowRelocatesAuthoring`, and creates
a namespace editor for :code:`stage` with these options.

.. code-block:: python

    noRelocatesEditOptions = Usd.NamespaceEditor.EditOptions()
    noRelocatesEditOptions.allowRelocatesAuthoring = False

    # Create/use namespace editor that will not use relocates
    noRelocatesEditor = Usd.NamespaceEditor(stage, noRelocatesEditOptions)

.. _nsedit_working_with_relocates:

Working With Relocates
======================

As mentioned earlier, namespace editing will use **relocates** if necessary
for edit operations across composition arcs. Namespace editing will add or
update relocates in the appropriate layer's metadata relocates list. The
following example shows layer metadata with a relocate added by a namespace
editing operation to rename :sdfpath:`/mainModelA/modelAChild` to 
:sdfpath:`/mainModelA/renamedChild`:

.. code-block:: usda

    #usda 1.0
    (
        relocates = {
            </mainModelA/modelAChild>: </mainModelA/renamedChild>
        }
    )

For delete operations that require using relocates, namespace editing will
create a relocates mapping that maps a prim or property to a "deleted" target:

.. code-block:: python

    # Delete a referenced prim, which will add a new relocates
    editor.DeletePrimAtPath('/RootPrim/ChildInRef')
    editor.ApplyEdits()

.. code-block:: 

    #usda 1.0
    (
        relocates = {
            </RootPrim/ChildInRef>: <>
        }
    )

Note that the way in which relocates must "transfer" composed opinions prevents 
you from re-defining a new prim at the deleted or moved target location:

.. code-block:: python

    # Continuing from the earlier Python code, now try and define a prim at the
    # deleted /RootPrim/ChildInRef path. This will result in a 
    # "Failed to define UsdPrim" error.
    stage.DefinePrim('/RootPrim/ChildInRef')

.. _nsedit_fixing_paths_for_moved_objects:

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

.. _nsedit_fixing_edits_with_dependent_stages:

Applying Edits to Dependent Stages
==================================

In some situations, a namespace edit applied to one stage can impact other
**dependent stages**. A dependent stage is any additional stage open in the 
current session that has a composition dependency on any layer edits made for 
the editor's primary stage. For example, you might have a stage that has
references to prims in the editor's primary stage, and you might have edits
that rename or delete the referenced prims.

By default, an editor only makes edits and fixes to the editor's primary stage.
However, you can add dependent stages to an editor via 
:code:`AddDependentStage()` and the editor will make any necessary additional 
edits in those dependent stages to update the composition dependencies 
appropriately.

For example, we might have the layer :filename:`layer1.usda`, which will get 
composed into the stage for which we'll create a namespace editor.

.. code-block:: usda

    #usda 1.0
    (
    )                

    def "Prim1" {
        def "Child" {
        }
    }

    def "InternalRef1" (
        references = </Prim1>
    ) {
        over "Child" {
            int overChildAttr
        }
    }

We also have an additional layer :filename:`layer2.usda` that references prims 
in :filename:`layer1.usda`:

.. code-block:: usda

    #usda 1.0
    (
    )                

    def "OtherStageRef2" (
        references = @layer1.usda@</Prim1/Child>
    ) {
        int overChildAttr
    }    

We can open both layers in separate stages, create a namespace editor for the
stage containing layer1, and add the stage containing layer2 as a dependent 
stage.

.. code-block:: python

    stage1 = Usd.Stage.Open("layer1.usda")
    stage2 = Usd.Stage.Open("layer2.usda")

    # Create a namespace editor for stage1
    editor = Usd.NamespaceEditor(stage1)

    # Add stage2 as a dependent stage for our stage1 editor
    editor.AddDependentStage(stage2)

    # Move /Prim1/Child to /Prim1/RenamedChild. This will not only 
    # update the prims and references in stage1, but also update the
    # OtherStageRef2 reference in stage2
    editor.MovePrimAtPath('/Prim1/Child', '/Prim1/RenamedChild')
    editor.ApplyEdits()

After the edit, the root layer of stage2 looks like:

.. code-block:: usda

    #usda 1.0

    def "OtherStageRef2" (
        references = @layer1.usda@</Prim1/RenamedChild>
    )
    {
        int overChildAttr
    }

You can use :code:`RemoveDependentStage()` to remove any added dependent stages
before making a namespace edit, when you do not want the dependent stage
dependencies updated. 

If you have several dependent stages, you can set a list of dependent stages on
an editor using :code:`SetDependentStages()`. 

Note that namespace editing finds dependencies in dependent stages based on 
what is currently loaded in those stages. If a stage has dependencies in 
unloaded payloads, load mask filtered prims, unselected variants, or children of
inactive prims, the namespace editor cannot find and update those dependencies.

.. _nsedit_batch_edits:

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

.. _nsedit_editing_best_practices:

********************************
Namespace Editing Best Practices
********************************

The following are some general best practices and caveats for namespace editing.

.. _nsedit_canapplyedits_validate_operations:

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

.. _nsedit_builtin_properties_not_editable:

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

.. _nsedit_relocates_performance_impact:

Be Aware of Relocates Performance Impact
========================================

If your namespace editing operations result in adding **relocates** to your 
stage, this can increase the number of composition arcs in your stage, which
could impact stage composition performance.

If you want to test whether a namespace edit will add relocates, you can
use an editor configured to disallow authoring relocates, and use 
:code:`CanApplyEdits()` looking for any errors that indicate relocates would 
be needed.  

.. code-block:: python

    # Create/use namespace editor that will not use relocates
    noRelocatesEditOptions = Usd.NamespaceEditor.EditOptions()
    noRelocatesEditOptions.allowRelocatesAuthoring = False
    noRelocatesEditor = Usd.NamespaceEditor(stage, noRelocatesEditOptions)

    # Rename /mainModelA/modelAChild to /mainModelA/renamedChild
    # This editor is configured to not use relocates, so an error will be shown
    removeResult = noRelocatesEditor.MovePrimAtPath("/mainModelA/modelAChild", "/mainModelA/renamedChild")
    applyResult = noRelocatesEditor.CanApplyEdits()
    if applyResult is not True:
        # We should get a "The prim to edit requires authoring relocates since 
        # it composes opinions introduced by ancestral composition arcs; 
        # relocates authoring must be enabled to perform this edit" error
        print ("noRelocatesEditor: Cannot apply edits, reason: " + applyResult.whyNot)
 