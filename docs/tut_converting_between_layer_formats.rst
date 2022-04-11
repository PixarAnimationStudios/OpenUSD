.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst             

================================
Converting Between Layer Formats
================================

This tutorial walks through converting layer files between the
:ref:`different native USD file formats <usdfaq:So what file formats does USD support?>`.

The :ref:`toolset:usdcat` tool is useful for inspecting these files, but it can
also convert files between the different formats by using the :option:`-o`
option and supplying a filename with the extension of the desired output
format. Here are some examples of how to do this.

.. _converting-to-usd:

Converting between :filename:`.usda`/:filename:`.usdc` and :filename:`.usd`
###########################################################################

A :filename:`.usd` file can be either a text or binary format file. When USD
opens a :filename:`.usd` file, it detects the underlying format and handles the
file appropriately. You can convert any :filename:`.usda` or :filename:`.usdc`
file to a :filename:`.usd` file simply by renaming it.  For example:

* Given :filename:`Sphere.usda`, you can simply rename it to
  :filename:`Sphere.usd`.

     * When USD opens :filename:`Sphere.usd`, it detects that it uses the text
       file format and opens it appropriately.  This works similarly for
       :filename:`.usdc` files.

* A :filename:`Sphere.usd` file that uses the text format can be renamed to
  :filename:`Sphere.usda`
  
* A :filename:`Sphere.usd` file that uses the binary format can be renamed to
  :filename:`Sphere.usdc`.

Converting between :filename:`.usda` and :filename:`.usdc` Files
################################################################

Consider :filename:`Sphere.usda` in
:filename:`USD/extras/usd/tutorials/convertingLayerFormats`. This is a text
format file, so it can be examined in any text editor or via
:ref:`toolset:usdcat`.

.. code-block:: sh

   $ usdcat Sphere.usda
   
   #usda 1.0
   def Sphere "sphere"
   {
   }

To convert this file to binary, we can specify an output filename with the
:filename:`.usdc` extension:

.. code-block:: sh

   $ usdcat -o NewSphere.usdc Sphere.usda

This produces a binary file named :filename:`Sphere.usdc` in the current
directory containing the same content as :filename:`Sphere.usda`.  We can verify
this using :ref:`toolset:usddiff`:

.. code-block:: sh

   $ usddiff Sphere.usda NewSphere.usdc

Converting binary to text can be done the same way:

.. code-block:: sh

   $ usdcat -o NewSphere.usda NewSphere.usdc
   $ usddiff NewSphere.usdc NewSphere.usda

Converting between :filename:`.usd` Files of Different Formats
##############################################################

.. note::
   
   If you have a :filename:`.usda` or :filename:`.usdc` file and want to make it
   a :filename:`.usd` file without changing the underlying format, you **do
   not** need to use :ref:`toolset:usdcat`; just rename the file. See
   :ref:`converting-to-usd` above.

If you have a :filename:`.usd` file of a particular format and want to convert
it to a :filename:`.usd` file using a different format, pass the
:option:`--usdFormat` option to :ref:`toolset:usdcat`.  Starting with
:filename:`Sphere.usd` in
:filename:`USD/extras/usd/tutorials/convertingLayerFormats`, which is a text
format file, we can converty to a binary format file:

.. code-block:: sh

   $ usdcat -o NewSphere_binary.usd --usdFormat usdc Sphere.usd

We can verify the two files match:

.. code-block:: sh

   $ usddiff Sphere.usd NewSphere_binary.usd

Convert a binary :filename:`.usd` file to a text :filename:`.usd` the same way:

.. code-block:: sh

   $ usdcat -o NewSphere_text.usd --usdFormat usda NewSphere_binary.usd
   $ usddiff NewSphere_binary.usd NewSphere_text.usd

