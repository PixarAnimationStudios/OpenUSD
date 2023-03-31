.. include:: rolesAndUtils.rst

.. include:: tut_historical_badge.rst

==========================
Variants Example in Katana
==========================

.. note:: The Katana USD plugin was removed from the USD distribution in version
   20.05 in favor of the Foundry-supported plugin `hosted on GitHub
   <https://github.com/TheFoundryVisionmongers/KatanaUsdPlugins>`_.  This
   documentation remains for historical reference.

This is a proof-of-concept example of how one might expose USD variant 
switching in :program:`Katana`. The scene shown here is located at
:filename:`USD/extras/usd/tutorials/exampleModelingVariantsKatana`, which 
contains the :program:`katana` file 
(:filename:`exampleModelingVariants.katana`) and a USD asset with modeling 
variants (:filename:`ExampleModelingVariants.usd` and associated usd files).

When bringing up the USD asset in :program:`usdview`, we can select our
prim, :filename:`/ExampleModelingVariants` which will populate a pane in the 
lower right hand side called "Meta Data". In this section, we can select 
different variants. For illustration, click that selector and choose "Torus".

.. image:: http://openusd.org/images/tut_variants_example_in_katana_sphere.png

From there you will see the variant change.

.. image:: http://openusd.org/images/tut_variants_example_in_katana_torus.png

Now open :filename:`exampleModelingVariants.katana` in :program:`Katana`. The 
model, ExampleModelingVariants, can be toggled between its various shapes with 
the provided PxrUsdInVariantSelect node . Simply target the model scenegraph
location and the authored variant set names will auto-populate. Select
"modelingVariant" in the variantSetName parameter and the variantSelection list
will populate with the shape variants.

.. image:: http://openusd.org/images/tut_variants_example_in_katana_nodegraph.png

