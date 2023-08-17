.. It is unfortunate we disable sub-structure with "titlesonly" for all the
.. Resources pages, but until we can figure out how to do it selectively for
.. the Glossary, which is much too long to be indexed thusly, that's where we are

.. Note that for the left nav for the RTD theme, certain settings in this file
.. are replaced by RTD settings in conf.py. Example: maxdepth is replaced by 
.. navigation_depth in conf.py

.. toctree::
   :hidden:
   :titlesonly:
   :caption: Learn

   intro
   Terms and Concepts <glossary>
   Tutorials <tut_usd_tutorials>
   dl_downloads
   usd_products

.. toctree::
   :hidden:
   :caption: User Guides
   :maxdepth: 1

   Rendering with USD <user_guides/render_user_guide>

.. toctree::
   :hidden:
   :caption: Reference
   :maxdepth: 1

   apiDocs
   Toolset <toolset>
   Specifications <spec>
   Proposals <wp>
   FAQ <usdfaq>
   Performance Considerations <maxperf>
   Third Party Plugins <plugins>

.. toctree::
   :hidden:
   :caption: Collaborate

   Source Code @ GitHub <https://github.com/PixarAnimationStudios/USD>
   OpenUSD Forum <https://forum.openusd.org>
   Contributing <contributing_to_usd>
   Contributors <contributors>

.. toctree::
   :hidden:
   :titlesonly:
   :caption: Press

   press_opensource_release
   press_opensource_announce
