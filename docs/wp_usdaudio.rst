=================
UsdAudio Proposal
=================

.. include:: rolesAndUtils.rst
.. include:: <isonum.txt>

.. note::
   This proposal has been implemented. This document exists for historical
   reference and may be outdated. For up-to-date documentation, see the
   `UsdMediaSpatialAudio page <api/class_usd_media_spatial_audio.html>`_.

Copyright |copy| 2019, Pixar Animation Studios,  *version 1.1*

.. contents:: :local:

Goal 
====

The goal of this proposal is to define a standard schema for adding audio
support to USD. The schema must support basic, timed-start audio playback, as
well as the ability to identify sounds as having well-defined locations (and
orientations) in space. The primary purpose of the new schema is to provide an
interchange solution to audio elements embedded in USD scenes and animations.
Given that interchange is the main goal, we want to keep the following schema
simple, providing what we believe are the minimal requirements to play audio
elements in the scene. This schema is not intended as a document format for an
audio editing tool since more complex operations on sound elements are not
supported. The addition of such a schema may lead to thoughts about describing
interactive environments in USD, including such features as triggers for sounds.
While encoding triggers for sounds and other actions may be a worthwhile area of
future investigation, we defer any consideration of triggers or interactive
environments for now in the interests of simplicity and resources .

Initial Requirements  
====================

    * The USD schema should not directly contain any audio data but should refer
      to an audio file or stream via an *assetPath*

    * The USD schema should not directly define the supported audio file
      formats, but see :ref:`Usdz Considerations <wp_usdaudio:usdz considerations>` 
      below for usdz-related restrictions

    * Any number of audio elements should be allowed in a USD stage

    * Each audio element is consumed as a whole: the USD schema does not provide
      for isolating specific sub-channels or tracks within an audio asset.

    * The same audio file should be referenceable by multiple USD primitives

    * Audio elements should be positionable (and orientable) in space with a
      given 3D transformation that can change over time

    * Audio elements should also be able to represent "ambient" sound where the
      mono or stereo sound is played regardless of spatial positioning

    * Audio should be able to be amplified/attenuated over time 

    * This does not provide any absolute measure of sound pressure (i.e. dB)

    * Each audio element should be able to be amplified/attenuated
      independently. At a later date we may introduce a "grouping" feature to
      modulate multiple audio streams at once

    * No explicit support for "fadein" or "fadeout", which could be encoded via
      time-varying attenuation, should fading not be embedded directly in the
      audio streams being referenced

    * The start of playback within a UsdStage's time-range should be
      independently specifiable for each audio element, as should an optional
      endpoint.

    * Should be able to specify an offset into each referenced audio file from
      which playback should begin

    * Should be able to easily loop the audio stream, if desired

    * Each audio stream should be able to have its own arbitrary sample rate. It
      should be the responsibility of the playback tool to properly determine
      and play the sample rate specified in a given stream

Proposed Prim Schema 
====================

SpatialAudio
############

The :usda:`SpatialAudio` schema defines basic properties for encoding playback
of an audio file or stream within a USD Stage. The :usda:`SpatialAudio` schema
should live in a new domain, UsdMedia, as we expect the set of media-related
schemas to grow over time (e.g. imagine a move-clip-related schema). The
:cpp:`UsdMediaSpatialAudio` schema derives from `UsdGeomXformable
<api/class_usd_geom_xformable.html#details>`_
since it can support full spatial audio, while also supporting non-spatial mono
and stereo sounds. One or more :usda:`SpatialAudio` prims can be placed anywhere
in the namespace, though it is advantageous to place truly spatial audio prims
under/inside the models from which the sound emanates, so that the audio prim
need only be transformed *relative* to the model, rather than copying its
animation.

Below we define the builtin properties of the schema, with their fallback
values. Note that the only non-uniform (i.e. animatable) property is :bi:`gain`.

