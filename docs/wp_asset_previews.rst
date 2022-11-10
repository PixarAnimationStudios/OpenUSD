=====================
Asset Previews in USD
=====================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

Copyright |copy| 2022, Pixar Animation Studios,  *version 1.0*

.. contents:: :local:

############
Introduction
############

Fast, scalable browsing of numerous USD assets is a concern that grows in
importance as the number of readily available assets grows.  On many
platforms it will be impractical to load and render all assets in a directory
just-in-time to generate preview/thumbnail views, even using background
threads and caching.  We therefore propose an encoding in USD for specifying
pre-generated, lightweight preview assets for USD scenes, that can be
recovered from a USD asset with small expense.

While we expect to eventually **expand** the kinds and variations of previews USD
assets are allowed to contain, we propose to start out very simply with
**basic thumbnail images**, encoded in a forward-looking way.


########
Proposal
########

************
Object Model
************

The first association that comes to mind for previews is one between a
**stage** (i.e. the **root layer** of an asset) and a preview.  However, many
assets will have more than one interesting element that may benefit from
having its own preview. One example might be a material library, where we
would like independent previews for each material.  Another example might be
a scene (such as the open `Moana Island dataset
<https://www.disneyanimation.com/resources/moana-island-scene/>`_ ) that
embeds multiple user-facing cameras, and it would be useful to have a
pre-rendered thumbnail image for each camera view.  A final example might be
that in an assembly representing an environment, like the Moana island, while
we would of course want a default preview of the entire scene, it may also be
useful to discover and present all of the referenceable assets used by the
environment.

All of these use-cases suggest instead **associating previews with prims**,
rather than layers.  How, then, do we facilitate the *base-level* need to
"specify the preview for the scene"?  It is already accepted practice in USD
content creation to specify an asset's **defaultPrim** in layer metadata,
which names a root prim that represents the default reference target for the
scene. We propose to leverage this pattern, stipulating that the preview for
a stage is the preview specified on the stage’s defaultPrim.  To ensure the
specification is robust in the face of common manipulations like overriding a
scene by creating a new, initially empty layer that sublayers an existing
scene (which is what :mono:`usdview`'s "Save Overrides" does), we require
that the defaultPrim be **composed** in order to extract the previews, and we
will discuss below how we can ensure that process requires minimal
computation.

Relationship to UsdGeomModel Texture Cards
==========================================

Associating images or other "lightweight previews" with prims sounds very
similar to what we already encode in `UsdGeomModel's "cards" DrawMode
<api/class_usd_geom_model_a_p_i.html#UsdGeomModelAPI_drawMode>`_?  The
GeomModel feature was designed to address targeted needs of 3D rendering of
very large scenes.  It is therefore limited in two important ways that
facilitate its use in scalable 3D rendering, but we find too limiting for the
needs of external clients who are not necessarily interested in performing
Hydra or other 3D rendering:

#. GeomModel is tied to the :ref:`glossary:Model Hierarchy`, and as discussed
   at the beginning of this section, we want to be able to encode previews on
   Material, Camera, or other prims that are not part of the model hierarchy.
#. Texture cards are tied to specific orthographic views, which serves their
   embedding as "2.x dimension" objects in a 3D space, but for, e.g. 2D
   catalog and preview needs, more artistically crafted views may be more
   appropriate.  


*****************
Concrete Encoding
*****************

We propose to specify preview assets such as thumbnails using a scene
description :code:`dictionary` datatype, which provides great flexibility in
authoring and organizational nesting.  Given the strong affinity of previews
with assets, we propose to encode previews as an extension of the existing
:ref:`assetInfo dictionary <glossary:AssetInfo>`. Leveraging assetInfo has
the additional benefit of making the proposed data backwards and forwards
compatible with essentially all versions of USD; one can interact with
assetInfo using generic API's in applications using old versions of USD to
process assets with previews.

The initial recognized/supported structure for a "default" thumbnail would
look like the following in usda serialization: 

.. code-block:: usda

  assetInfo = {
    dictionary previews = {
        dictionary thumbnails = {
            dictionary default = {
                asset defaultImage = @chair_thumb.jpg@
            }
        }
    }
  }

Where :code:`defaultImage` must identify an image of :ref:`a format allowed
in USDZ packages <spec_usdz:Usdz Specification>`. 

The triple-nesting of dictionaries for a single thumbnail may seem like
overkill, but it allows us room to grow: 

* Alternative types of previews to thumbnail images (e.g. simplified 3D
  USD representations)
* Alternative sets of thumbnails (per-renderer, variations, etc)
* More than one fidelity, size, turntable, lenticular, etc view of a given
  "thumbnail" 

However, all of these extensions require design and debate, and our initial
goal is to serve basic needs on an accelerated schedule.  We will explore
them in an extension to this proposal in the future. 

######
Schema
######

We propose to add an applied schema, :usdcpp:`UsdMediaPreviewsAPI` in the
`UsdMedia domain <api/usd_media_page_front.html>`_ for interacting with
previews on prims.  Even though the schema would possess no properties,
requiring an application of an API schema to encode its metadata provides an
idiomatic, fast, and validatable means of discovering which prims possess
previews.  The schema would also provide a method that takes an SdfLayer, and
manages a stage and population strategy for the client who needs only to
extract the default asset preview data (i.e. on the default prim) from a
stage.
