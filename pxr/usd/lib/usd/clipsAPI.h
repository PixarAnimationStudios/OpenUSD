//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USD_GENERATED_CLIPSAPI_H
#define USD_GENERATED_CLIPSAPI_H

/// \file usd/clipsAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// CLIPSAPI                                                                   //
// -------------------------------------------------------------------------- //

/// \class UsdClipsAPI
///
/// UsdClipsAPI is an API schema that provides an interface to
/// a prim's clip metadata. Clips are a "value resolution" feature that 
/// allows one to specify a sequence of usd files (clips) to be consulted, 
/// over time, as a source of varying overrides for the prims at and 
/// beneath this prim in namespace.
/// 
/// SetClipAssetPaths() establishes the set of clips that can be consulted.
/// SetClipActive() specifies the ordering of clip application over time 
/// (clips can be repeated), while SetClipTimes() specifies time-mapping
/// from stage-time to clip-time for the clip active at a given stage-time,
/// which allows for time-dilation and repetition of clips. 
/// Finally, SetClipPrimPath() determines the path within each clip that will 
/// map to this prim, i.e. the location within the clip at which we will look
/// for opinions for this prim. 
/// 
/// The clip asset paths, times and active metadata can also be specified 
/// through template clip metadata. This can be desirable when your set of 
/// assets is very large, as the template metadata is much more concise. 
/// SetClipTemplateAssetPath() establishes the asset identifier pattern of the 
/// set of clips to be consulted. SetClipTemplateStride(), 
/// SetClipTemplateEndTime(), and SetClipTemplateStartTime() specify the range 
/// in which USD will search, based on the template. From the set of resolved 
/// asset paths, times and active will be derived internally.
/// 
/// A prim may have multiple "clip sets" -- named sets of clips that each
/// have their own values for the metadata described above. For example, 
/// a prim might have a clip set named "Clips_1" that specifies some group
/// of clip asset paths, and another clip set named "Clips_2" that uses
/// an entirely different set of clip asset paths. These clip sets are 
/// composed across composition arcs, so clip sets for a prim may be
/// defined in multiple sublayers or references, for example. Individual
/// metadata for a given clip set may be sparsely overridden.
/// 
/// Important facts about clips:            
/// \li Within the layerstack in which clips are established, the           
/// opinions within the clips will be em weaker than any direct opinions
/// in the layerstack, but em stronger than varying opinions coming across
/// references and variants.            
/// \li We will never look for metadata or default opinions in clips            
/// when performing value resolution on the owning stage, since these           
/// quantities must be time-invariant.          
/// 
/// This leads to the common structure in which we reference a model asset
/// on a prim, and then author clips at the same site: the asset reference
/// will provide the topology and unvarying data for the model, while the 
/// clips will provide the time-sampled animation.
/// 
/// For further information, see \ref Usd_Page_ValueClips
/// 
///
class UsdClipsAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdClipsAPI on UsdPrim \p prim .
    /// Equivalent to UsdClipsAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdClipsAPI(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdClipsAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdClipsAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdClipsAPI(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USD_API
    virtual ~UsdClipsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdClipsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdClipsAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USD_API
    static UsdClipsAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USD_API
    virtual const TfType &_GetTfType() const;

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
    //

    // --------------------------------------------------------------------- //
    /// \anchor Usd_ClipInfo_API
    /// \name Value Clip Info
    ///
    /// Setters and getters for interacting with metadata that control
    /// value clip behavior.
    ///
    /// @{
    // --------------------------------------------------------------------- //

    /// Dictionary that contains the definition of the clip sets on this prim.
    ///
    /// Each entry in this dictionary defines a clip set: the entry's key
    /// is the name of the clip set and the entry's value is a dictionary
    /// containing the metadata that specifies the clips in the set.
    ///
    /// See \ref UsdClipsAPIInfoKeys for the keys used for each clip set's
    /// dictionary, or use the other API to set or get values for a given
    /// clip set.
    USD_API
    bool GetClips(VtDictionary* clips) const;

    /// Set the clips dictionary for this prim.
    /// \sa GetClips
    USD_API
    bool SetClips(const VtDictionary& clips);

    /// ListOp that may be used to affect how opinions from clip 
    /// sets are applied during value resolution.
    ///
    /// By default, clip sets in a layer stack are examined in 
    /// lexicographical order by name for attribute values during value 
    /// resolution. The clip sets listOp can be used to reorder the clip 
    /// sets in a layer stack or remove them entirely from consideration
    /// during value resolution without modifying the clips dictionary.
    ///
    /// This is *not* the list of clip sets that are authored on this prim.
    /// To retrieve that information, use GetClips to examine the clips 
    /// dictionary directly.
    ///
    /// This function returns the clip sets listOp from the current edit
    /// target.
    USD_API
    bool GetClipSets(SdfStringListOp* clipSets) const;

    /// Set the clip sets list op for this prim.
    /// \sa GetClipSets
    USD_API
    bool SetClipSets(const SdfStringListOp& clipSets);

    /// List of asset paths to the clips in the clip set named \p clipSet.
    /// This list is unordered, but elements in this list are referred to 
    /// by index in other clip-related fields.
    USD_API
    bool GetClipAssetPaths(VtArray<SdfAssetPath>* assetPaths,
                           const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipAssetPaths(VtArray<SdfAssetPath>* assetPaths) const;

    /// Set the clip asset paths for the clip set named \p clipSet
    /// \sa GetClipAssetPaths()
    USD_API
    bool SetClipAssetPaths(const VtArray<SdfAssetPath>& assetPaths,
                           const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipAssetPaths(const VtArray<SdfAssetPath>& assetPaths);

    /// Path to the prim in the clips in the clip set named \p clipSet
    /// from which time samples will be read.
    ///
    /// This prim's path will be substituted with this value to determine
    /// the final path in the clip from which to read data. For instance,
    /// if this prims' path is '/Prim_1', the clip prim path is '/Prim', 
    /// and we want to get values for the attribute '/Prim_1.size'. The
    /// clip prim path will be substituted in, yielding '/Prim.size', and
    /// each clip will be examined for values at that path.
    USD_API
    bool GetClipPrimPath(std::string* primPath, 
                         const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipPrimPath(std::string* primPath) const;

    /// Set the clip prim path for the clip set named \p clipSet.
    /// \sa GetClipPrimPath()
    USD_API
    bool SetClipPrimPath(const std::string& primPath, 
                         const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipPrimPath(const std::string& primPath);

    /// List of pairs (time, clip index) indicating the time on the stage
    /// at which the clip in the clip set named \p clipSet specified by 
    /// the clip index is active. For instance, a value of 
    /// [(0.0, 0), (20.0, 1)] indicates that clip 0 is active at time 0 
    /// and clip 1 is active at time 20.
    USD_API
    bool GetClipActive(VtVec2dArray* activeClips, 
                       const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipActive(VtVec2dArray* activeClips) const;

    /// Set the active clip metadata for the clip set named \p clipSet.
    /// \sa GetClipActive()
    USD_API
    bool SetClipActive(const VtVec2dArray& activeClips, 
                       const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipActive(const VtVec2dArray& activeClips);

    /// List of pairs (stage time, clip time) indicating the time in the
    /// active clip in the clip set named \p clipSet that should be 
    /// consulted for values at the corresponding stage time. 
    ///
    /// During value resolution, this list will be sorted by stage time; 
    /// times will then be linearly interpolated between consecutive entries.
    /// For instance, for clip times [(0.0, 0.0), (10.0, 20.0)], 
    /// at stage time 0, values from the active clip at time 0 will be used,
    /// at stage time 5, values from the active clip at time 10, and at stage 
    /// time 10, clip values at time 20.
    USD_API
    bool GetClipTimes(VtVec2dArray* clipTimes, 
                      const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipTimes(VtVec2dArray* clipTimes) const;

    /// Set the clip times metadata for this prim.
    /// \sa GetClipTimes()
    USD_API
    bool SetClipTimes(const VtVec2dArray& clipTimes, 
                      const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipTimes(const VtVec2dArray& clipTimes);

    /// Asset path for the clip manifest for the clip set named \p clipSet. 
    /// The clip manifest indicates which attributes have time samples 
    /// authored in the clips specified on this prim. During value resolution, 
    /// clips will only be examined if the attribute exists and is declared 
    /// as varying in the manifest. Note that the clip manifest is only 
    /// consulted to check if an attribute exists and what its variability is. 
    /// Other values and metadata authored in the manifest will be ignored.
    ///
    /// For instance, if this prim's path is </Prim_1>, the clip prim path is
    /// </Prim>, and we want values for the attribute </Prim_1.size>, we will
    /// only look within this prim's clips if the attribute </Prim.size>
    /// exists and is varying in the manifest.
    USD_API
    bool GetClipManifestAssetPath(SdfAssetPath* manifestAssetPath,
                                  const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipManifestAssetPath(SdfAssetPath* manifestAssetPath) const;

    /// Set the clip manifest asset path for this prim.
    /// \sa GetClipManifestAssetPath()
    USD_API
    bool SetClipManifestAssetPath(const SdfAssetPath& manifestAssetPath,
                                  const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipManifestAssetPath(const SdfAssetPath& manifestAssetPath);

    /// A template string representing a set of assets to be used as clips
    /// for the clip set named \p clipSet. This string can be of two forms: 
    ///
    /// integer frames: path/basename.###.usd 
    ///
    /// subinteger frames: path/basename.##.##.usd.
    ///
    /// For the integer portion of the specification, USD will take 
    /// a particular time, determined by the template start time, stride, and
    /// end time, and pad it with zeros up to the number of hashes provided 
    /// so long as the number of hashes is greater than the digits required 
    /// to specify the integer value.
    ///
    /// For instance:
    ///
    ///    time = 12,  template asset path = foo.##.usd  => foo.12.usd
    ///    time = 12,  template asset path = foo.###.usd => foo.012.usd
    ///    time = 333, template asset path = foo.#.usd   => foo.333.usd
    ///
    /// In the case of subinteger portion of a specifications, USD requires the 
    /// specification to be exact. 
    ///
    /// For instance:
    /// 
    ///    time = 1.15,  template asset path = foo.#.###.usd => foo.1.150.usd
    ///    time = 1.145, template asset path = foo.#.##.usd  => foo.1.15.usd
    ///    time = 1.1,   template asset path = foo.#.##.usd  => foo.1.10.usd
    ///
    /// Note that USD requires that hash groups be adjacent in the string, 
    /// and that there only be one or two such groups.
    USD_API
    bool GetClipTemplateAssetPath(std::string* clipTemplateAssetPath, 
                                  const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipTemplateAssetPath(std::string* clipTemplateAssetPath) const;

    /// Set the clip template asset path for the clip set named \p clipSet.
    /// \sa GetClipTemplateAssetPath
    USD_API
    bool SetClipTemplateAssetPath(const std::string& clipTemplateAssetPath, 
                                  const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipTemplateAssetPath(const std::string& clipTemplateAssetPath);

    /// A double representing the increment value USD will use when
    /// searching for asset paths for the clip set named \p clipSet.
    /// \sa GetClipTemplateAssetPath.
    USD_API
    bool GetClipTemplateStride(double* clipTemplateStride, 
                               const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipTemplateStride(double* clipTemplateStride) const;

    /// Set the template stride for the clip set named \p clipSet.
    /// \sa GetClipTemplateStride()
    USD_API
    bool SetClipTemplateStride(const double clipTemplateStride, 
                               const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipTemplateStride(const double clipTemplateStride);

    /// A double which indicates the start of the range USD will use 
    /// to search for asset paths for the clip set named \p clipSet. 
    /// This value is inclusive in that range.
    /// \sa GetClipTemplateAssetPath.
    USD_API
    bool GetClipTemplateStartTime(double* clipTemplateStartTime, 
                                  const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipTemplateStartTime(double* clipTemplateStartTime) const;

    /// Set the template start time for the clip set named \p clipSet.
    /// \sa GetClipTemplateStartTime
    USD_API
    bool SetClipTemplateStartTime(const double clipTemplateStartTime, 
                                  const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipTemplateStartTime(const double clipTemplateStartTime);

    /// A double which indicates the end of the range USD will use to
    /// to search for asset paths for the clip set named \p clipSet. 
    /// This value is inclusive in that range.
    /// \sa GetClipTemplateAssetPath.
    USD_API
    bool GetClipTemplateEndTime(double* clipTemplateEndTime, 
                                const std::string& clipSet) const;
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool GetClipTemplateEndTime(double* clipTemplateEndTime) const;

    /// Set the template end time for the clipset named \p clipSet. 
    /// \sa GetClipTemplateEndTime()
    USD_API
    bool SetClipTemplateEndTime(const double clipTemplateEndTime, 
                                const std::string& clipSet);
    /// \overload
    /// This function operates on the default clip set. 
    /// \sa \ref UsdClipsAPISetNames
    USD_API
    bool SetClipTemplateEndTime(const double clipTemplateEndTime);

    /// Return true if the setter functions that do not take a clip set author
    /// values to legacy metadata fields (e.g. clipAssetPaths, clipTimes, etc.),
    /// or false if values are authored to the default clip set. 
    /// 
    /// This is controlled by the USD_AUTHOR_LEGACY_CLIPS environment variable
    /// and is intended to be an aid for transitioning.
    USD_API
    static bool IsAuthoringLegacyClipMetadata();

    /// @}

};

/// \hideinitializer
#define USDCLIPS_INFO_KEYS  \
    (active)                \
    (assetPaths)            \
    (manifestAssetPath)     \
    (primPath)              \
    (templateAssetPath)     \
    (templateEndTime)       \
    (templateStartTime)     \
    (templateStride)        \
    (times)                 \

/// \anchor UsdClipsAPIInfoKeys
///
/// <b>UsdClipsAPIInfoKeys</b> provides tokens for the various entries
/// in the clips dictionary. UsdClipsAPI provides named API corresponding
/// to each of these entries; see documentation on API for expected values.
///
/// \sa UsdClipsAPI::GetClips
///
/// The keys provided here are:
/// \li <b>active</b> - see UsdClipsAPI::GetClipActive
/// \li <b>assetPaths</b> - see UsdClipsAPI::GetClipAssetPaths
/// \li <b>manifestAssetPath</b> - see UsdClipsAPI::GetClipManifestAssetPath
/// \li <b>primPath</b> - see UsdClipsAPI::GetClipPrimPath
/// \li <b>templateAssetPath</b> - see UsdClipsAPI::GetClipTemplateAssetPath
/// \li <b>templateEndTime</b> - see UsdClipsAPI::GetClipTemplateEndTime
/// \li <b>templateStartTime</b> - see UsdClipsAPI::GetClipTemplateStartTime
/// \li <b>templateStride</b> - see UsdClipsAPI::GetClipTemplateStride
/// \li <b>times</b> - see UsdClipsAPI::GetClipTimes
TF_DECLARE_PUBLIC_TOKENS(UsdClipsAPIInfoKeys, USD_API, USDCLIPS_INFO_KEYS);

/// \hideinitializer
#define USDCLIPS_SET_NAMES     \
    ((default_, "default"))    \

/// \anchor UsdClipsAPISetNames
///
/// <b>UsdClipsAPISetNames</b> provides tokens for pre-defined clip set
/// names that may be used with the 
/// \ref Usd_ClipInfo_API "value clip info functions" on UsdClipsAPI.
///
/// The tokens are:
/// \li <b>default_</b> - The default clip set used for API where no clip set is specified.
TF_DECLARE_PUBLIC_TOKENS(UsdClipsAPISetNames, USD_API, USDCLIPS_SET_NAMES);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