**Properties**

    * **uniform asset filePath = @@** 

      Path to the audio file.

    * **uniform token auralMode = "spatial"** 

      Determines how audio should be consumed.  Allowed values: 

      *spatial*: Play the audio in 3D space if the device can support spatial
      audio. If not, fall back to mono.

      *nonSpatial* : Play the audio without regard to the :usda:`SpatialAudio`
      prim's position. If the audio media contains any form of stereo or other
      multi-channel sound, it is left to the application to determine whether
      the listener's position should be taken into account. We expect
      *nonSpatial* to be the choice for ambient sounds and music sound-tracks.

      For now, we consider *oriented* and *spread-limited* sounds, which are
      supported by major game engines, as beyond scope. However, in anticipation
      of future support of these features, in consideration to authoring tools
      needing to author transformations for :usda:`SpatialAudio` prims *now* ,
      **we stipulate that the emission direction for** :usda:`SpatialAudio`
      **prims is the -Z axis** , which matches the orientation of directional
      lights in `UsdLux
      <api/usd_lux_page_front.html>`_ .

    * **uniform token playbackMode = "onceFromStart"** 

      For the fallback value of "once" play the referenced audio once, beginning
      at :bi:`startTime`, continuing until :bi:`endTime`, if :bi:`endTime` is
      greater than :bi:`startTime`, otherwise until the audio completes. For any
      value other than "once", **loop** the audio file to fill time, as
      described below. If specified, :bi:`mediaOffset` is applied to the first
      run-through of the audio clip with the second and all other loops
      beginning from the start of the audio clip. In future we can add
      additional attributes that can identify cut-points within the referenced
      media such that we can continuously loop a subsection of the referenced
      clip, but for now we choose to start more simply. Non-fallback values of
      :bi:`playbackMode` determine when looping should begin and end: 

      * *onceFromStart :* Play the audio once from :bi:`startTime`, continuing 
        until the audio clip completes.

      * *onceFromStartToEnd :* Play the audio once from :bi:`startTime` to
        :bi:`endTime` or until the audio clip completes, whichever
        comes first. See :ref:`SdfTimeCode and TimeScaling
        <wp_usdaudio:sdftimecode and time scaling>` to understand how this
        mode is affected by time-scaling. 

      * *loopFromStart :* Start playing the audio at :bi:`startTime` and
        continue looping through to the stage's authored :bi:`endTimeCode`.

      * *loopFromStartToEnd :* Start playing the audio at :bi:`startTime`
        and continue looping through, stopping the audio at :bi:`endTime`. See
        :ref:`SdfTimeCode and TimeScaling<wp_usdaudio:sdftimecode and time scaling>` 
        to understand how this mode is affected by time-scaling. 

      * *loopFromStage :* Start playing the audio at the containing stage's a
        uthored :bi:`startTimeCode`, and continue looping through to the stage's
        authored :bi:`endTimeCode`. This can be useful for ambient sounds that
        should always be active.

    * **uniform timeCode startTime = 0** 

      Expressed in the :ref:`timeCodesPerSecond<glossary:TimeCode>` of the
      containing stage, ***startTime*** specifies when the audio stream will
      start playing during animation playback. See :ref:`SdfTimeCode and
      TimeScaling<wp_usdaudio:sdftimecode and time scaling>` to understand how
      this property resolves on a stage.

    * **uniform timeCode endTime = 0** 

      Expressed in the :ref:`timeCodesPerSecond<glossary:TimeCode>` of the
      containing stage, ***endTime*** specifies when the audio stream will cease
      playing during animation playback, if the length of the referenced audio
      clip is longer than desired.  If **e *ndTime*** is less than or equal to
      **startTime** , then play the audio stream to its completion.  See
      :ref:`SdfTimeCode and TimeScaling<wp_usdaudio:sdftimecode and time
      scaling>` to understand how this property resolves on a stage.

    * **uniform double mediaOffset = 0** 

      Expressed in **seconds** , :bi:`mediaOffset` specifies the offset from the
      referenced audio file's beginning at which we should begin playback when
      stage playback reaches the prim's :bi:`startTime`. If the prim's
      :bi:`playbackMode` is a looping mode, :bi:`mediaOffset` is applied only to
      the first run-through of the audio clip; the second and all other loops
      begin from the start of the audio clip.

    * **double gain = 1.0** 

      Float multiplier on the incoming audio signal. A value of 0 "mutes" the
      signal. Negative values will be clamped to 0. Although gain is commonly
      expressed in dB in the audio world, that formulation requires the addition
      of an extra animatable :bi:`mute` property since dB can only express
      ratios of signal, not an absolute scale factor, except theoretically as
      :math:`- inf` dB. Further, given the intended use of SpatialAudio for
      content delivery in usdz assets, the commonality with the `Web Audio API
      <https://www.w3.org/TR/webaudio/#GainNode>`_ seems relevant.

USD Sample 
==========

Here is an example of two different kinds of sounds encoded in USD with the
SpatialAudio schema.

