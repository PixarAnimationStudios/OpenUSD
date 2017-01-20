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

#include "pxr/usd/usd/schemaBase.h"
#include "pxr/pxr.h"
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
/// beneath this model prim in namespace.
/// 
/// Clips are a "value resolution" feature that allows one to specify           
/// a sequence of usd files (clips) to be consulted, over time, as a source         
/// of varying overrides for the prims at and beneath this model prim in            
/// namespace.          
/// 
/// SetClipAssetPaths() establishes the set of clips that can be consulted.         
/// SetClipActive() specifies the ordering of clip application over time            
/// (clips can be repeated), while SetClipTimes() specifies time-mapping            
/// from stage-time to clip-time for the clip active at a given stage-time,         
/// which allows for time-dilation and repetition of clips. 
/// Finally, SetClipPrimPath() determines the path within each clip that will map            
/// to this prim, i.e. the location within the clip at which we will look           
/// for opinions for this prim. 
/// 
/// The clipAssetPaths, clipTimes and clipActive metadata can also be specified 
/// through template clip metadata. This can be desirable when your set of 
/// assets is very large, as the template metadata is much more concise. 
/// SetClipTemplateAssetPath() establishes the asset identifier pattern of the set of
/// clips to be consulted. SetClipTemplateStride(), SetClipTemplateEndTime(), 
/// and SetClipTemplateStartTime() specify the range in which USD will search, based
/// on the template. From the set of resolved asset paths, clipTimes, and clipActive
/// will be derived internally.
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
/// will provide the topology and unvarying data for the model, while           
/// the clips will provide the time-sampled animation.
/// 
/// For further information, see \ref Usd_AdvancedFeatures_ClipsOverview 
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
    virtual ~UsdClipsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
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
    static UsdClipsAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
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

    /// List of asset paths to the clips for this prim. This list is
    /// unordered, but elements in this list are referred to by index in
    /// other clip-related fields.
    bool GetClipAssetPaths(VtArray<SdfAssetPath>* assetPaths) const;
    /// Set the clipAssetPaths metadata for this prim.
    /// \sa GetClipAssetPaths()
    bool SetClipAssetPaths(const VtArray<SdfAssetPath>& assetPaths);

    /// Path to the prim in the clips from which time samples will be read.
    /// This prim's path will be substituted with this value to determine
    /// the final path in the clip from which to read data. For instance,
    /// if this prims' path is '/Prim_1', the clip prim path is '/Prim', 
    /// and we want to get values for the attribute '/Prim_1.size'. The
    /// clip prim path will be substituted in, yielding '/Prim.size', and
    /// each clip will be examined for values at that path.
    bool GetClipPrimPath(std::string* primPath) const;
    /// Set the clipPrimPath metadata for this prim.
    /// \sa GetClipPrimPath()
    bool SetClipPrimPath(const std::string& primPath);

    /// List of pairs (time, clip index) indicating the time on the stage
    /// at which the clip specified by the clip index is active. For instance,
    /// a value of [(0.0, 0), (20.0, 1)] indicates that clip 0 is active
    /// at time 0 and clip 1 is active at time 20.
    bool GetClipActive(VtVec2dArray* activeClips) const;
    /// Set the clipActive metadata for this prim.
    /// \sa GetClipActive()
    bool SetClipActive(const VtVec2dArray& activeClips);

    /// List of pairs (stage time, clip time) indicating the time in the
    /// active clip that should be consulted for values at the corresponding
    /// stage time. 
    ///
    /// During value resolution, this list will be sorted by stage time; 
    /// times will then be linearly interpolated between consecutive entries.
    /// For instance, for clip times [(0.0, 0.0), (10.0, 20.0)], 
    /// at stage time 0, values from the active clip at time 0 will be used,
    /// at stage time 5, values from the active clip at time 10, and at stage 
    /// time 10, clip values at time 20.
    bool GetClipTimes(VtVec2dArray* clipTimes) const;
    /// Set the clipTimes metadata for this prim.
    /// \sa GetClipTimes()
    bool SetClipTimes(const VtVec2dArray& clipTimes);

    /// Asset path for the clip manifest. The clip manifest indicates which
    /// attributes have time samples authored in the clips specified on this
    /// prim. During value resolution, we will only look for time samples 
    /// in clips if the attribute exists and is declared as varying in the
    /// manifest. Note that the clip manifest is only consulted to check
    /// check if an attribute exists and what its variability is. Other values
    /// and metadata authored in the manifest will be ignored.
    ///
    /// For instance, if this prim's path is </Prim_1>, the clip prim path is
    /// </Prim>, and we want values for the attribute </Prim_1.size>, we will
    /// only look within this prim's clips if the attribute </Prim.size>
    /// exists and is varying in the manifest.
    bool GetClipManifestAssetPath(SdfAssetPath* manifestAssetPath) const;
    /// Set the clipManifestAssetPath metadata for this prim.
    /// \sa GetClipManifestAssetPath()
    bool SetClipManifestAssetPath(const SdfAssetPath& manifestAssetPath);

    /// A template string representing a set of assets. This string
    /// can be of two forms: 
    ///
    /// integer frames: path/basename.###.usd 
    ///
    /// subinteger frames: path/basename.##.##.usd.
    ///
    /// For the integer portion of the specification, USD will take 
    /// a particular time, determined by the clipTemplateStartTime, 
    /// clipTemplateStride and clipTemplateEndTime, and pad it with 
    /// zeros up to the number of hashes provided so long as the number of hashes 
    /// is greater than the digits required to specify the integer value.
    ///
    /// For instance:
    ///
    ///    time = 12,  clipTemplateAssetPath = foo.##.usd  => foo.12.usd
    ///    time = 12,  clipTemplateAssetPath = foo.###.usd => foo.012.usd
    ///    time = 333, clipTemplateAssetPath = foo.#.usd   => foo.333.usd
    ///
    /// In the case of subinteger portion of a specifications, USD requires the 
    /// specification to be exact. 
    ///
    /// For instance:
    /// 
    ///    time = 1.15,  clipTemplateAssetPath = foo.#.###.usd => foo.1.150.usd
    ///    time = 1.145, clipTemplateAssetPath = foo.#.##.usd  => foo.1.15.usd
    ///    time = 1.1,   clipTemplateAssetPath = foo.#.##.usd  => foo.1.10.usd
    ///
    /// Note that USD requires that hash groups be adjacent in the string, 
    /// and that there only be one or two such groups.
    bool GetClipTemplateAssetPath(std::string* clipTemplateAssetPath) const;
    /// Set the clipTemplateAssetPath metadata for this prim.
    /// \sa GetClipTemplateAssetPath
    bool SetClipTemplateAssetPath(const std::string& clipTemplateAssetPath);

    /// A double representing the increment value USD will use when
    /// searching for asset paths. For example usage \sa GetClipTemplateAssetPath.
    bool GetClipTemplateStride(double* clipTemplateStride) const;
    /// Set the clipTemplateStride metadata for this prim
    /// \sa GetClipTemplateStride()
    bool SetClipTemplateStride(const double clipTemplateStride);

    /// A double which indicates the start of the range USD will use 
    /// to search for asset paths. This value is inclusive in that range.
    /// For example usage \sa GetClipTemplateAssetPath.
    bool GetClipTemplateStartTime(double* clipTemplateStartTime) const;
    /// Set the clipTemplateStartTime metadata for this prim
    /// \sa GetClipTemplateStartTime
    bool SetClipTemplateStartTime(const double clipTemplateStartTime);

    /// A double which indicates the end of the range USD will use to
    /// to search for asset paths. This value is inclusive in that range.
    /// For example usage \sa GetClipTemplateAssetPath.
    bool GetClipTemplateEndTime(double* clipTemplateEndTime) const;
    /// Set the clipTemplateEndTime metadata for this prim
    /// \sa GetClipTemplateEndTime()
    bool SetClipTemplateEndTime(const double clipTemplateEndTime);

    /// Clear out the following metadata from the current edit target:
    /// 
    /// clipTemplateAssetPath
    /// clipTemplateStride
    /// clipTemplateStartTime
    /// clipTemplateEndTime
    ///
    /// \sa ClearNonTemplateClipMetadata()
    bool ClearTemplateClipMetadata();

    /// Clear out the following metadata from the current edit target:
    ///
    /// clipTimes
    /// clipActive
    /// clipAssetPaths
    ///
    /// \sa ClearTemplateClipMetadata()
    bool ClearNonTemplateClipMetadata();

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
