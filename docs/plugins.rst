=======================
USD Third-Party Plugins
=======================

USD lets developers write plugins to integrate third-party libraries into various
parts of the system. For example:

* The Hydra rendering architecture lets custom and third-party renderers
  plug in, so that any application that renders though "USD Imaging" can image
  using that renderer, simply by selecting it. A number of vendors and studios
  have written their own Hydra "render delegate" plugins.

    ..

* Third-party file formats can be integrated into USD by writing file format
  plugins. These plugins provide USD representations of the data in files
  with the associated file format, allowing them to be used as layers anywhere
  in USD.

The USD distribution ships with several optional plugins. Documentation for
these plugins can be found on the pages linked below.

.. toctree::
   :titlesonly:
   :caption: Plugins

   plugins_renderman
   plugins_alembic
