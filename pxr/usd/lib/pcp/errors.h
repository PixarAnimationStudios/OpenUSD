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
#ifndef PCP_ERRORS_H
#define PCP_ERRORS_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/enum.h"

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Forward declaration:
class PcpCache;

/// \enum PcpErrorType
///
/// Enum to indicate the type represented by a Pcp error.
///
enum PcpErrorType {
    PcpErrorType_ArcCycle,
    PcpErrorType_ArcPermissionDenied,
    PcpErrorType_InconsistentPropertyType,
    PcpErrorType_InconsistentAttributeType,
    PcpErrorType_InconsistentAttributeVariability,
    PcpErrorType_InternalAssetPath,
    PcpErrorType_InvalidPrimPath,
    PcpErrorType_InvalidAssetPath,
    PcpErrorType_InvalidInstanceTargetPath,
    PcpErrorType_InvalidExternalTargetPath,
    PcpErrorType_InvalidTargetPath,
    PcpErrorType_InvalidReferenceOffset,
    PcpErrorType_InvalidSublayerOffset,
    PcpErrorType_InvalidSublayerOwnership,
    PcpErrorType_InvalidSublayerPath,
    PcpErrorType_InvalidVariantSelection,
    PcpErrorType_MutedAssetPath,
    PcpErrorType_OpinionAtRelocationSource,
    PcpErrorType_PrimPermissionDenied,
    PcpErrorType_PropertyPermissionDenied,
    PcpErrorType_SublayerCycle,
    PcpErrorType_TargetPermissionDenied,
    PcpErrorType_UnresolvedPrimPath
};

// Forward declarations:
class PcpErrorBase;
typedef std::shared_ptr<PcpErrorBase> PcpErrorBasePtr;
typedef std::vector<PcpErrorBasePtr> PcpErrorVector;

/// \class PcpErrorBase
///
/// Base class for all error types.
///
class PcpErrorBase {
public:
    /// Destructor.
    PCP_API virtual ~PcpErrorBase();
    /// Converts error to string message.
    virtual std::string ToString() const = 0;

    /// The error code.
    const TfEnum errorType;

    /// The site of the composed prim or property being computed when
    /// the error was encountered.  (Note that some error types
    /// contain an additional site to capture more specific information
    /// about the site of the error.)
    PcpSiteStr rootSite;
   
protected:
    /// Constructor.
    PcpErrorBase(TfEnum errorType);
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorArcCycle;
typedef std::shared_ptr<PcpErrorArcCycle> PcpErrorArcCyclePtr;

/// \class PcpErrorArcCycle
///
/// Arcs between PcpNodes that form a cycle.
///
class PcpErrorArcCycle : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorArcCyclePtr New();
    /// Destructor.
    PCP_API ~PcpErrorArcCycle();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    PcpSiteTracker cycle;
    
private:
    /// Constructor is private. Use New() instead.
    PcpErrorArcCycle();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorArcPermissionDenied;
typedef std::shared_ptr<PcpErrorArcPermissionDenied> 
    PcpErrorArcPermissionDeniedPtr;

/// \class PcpErrorArcPermissionDenied
///
/// Arcs that were not made between PcpNodes because of permission
/// restrictions.
///
class PcpErrorArcPermissionDenied : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorArcPermissionDeniedPtr New();
    /// Destructor.
    PCP_API ~PcpErrorArcPermissionDenied();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    /// The site where the invalid arc was expressed.
    PcpSite site;
    /// The private, invalid target of the arc.
    PcpSite privateSite;
    /// The type of arc.
    PcpArcType arcType;
    
private:
    /// Constructor is private. Use New() instead.
    PcpErrorArcPermissionDenied();
};

///////////////////////////////////////////////////////////////////////////////

class PcpErrorInconsistentPropertyBase : public PcpErrorBase {
public:
    /// Destructor.
    PCP_API virtual ~PcpErrorInconsistentPropertyBase();
    
    /// The identifier of the layer with the defining property spec.
    std::string definingLayerIdentifier;
    /// The path of the defining property spec.
    SdfPath definingSpecPath;

