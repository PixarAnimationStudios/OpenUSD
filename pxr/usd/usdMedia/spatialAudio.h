//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDMEDIA_GENERATED_SPATIALAUDIO_H
#define USDMEDIA_GENERATED_SPATIALAUDIO_H

/// \file usdMedia/spatialAudio.h

#include "pxr/pxr.h"
#include "pxr/usd/usdMedia/api.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdMedia/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// SPATIALAUDIO                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdMediaSpatialAudio
///
/// The SpatialAudio primitive defines basic properties for encoding 
/// playback of an audio file or stream within a USD Stage. The SpatialAudio 
/// schema derives from UsdGeomXformable since it can support full spatial 
/// audio while also supporting non-spatial mono and stereo sounds. One or 
/// more SpatialAudio prims can be placed anywhere in the namespace, though it 
/// is advantageous to place truly spatial audio prims under/inside the models 
/// from which the sound emanates, so that the audio prim need only be 
/// transformed relative to the model, rather than copying its animation.
/// 
/// \section Usd_SpatialAudio_TimeScaling Timecode Attributes and Time Scaling
/// \a startTime and \a endTime are \ref SdfTimeCode "timecode" valued 
/// attributes which gives them the special behavior that 
/// \ref SdfLayerOffset "layer offsets" affecting the layer in 
/// which one of these values is authored are applied to the attribute's value 
/// itself during value resolution. This allows audio playback to be kept in 
/// sync with time sampled animation as the animation is affected by 
/// \ref SdfLayerOffset "layer offsets" in the composition. But this behavior 
/// brings with it some interesting edge cases and caveats when it comes to 
/// \ref SdfLayerOffset "layer offsets" that include scale.
/// 
/// ####  Layer Offsets do not affect Media Dilation
/// Although authored layer offsets may have a time scale which can scale the
/// duration between an authored \a startTime and \a endTime, we make no 
/// attempt to infer any playback dilation of the actual audio media itself. 
/// Given that \a startTime and \a endTime can be independently authored in 
/// different layers with differing time scales, it is not possible, in general,
/// to define an "original timeframe" from which we can compute a dilation to 
/// composed stage-time. Even if we could compute a composed dilation this way,
/// it would still be impossible to flatten a stage or layer stack into a single
/// layer and still retain the composed audio dilation using this schema.
/// 
/// #### Inverting startTime and endTime
/// Although we do not expect it to be common, it is possible to apply a 
/// negative time scale to USD layers, which mostly has the effect of reversing
/// animation in the affected composition. If a negative scale is applied to a
/// composition that contains authored \a startTime and \a endTime, it will
/// reverse their relative ordering in time. Therefore, we stipulate when
/// \a playbackMode is "onceFromStartToEnd" or "loopFromStartToEnd", if 
/// \a endTime is less than \a startTime, then begin playback at \a endTime, 
/// and continue until \a startTime. When \a startTime and \a endTime are 
/// inverted, we do not, however, stipulate that playback of the audio media 
/// itself be inverted, since doing so "successfully" would require perfect 
/// knowledge of when, within the audio clip, relevant audio ends (so that we 
/// know how to offset the reversed audio to align it so that we reach the 
/// "beginning" at \a startTime), and sounds played in reverse are not likely 
/// to produce desirable results. 
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdMediaTokens.
/// So to set an attribute to the value "rightHanded", use UsdMediaTokens->rightHanded
/// as the value.
///
class UsdMediaSpatialAudio : public UsdGeomXformable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdMediaSpatialAudio on UsdPrim \p prim .
    /// Equivalent to UsdMediaSpatialAudio::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdMediaSpatialAudio(const UsdPrim& prim=UsdPrim())
        : UsdGeomXformable(prim)
    {
    }

    /// Construct a UsdMediaSpatialAudio on the prim held by \p schemaObj .
    /// Should be preferred over UsdMediaSpatialAudio(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdMediaSpatialAudio(const UsdSchemaBase& schemaObj)
        : UsdGeomXformable(schemaObj)
    {
    }

    /// Destructor.
    USDMEDIA_API
    virtual ~UsdMediaSpatialAudio();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDMEDIA_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdMediaSpatialAudio holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdMediaSpatialAudio(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDMEDIA_API
    static UsdMediaSpatialAudio
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDMEDIA_API
    static UsdMediaSpatialAudio
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDMEDIA_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDMEDIA_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDMEDIA_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // FILEPATH 
    // --------------------------------------------------------------------- //
    /// Path to the audio file.
    /// In general, the formats allowed for audio files is no more constrained 
    /// by USD than is image-type. As with images, however, usdz has stricter 
    /// requirements based on DMA and format support in browsers and consumer 
    /// devices. The allowed audio filetypes for usdz are M4A, MP3, WAV 
    /// (in order of preference).
    /// \sa <a href="https://openusd.org/release/spec_usdz.html">Usdz Specification</a>
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform asset filePath = @@` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetFilePathAttr() const;

    /// See GetFilePathAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateFilePathAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // AURALMODE 
    // --------------------------------------------------------------------- //
    /// Determines how audio should be played.
    /// Valid values are:
    /// - spatial: Play the audio in 3D space if the device can support spatial
    /// audio. if not, fall back to mono.
    /// - nonSpatial: Play the audio without regard to the SpatialAudio prim's 
    /// position. If the audio media contains any form of stereo or other 
    /// multi-channel sound, it is left to the application to determine 
    /// whether the listener's position should be taken into account. We 
    /// expect nonSpatial to be the choice for ambient sounds and music 
    /// sound-tracks.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token auralMode = "spatial"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdMediaTokens "Allowed Values" | spatial, nonSpatial |
    USDMEDIA_API
    UsdAttribute GetAuralModeAttr() const;

    /// See GetAuralModeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateAuralModeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PLAYBACKMODE 
    // --------------------------------------------------------------------- //
    /// Along with \a startTime and \a endTime, determines when the 
    /// audio playback should start and stop during the stage's animation 
    /// playback and whether the audio should loop during its duration. 
    /// Valid values are:
    /// - onceFromStart: Play the audio once, starting at \a startTime, 
    /// continuing until the audio completes.
    /// - onceFromStartToEnd: Play the audio once beginning at \a startTime, 
    /// continuing until \a endTime or until the audio completes, whichever 
    /// comes first.
    /// - loopFromStart: Start playing the audio at \a startTime and continue 
    /// looping through to the stage's authored \a endTimeCode.
    /// - loopFromStartToEnd: Start playing the audio at \a startTime and 
    /// continue looping through, stopping the audio at \a endTime.
    /// - loopFromStage: Start playing the audio at the stage's authored 
    /// \a startTimeCode and continue looping through to the stage's authored 
    /// \a endTimeCode. This can be useful for ambient sounds that should always 
    /// be active.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token playbackMode = "onceFromStart"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdMediaTokens "Allowed Values" | onceFromStart, onceFromStartToEnd, loopFromStart, loopFromStartToEnd, loopFromStage |
    USDMEDIA_API
    UsdAttribute GetPlaybackModeAttr() const;

    /// See GetPlaybackModeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreatePlaybackModeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STARTTIME 
    // --------------------------------------------------------------------- //
    /// Expressed in the timeCodesPerSecond of the containing stage, 
    /// \a startTime specifies when the audio stream will start playing during 
    /// animation playback. This value is ignored when \a playbackMode is set 
    /// to loopFromStage as, in this mode, the audio will always start at the 
    /// stage's authored \a startTimeCode.
    /// Note that \a startTime is expressed as a timecode so that the stage can 
    /// properly apply layer offsets when resolving its value. See 
    /// \ref Usd_SpatialAudio_TimeScaling for more details and caveats.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform timecode startTime = 0` |
    /// | C++ Type | SdfTimeCode |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->TimeCode |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetStartTimeAttr() const;

    /// See GetStartTimeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateStartTimeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ENDTIME 
    // --------------------------------------------------------------------- //
    /// Expressed in the timeCodesPerSecond of the containing stage, 
    /// \a endTime specifies when the audio stream will cease playing during 
    /// animation playback if the length of the referenced audio clip is 
    /// longer than desired. This only applies if \a playbackMode is set to 
    /// onceFromStartToEnd or loopFromStartToEnd, otherwise the \a endTimeCode 
    /// of the stage is used instead of \a endTime.
    /// If \a endTime is less than \a startTime, it is expected that the audio 
    /// will instead be played from \a endTime to \a startTime.
    /// Note that \a endTime is expressed as a timecode so that the stage can 
    /// properly apply layer offsets when resolving its value.
    /// See \ref Usd_SpatialAudio_TimeScaling for more details and caveats.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform timecode endTime = 0` |
    /// | C++ Type | SdfTimeCode |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->TimeCode |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetEndTimeAttr() const;

    /// See GetEndTimeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateEndTimeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MEDIAOFFSET 
    // --------------------------------------------------------------------- //
    /// Expressed in seconds, \a mediaOffset specifies the offset from 
    /// the referenced audio file's beginning at which we should begin playback 
    /// when stage playback reaches the time that prim's audio should start.
    /// If the prim's \a playbackMode is a looping mode, \a mediaOffset is 
    /// applied only to the first run-through of the audio clip; the second and 
    /// all other loops begin from the start of the audio clip.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform double mediaOffset = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDMEDIA_API
    UsdAttribute GetMediaOffsetAttr() const;

    /// See GetMediaOffsetAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateMediaOffsetAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // GAIN 
    // --------------------------------------------------------------------- //
    /// Multiplier on the incoming audio signal. A value of 0 "mutes" 
    /// the signal. Negative values will be clamped to 0. 
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double gain = 1` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    USDMEDIA_API
    UsdAttribute GetGainAttr() const;

    /// See GetGainAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMEDIA_API
    UsdAttribute CreateGainAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
