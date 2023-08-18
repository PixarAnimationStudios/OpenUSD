=============
USD Tutorials
=============


.. include:: rolesAndUtils.rst

USD provides a set of tutorials in the `Github repository
<https://github.com/PixarAnimationStudios/OpenUSD>`_. The tutorial code is located
in the :filename:`extras/usd/tutorials` directory unless otherwise noted. Each
tutorial indicates which version of USD it was tested with. These versions
correspond to releases on `Github
<https://github.com/PixarAnimationStudios/OpenUSD/releases>`_.

Environment Setup
*****************

These tutorials use USD's built-in Python bindings almost exclusively, and some
programs in the :doc:`toolset` rely on one another.  So please set the following
environment variables to successfully complete the tutorials. Make sure to use 
the Python interpreter corresponding to the version used to build USD (for 
supported Python versions see `3rd Party Library and Application Versions 
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/VERSIONS.md>`_,
note that Python 2.x is not supported). These tutorials assume the interpreter 
is named "python". 

+---------------------+------------+----------------------------------------+
|Variable             |Meaning     |Value                                   |
+=====================+============+========================================+
|:envvar:`PYTHONPATH` |Python      |:filename:`USD_INSTALL_ROOT/lib/python` |
|                     |module      |                                        |
|                     |search path |                                        |
+---------------------+------------+----------------------------------------+
|:envvar:`PATH`       |Program     |:filename:`USD_INSTALL_ROOT/bin`        |
|                     |search path |                                        |
|                     |            |On Windows also add                     |
|                     |            |:filename:`USD_INSTALL_ROOT/lib`        |
+---------------------+------------+----------------------------------------+

For more information see `Advanced Build Configuration
<https://github.com/PixarAnimationStudios/OpenUSD/blob/release/BUILDING.md>`_.

.. toctree::
   :titlesonly:
   :caption: Tutorials

   tut_helloworld
   tut_helloworld_redux
   tut_inspect_and_author_props
   tut_referencing_layers
   tut_converting_between_layer_formats
   tut_traversing_stage
   tut_authoring_variants
   tut_variants_example_in_katana
   tut_xforms
   tut_simple_shading
   tut_end_to_end
   tut_houdini_example.rst
   tut_generating_new_schema
   tut_usdview_plugin

