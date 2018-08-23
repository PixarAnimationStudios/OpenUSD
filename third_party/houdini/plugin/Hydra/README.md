## Hydra houdini plugin ##

Draws gusd prims (USD data from the Pixar USD Import SOP) using Hydra. Correctly
Z-buffers with any non-USD geometry in the viewer, and the basic Houdini lighting is
reproduced using Hydra lights. Subdivision is controlled by the LOD in the display
options.

* Renders are much faster (allowing use of $FF for the frame number in the USD import
  in all cases) and it can deal with much finer traversal and unpacking.

* Renders any prims that usdview can render, including ones that USD Import does not know
  how to convert to Houdini geometry, and ones it ignores due to more strict type checking.

* Renders normals and subdivision more accurately

* Obeys VisibilityAttr

* USD Import of Alembic also draws faster than native Houdini Alembic import.

If you "unpack" enough times then some/all the prims are converted to Houdini geometry,
which is not drawn by Hydra.

## Environment variables ##

* HYDRA_HOUDINI_DISABLE=1: Ignore the plugin, so the normal drawing code supplied with
USD Import is used.

* HYDRA_HOUDINI_DISABLE=2: Install the plugin but do not use Hydra, instead all meshes
are converted to RE_Geometry. This converter is faster than the one in USD Import.

* HYDRA_HOUDINI_POSTPASS=0: Don't use post pass, and don't merge multiple OBJ renders
from the same stage into a single Hydra render.

* GUSD_STAGEMASK_ENABLE=false: If you are reading multiple prims by name (not due to
traversal), drawing will be significantly faster if you disable stage masking, so
that they are all on the same stage. However this will slow down performance if you
are only reading a small subset of the prims from a very large stage.

## Known bugs ##

* Points, Normals, Prim and Vertex numbers, and other Houdini overlays are not drawn.

* Snapping is not implemented.

* Shadows are not implemented.

* LOD is clamped to 1.4 to prevent Hydra hanging forever doing subdivision.

* An RE_Geometry replacement is used when necessary to make things work. This
  replacement is not fully accurate, but the hope is to use Hydra more, rather than
  improve this:

  - For anything other than UsdGeomMesh, it is a octahedron inscribed in the bounding box.
    This includes PointInstancer and curves.
  - Subdivision is ignored
  - displayOpacity is ignored
  - normals and colors are vertex interpolated, with simplistic conversions from
    other interpolations.
  - instancing is not implemented except between top-level matching gusd prims

* Hit detection and preview highlighting uses the RE_Geometry replacement. May be
  fixable by changes to Hydra so it can draw constant color mattes without modifying
  the shader, compositing, or other OpenGL settings.

* Hydra will not directly draw descendents of a PointInstancer. This will happen if
  you select the prototype for import, or if you unpack a PointInstancer.  The
  RE_Geometry substitute is used instead.

* RE_Geometry substitute is also used for any object that occurs more than once in
  the same OBJ, such as the results of using Houdini instancing SOP on a usd prim, or
  unpacking a PointInstancer. In this case the RE_Geometry correctly uses instancing
  for these top prims.

* Multiple Hydra renders are run if the frame number is different, for prims from
  different stages, if some prims have been transformed a different amount than
  others, and for the selected object to change the wireframe color.

* A seperate Hydra render is run for each ghosted object.

* Hydra incorrectly draws any subset of the unmasked instances of an instanced object
  (it will either draw all of them or none of them). This is probably the most serious
  bug for our current usage, and I suspect a simple one to fix in Hydra, as the
  correct instance is highlighted.

* Hydra does not draw any PointInstancer inside an instanced object (it only draws
  them when told to draw the pseudo-root, which is why usdview works).

* Onion skinning draws some instances in the wrong location because the frame
  attribute is set wrong. Probably a bug in how Houdini evaluates $FF. May be fixable
  by having USD Import detect if the expression is $FF and using the frame directly.

## Credits ##

Written by Bill Spitzak at DreamWorks Animation