    /// The identifier of the layer with the conflicting property spec.
    std::string conflictingLayerIdentifier;
    /// The path of the conflicting property spec.
    SdfPath conflictingSpecPath;
    
protected:
    /// Constructor.
    PcpErrorInconsistentPropertyBase(TfEnum errorType);
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInconsistentPropertyType;
typedef std::shared_ptr<PcpErrorInconsistentPropertyType> 
    PcpErrorInconsistentPropertyTypePtr;

/// \class PcpErrorInconsistentPropertyType
///
/// Properties that have specs with conflicting definitions.
///
class PcpErrorInconsistentPropertyType : 
    public PcpErrorInconsistentPropertyBase {
public:
    /// Returns a new error object.
    static PcpErrorInconsistentPropertyTypePtr New();
    /// Destructor.
    PCP_API ~PcpErrorInconsistentPropertyType();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

    /// The type of the defining spec.
    SdfSpecType definingSpecType;
    /// The type of the conflicting spec.
    SdfSpecType conflictingSpecType;
    
private:
    /// Constructor is private. Use New() instead.
    PcpErrorInconsistentPropertyType();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInconsistentAttributeType;
typedef std::shared_ptr<PcpErrorInconsistentAttributeType> 
    PcpErrorInconsistentAttributeTypePtr;

/// \class PcpErrorInconsistentAttributeType
///
/// Attributes that have specs with conflicting definitions.
///
class PcpErrorInconsistentAttributeType : 
    public PcpErrorInconsistentPropertyBase {
public:
    /// Returns a new error object.
    static PcpErrorInconsistentAttributeTypePtr New();
    /// Destructor.
    PCP_API ~PcpErrorInconsistentAttributeType();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

    /// The value type from the defining spec.
    TfToken definingValueType;
    /// The value type from the conflicting spec.
    TfToken conflictingValueType;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInconsistentAttributeType();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInconsistentAttributeVariability;
typedef std::shared_ptr<PcpErrorInconsistentAttributeVariability> 
    PcpErrorInconsistentAttributeVariabilityPtr;

/// \class PcpErrorInconsistentAttributeVariability
///
/// Attributes that have specs with conflicting variability.
///
class PcpErrorInconsistentAttributeVariability : 
    public PcpErrorInconsistentPropertyBase {
public:
    /// Returns a new error object.
    static PcpErrorInconsistentAttributeVariabilityPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInconsistentAttributeVariability();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

    /// The variability of the defining spec.
    SdfVariability definingVariability;
    /// The variability of the conflicting spec.
    SdfVariability conflictingVariability;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInconsistentAttributeVariability();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInternalAssetPath;
typedef std::shared_ptr<PcpErrorInternalAssetPath>
    PcpErrorInternalAssetPathPtr;

/// \class PcpErrorInternalAssetPath
///
/// Error about an arc that is prohibited due to being internal to an asset.
///
class PcpErrorInternalAssetPath : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorInternalAssetPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInternalAssetPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    /// The site where the invalid arc was expressed.
    PcpSite site;
    SdfPath targetPath;
    std::string assetPath;
    std::string resolvedAssetPath;
    PcpArcType arcType;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInternalAssetPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidPrimPath;
typedef std::shared_ptr<PcpErrorInvalidPrimPath>
    PcpErrorInvalidPrimPathPtr;

/// \class PcpErrorInvalidPrimPath
///
/// Invalid prim paths used by references or payloads.
///
class PcpErrorInvalidPrimPath : public PcpErrorBase {
public:        
    /// Returns a new error object.
    static PcpErrorInvalidPrimPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidPrimPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

    /// The site where the invalid arc was expressed.
    PcpSite site;
    SdfPath primPath;
    PcpArcType arcType;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidPrimPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidAssetPathBase;
typedef std::shared_ptr<PcpErrorInvalidAssetPathBase>
    PcpErrorInvalidAssetPathBasePtr;

class PcpErrorInvalidAssetPathBase : public PcpErrorBase {
public:
    /// Destructor.
    PCP_API ~PcpErrorInvalidAssetPathBase();
    
    /// The site where the invalid arc was expressed.
    PcpSite site;
    SdfPath targetPath;
    std::string assetPath;
    std::string resolvedAssetPath;
    PcpArcType arcType;
    SdfLayerHandle layer;
    std::string messages;

protected:
    /// Constructor.
    PcpErrorInvalidAssetPathBase(TfEnum errorType);
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidAssetPath;
typedef std::shared_ptr<PcpErrorInvalidAssetPath>
    PcpErrorInvalidAssetPathPtr;

/// \class PcpErrorInvalidAssetPath
///
/// Invalid asset paths used by references or payloads.
///
class PcpErrorInvalidAssetPath : public PcpErrorInvalidAssetPathBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidAssetPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidAssetPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidAssetPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorMutedAssetPath;
typedef std::shared_ptr<PcpErrorMutedAssetPath>
    PcpErrorMutedAssetPathPtr;

/// \class PcpErrorMutedAssetPath
///
/// Muted asset paths used by references or payloads.
///
class PcpErrorMutedAssetPath : public PcpErrorInvalidAssetPathBase {
public:
    /// Returns a new error object.
    static PcpErrorMutedAssetPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorMutedAssetPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorMutedAssetPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorTargetPathBase;
typedef std::shared_ptr<PcpErrorTargetPathBase>
    PcpErrorTargetPathBasePtr;

/// \class PcpErrorTargetPathBase
///
/// Base class for composition errors related to target or connection paths.
///
class PcpErrorTargetPathBase : public PcpErrorBase {
public:
    /// Destructor.
    PCP_API ~PcpErrorTargetPathBase();

    /// The invalid target or connection path that was authored.
    SdfPath targetPath;
    /// The path to the property where the target was authored.
    SdfPath owningPath;
    /// The spec type of the property where the target was authored.
    SdfSpecType ownerSpecType;
    /// The layer containing the property where the target was authored.
    SdfLayerHandle layer;

    /// The target or connection path in the composed scene.
    /// If this path could not be translated to the composed scene
    /// (e.g., in the case of an invalid external target path),
    /// this path will be empty.
    SdfPath composedTargetPath;

protected:
    PcpErrorTargetPathBase(TfEnum errorType);
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidInstanceTargetPath;
typedef std::shared_ptr<PcpErrorInvalidInstanceTargetPath>
    PcpErrorInvalidInstanceTargetPathPtr;

/// \class PcpErrorInvalidInstanceTargetPath
///
/// Invalid target or connection path authored in an inherited
/// class that points to an instance of that class.
///
class PcpErrorInvalidInstanceTargetPath : public PcpErrorTargetPathBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidInstanceTargetPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidInstanceTargetPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidInstanceTargetPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidExternalTargetPath;
typedef std::shared_ptr<PcpErrorInvalidExternalTargetPath>
    PcpErrorInvalidExternalTargetPathPtr;

/// \class PcpErrorInvalidExternalTargetPath
///
/// Invalid target or connection path in some scope that points to
/// an object outside of that scope.
///
class PcpErrorInvalidExternalTargetPath : public PcpErrorTargetPathBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidExternalTargetPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidExternalTargetPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    PcpArcType ownerArcType;
    SdfPath ownerIntroPath;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidExternalTargetPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidTargetPath;
typedef std::shared_ptr<PcpErrorInvalidTargetPath>
    PcpErrorInvalidTargetPathPtr;

/// \class PcpErrorInvalidTargetPath
/// 
/// Invalid target or connection path.
///
class PcpErrorInvalidTargetPath : public PcpErrorTargetPathBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidTargetPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidTargetPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidTargetPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidSublayerOffset;
typedef std::shared_ptr<PcpErrorInvalidSublayerOffset>
    PcpErrorInvalidSublayerOffsetPtr;

/// \class PcpErrorInvalidSublayerOffset
///
/// Sublayers that use invalid layer offsets.
///
class PcpErrorInvalidSublayerOffset : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidSublayerOffsetPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidSublayerOffset();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    SdfLayerHandle layer;
    SdfLayerHandle sublayer;
    SdfLayerOffset offset;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidSublayerOffset();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidReferenceOffset;
typedef std::shared_ptr<PcpErrorInvalidReferenceOffset>
    PcpErrorInvalidReferenceOffsetPtr;

/// \class PcpErrorInvalidReferenceOffset
///
/// Sublayers that use invalid layer offsets.
///
class PcpErrorInvalidReferenceOffset : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidReferenceOffsetPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidReferenceOffset();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    SdfLayerHandle layer;
    SdfPath sourcePath;
    std::string assetPath;
    SdfPath targetPath;
    SdfLayerOffset offset;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidReferenceOffset();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidSublayerOwnership;
typedef std::shared_ptr<PcpErrorInvalidSublayerOwnership>
    PcpErrorInvalidSublayerOwnershipPtr;

/// \class PcpErrorInvalidSublayerOwnership
///
/// Sibling layers that have the same owner.
///
class PcpErrorInvalidSublayerOwnership : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidSublayerOwnershipPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidSublayerOwnership();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

    std::string owner;
    SdfLayerHandle layer;
    SdfLayerHandleVector sublayers;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidSublayerOwnership();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidSublayerPath;
typedef std::shared_ptr<PcpErrorInvalidSublayerPath>
    PcpErrorInvalidSublayerPathPtr;

/// \class PcpErrorInvalidSublayerPath
///
/// Asset paths that could not be both resolved and loaded.
///
class PcpErrorInvalidSublayerPath : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidSublayerPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidSublayerPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    SdfLayerHandle layer;
    std::string sublayerPath;
    std::string messages;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidSublayerPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidVariantSelection;
typedef std::shared_ptr<PcpErrorInvalidVariantSelection>
    PcpErrorInvalidVariantSelectionPtr;

/// \class PcpErrorInvalidVariantSelection
///
/// Invalid variant selections.
///
class PcpErrorInvalidVariantSelection : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidVariantSelectionPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidVariantSelection();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    std::string siteAssetPath;
    SdfPath sitePath;
    std::string vset, vsel;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidVariantSelection();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorOpinionAtRelocationSource;
typedef std::shared_ptr<PcpErrorOpinionAtRelocationSource>
    PcpErrorOpinionAtRelocationSourcePtr;

/// \class PcpErrorOpinionAtRelocationSource
///
/// Opinions were found at a relocation source path.
///
class PcpErrorOpinionAtRelocationSource : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorOpinionAtRelocationSourcePtr New();
    /// Destructor.
    PCP_API ~PcpErrorOpinionAtRelocationSource();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    SdfLayerHandle layer;
    SdfPath path;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorOpinionAtRelocationSource();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorPrimPermissionDenied;
typedef std::shared_ptr<PcpErrorPrimPermissionDenied>
    PcpErrorPrimPermissionDeniedPtr;

/// \class PcpErrorPrimPermissionDenied
///
/// Layers with illegal opinions about private prims.
///
class PcpErrorPrimPermissionDenied : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorPrimPermissionDeniedPtr New();
    /// Destructor.
    PCP_API ~PcpErrorPrimPermissionDenied();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    /// The site where the invalid arc was expressed.
    PcpSiteStr site;
    /// The private, invalid target of the arc.
    PcpSiteStr privateSite;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorPrimPermissionDenied();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorPropertyPermissionDenied;
typedef std::shared_ptr<PcpErrorPropertyPermissionDenied>
    PcpErrorPropertyPermissionDeniedPtr;

/// \class PcpErrorPropertyPermissionDenied
///
/// Layers with illegal opinions about private properties.
///
class PcpErrorPropertyPermissionDenied : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorPropertyPermissionDeniedPtr New();
    /// Destructor.
    PCP_API ~PcpErrorPropertyPermissionDenied();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    SdfPath propPath;
    SdfSpecType propType;
    std::string layerPath;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorPropertyPermissionDenied();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorSublayerCycle;
typedef std::shared_ptr<PcpErrorSublayerCycle> PcpErrorSublayerCyclePtr;

/// \class PcpErrorSublayerCycle
///
/// Layers that recursively sublayer themselves.
///
class PcpErrorSublayerCycle : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorSublayerCyclePtr New();
    /// Destructor.
    PCP_API ~PcpErrorSublayerCycle();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    SdfLayerHandle layer;
    SdfLayerHandle sublayer;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorSublayerCycle();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorTargetPermissionDenied;
typedef std::shared_ptr<PcpErrorTargetPermissionDenied>
    PcpErrorTargetPermissionDeniedPtr;

/// \class PcpErrorTargetPermissionDenied
///
/// Paths with illegal opinions about private targets.
///
class PcpErrorTargetPermissionDenied : public PcpErrorTargetPathBase {
public:
    /// Returns a new error object.
    static PcpErrorTargetPermissionDeniedPtr New();
    /// Destructor.
    PCP_API ~PcpErrorTargetPermissionDenied();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorTargetPermissionDenied();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorUnresolvedPrimPath;
typedef std::shared_ptr<PcpErrorUnresolvedPrimPath>
    PcpErrorUnresolvedPrimPathPtr;

/// \class PcpErrorUnresolvedPrimPath
///
/// Asset paths that could not be both resolved and loaded.
///
class PcpErrorUnresolvedPrimPath : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorUnresolvedPrimPathPtr New();
    /// Destructor.
    PCP_API ~PcpErrorUnresolvedPrimPath();
    /// Converts error to string message.
    PCP_API virtual std::string ToString() const;
    
    /// The site where the invalid arc was expressed.
    PcpSiteStr site;
    SdfPath unresolvedPath;
    PcpArcType arcType;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorUnresolvedPrimPath();
};

/// Raise the given errors as runtime errors.
PCP_API
void PcpRaiseErrors(const PcpErrorVector &errors);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_ERRORS_H