.. code-block:: usda

   #usda 1.0
   (
      upAxis = "Z"
      endTimeCode = 200
      startTimeCode = 1
      timeCodesPerSecond = 24
   )
   
   def Xform "Sounds"
   {
       def SpatialAudio "AmbientSound"
       {
           # We need not encode startTime, mediaOffset, or level as the fallback 
           # values suffice for ambient sound.  Playback will begin at timeCode 1
           uniform asset filePath       = @AmbientSound.mp3@
           uniform token auralMode      = "nonSpatial"
           uniform token playbackMode   = "loopFromStage"
       }
   
       def SpatialAudio "WoodysVoice"
       {
           # SpatialAudio xform.  This prim might typically be located
           # as a child of the "Woody" model so that it's location need
           # only be specified relative to Woody, rather than replicating
           # Woody's animation
           double3 xformOp:translate     = (3.0, -3.0, 2)
           uniform token[] xformOpOrder  = ["xformOp:translate"]
   
   
           # SpatialAudio Properties.  We have left the playbackMode at its
           # fallback of "onceFromStart", so we do not require an endTime:
           # the sound will play to completion
           uniform asset  filePath       = @WoodysVoice.mp3@
           uniform token  auralMode      = "spatial"
           uniform timeCode startTime =  65.0
           # Skip the first third of a second in WoodysVoice.mp3 
           uniform double mediaOffset    =  0.33333333333
       }
   }

Other Notes/Questions
=====================

SdfTimeCode and Time Scaling
############################

USD release 19.11 introduced a new value type, `SdfTimeCode
<api/class_sdf_time_code.html#details>`_, to accommodate the needs of the
:usda:`SpatialAudio` schema, in which we need to encode time **as attribute
values** in USD layers, and expect that those values be properly adjusted during
value resolution such that any :ref:`SdfLayerOffsets<glossary:Layer Offset>`
affecting the layer in which the :usda:`SdfTimeCode` values are authored are
applied to the values themselves. This is necessary so that any audio that is
synchronized to animation remains synchronized as the animation is affected by
SdfLayerOffsets in the composition. This behavior brings with it some
interesting edge-cases and caveats:

* Inverting startTime and endTime

  Although we do not expect it to be common, it is *possible* to apply a
  negative time scale to USD layers, which *mostly* has the effect of reversing
  animation in the affected composition. If a negative scale is applied to a
  composition that contains authored :bi:`startTime` and :bi:`endTime`, it will
  reverse their relative ordering in time. Therefore, we stipulate for the
  :bi:`playbackModes` of "onceFromStartToEnd" and "loopFromStartToEnd", **if**
  :bi:`endTime` **is less than** :bi:`startTime` **, then begin playback at**
  :bi:`endTime` **, and continue until startTime.** When :bi:`startTime` and
  :bi:`endTime` are inverted, we do not, however, stipulate that playback of the
  audio media itself be inverted, since doing so "successfully" would require
  perfect knowledge of when, within the audio clip, relevant audio ends (so that
  we know how to offset the reversed audio to align it so that we reach the
  "beginning" at :bi:`startTime`), and sounds played in reverse are not likely
  to produce desirable results.

* :usda:`SdfLayerOffsets` do not Affect Media Dilation

  Although authored time-transformations will affect the value and ordering of
  :bi:`startTime` and :bi:`endTime`, we make no attempt to infer any playback
  dilation of the actual audio media itself. Given that :bi:`startTime` and
  :bi:`endTime` can be independently authored in different layers with differing
  time-scales, it is not possible, in general, to define an "original timeframe"
  from which we can compute a dilation to composed stage-time. Even if we could
  compute a composed dilation this way, it would still be impossible to flatten
  a stage or layer stack into a single layer and still retain the composed audio
  dilation using this schema.

Extent/Falloff for Ambient Sounds
*********************************

Ambient sounds may be associated with sub-sections (levels, areas, etc) of a
scene. A common way to represent such location-based effects is with a
parameterized spatial attenuation curve. We may consider adding such controls
in the future, but leave them out for simplicity, currently.

Usdz Considerations
*******************

In general, the formats allowed for audio files is no more constrained by USD
than is image-type. As with images, however, Usdz has stricter requirements
based on DMA and format support in browsers and consumer devices. We propose the
allowed audio filetypes for usdz be **M4A, MP3, !WAV** (in order of preference);
the :ref:`spec_usdz:Usdz Specification` has been accordingly updated.

Reference Implementation
************************

Unlike the majority of 3D-graphics-related features added to USD, for which we
strive to provide computation API's (if relevant) and reference imaging support
in Hydra and one or more of its renderers, the :usda:`SpatialAudio` schema
represents "external" data, for which we do not have an existing in-house
solution that could be easily adapted, nor existing workflows that would
exercise it (spatial audio, especially). Therefore it may be some time before we
are able to provide a reference implementation.

A nearer-term, graphical representation of :usda:`SpatialAudio` prims in Hydra
viewports seems reasonable: :usda:`SpatialAudio` prims would have a fallback
**purpose** of "guide", with a simple, oriented icon/card representation
indicating the position and orientation of the emitter.

