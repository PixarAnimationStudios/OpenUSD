.. include:: rolesAndUtils.rst

.. include:: tut_historical_badge.rst

============================
Houdini USD Example Workflow
============================

.. note:: The Houdini USD plugin was removed from the USD distribution in
          version 20.05 in favor of native USD support in Houdini Solaris. This
          documentation remains for historical reference.
   
An example of using :program:`Houdini` in a USD-centric pipeline

Authoring USD Overrides in Houdini
##################################

This tutorial shows an example using :program:`Houdini` to modify the 
transform of model in a USD scene. The files for this tutorial live in
:filename:`USD/extras/usd/tutorials/Houdini`. It is assumed that you have a 
working knowledge of :program:`Houdini`.

In summary you will do the following:

    #. View the USD Scene - use :program:`usdview` to determine what you need 
       to import into :program:`Houdini`.

    #. Import the USD geometry - import all the geometry to modify and 
       anything needed for reference into :program:`Houdini`.

    #. :program:`Houdini` Shot work - modifying the imported models position, 
       orientation et al.

    #. Export the chages to a USD Overlay.

    #. Insert the Overlay back into the USD Scene.

View the USD Scene
******************

Open the USD scene in :program:`usdview` to identify the geometry you want to 
modify by using the following command line.

.. code-block:: console

   $ usdview  USD/extras/usd/tutorials/Houdini/shot.usda

.. image:: http://openusd.org/images/tut_houdini_example_shot.png
    
In this case we are interested in the little red house at the origin:

    * First select the prim by clicking in the prim browser on the 'house' row.

    * Next hit 'f' to bring it into frame.

.. image:: http://openusd.org/images/tut_houdini_example_house.png

.. note:: Note that the house appears yellow because we have selected it.
   

You can find the name of the prim path of the house in this USD
stage(/World/sets/house) by looking at the Prim Name window in the upper left
hand corner of :program:`usdview`. You'll use this information later when we 
import the house into :program:`Houdini`.

Imagine that you have received a note that the house should be placed at the 
top of the highest hill on the terrain.

Import USD geometry into Houdini
********************************

It's time to import the little red house into :program:`Houdini`.

    #. From the Geometry context in :program:`Houdini` drop down a USD import 
       SOP.

    #. Enter the name of, or browse to the USD file that contains the stage
       you're interested in modifying in the "USD File" parameter, in this 
       case: :filename:`USD/extras/usd/tutorials/Houdini/shot.usda`

    #. Enter the name of USD Prim path you are interested in modifying into the
       "Prim Path" parameter, in this case: :filename:`/World/sets/house`. You 
       can also use the Tree view to visually browse through the USD stage to 
       select the prim path.

.. image:: http://openusd.org/images/tut_houdini_example_import.png

In this case we also need to set the "Traversal" parameter to Gprims

The above steps sets set the traversal criteria for importing geometry into
:program:`Houdini`

You should now see the little red house in your :program:`Houdini` viewport.

On middle mouse click of the usdimport SOP you will notice that there is one
primitive, a packed USD. As you will only be altering the transformation of 
this model you can keep it packed and export what is known as a transform 
overlay.

Import the terrain for visual reference.

Repeat the steps above to import the terrain model by duplicating the usdimport
SOP and replacing the "Prim Path" with :filename:`/World/sets/terrain` and 
templating the new usdimport SOP.

Houdini shot work
*****************

Use the transform sop to move the house to the top of the highest hill.

Export the USD Overlay
**********************

    #. Create a "USD Output" ROP in the Outputs context of :program:`Houdini`.

    #. Enter the path to the transform sop you used to move the little red
       house.

    #. Toggle the "Overlay Existing Geometry" Parameter on.

    #. Enter the path of the new USD file that will contain the
       overlay, in this case: 
       :filename:`USD/extras/usd/tutorials/Houdini/OVR_house.usda`

    #. Enter the file path of the USD file that contains the USD prim you wish
       to override into the "Overlay Reference File" parameter, in this case: 
       :filename:`USD/extras/usd/tutorials/Houdini/shot.usda`

    #. As you are only interested in altering the transform of our model we are
       going to uncheck "Overlay All" and check "Overlay Transforms".

.. image:: http://openusd.org/images/tut_houdini_example_overlay.png

Hit "Render to Disk"

Insert Overlay into USD Scene
*****************************

    #. Drop down a "USD Layer" ROP into the Outputs context of 
       :program:`Houdini`.

    #. Set the first "Source" paramter field to
       :filename:`USD/extras/usd/tutorials/Houdini/OVR_house.usda`

    #. Hit "Render to Disk" on the USD layer ROP.

If everything has gone according to plan, you should now be able to see the
house in it's new location at the top of the hill in :program:`usdview`:

The completed Houdin file can be found here:
:filename:`USD/extras/usd/tutorials/Houdini/overlay_tutorial.hip`

You can see other example of how to use USD in Houdini in the included hip 
file: :filename:`USD/extras/usd/tutorials/Houdini/examples.hip`
