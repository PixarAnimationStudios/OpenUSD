//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_ERRORS_H
#define PXR_USD_PCP_ERRORS_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \enum PcpErrorType
///
/// Enum to indicate the type represented by a Pcp error.
///
enum PcpErrorType {
    PcpErrorType_ArcCycle,
    PcpErrorType_ArcPermissionDenied,
    PcpErrorType_ArcToProhibitedChild,
    PcpErrorType_IndexCapacityExceeded,
    PcpErrorType_ArcCapacityExceeded,
    PcpErrorType_ArcNamespaceDepthCapacityExceeded,
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
    PcpErrorType_InvalidAuthoredRelocation,
    PcpErrorType_InvalidConflictingRelocation,
    PcpErrorType_InvalidSameTargetRelocations,
    PcpErrorType_OpinionAtRelocationSource,
    PcpErrorType_PrimPermissionDenied,
    PcpErrorType_PropertyPermissionDenied,
    PcpErrorType_SublayerCycle,
    PcpErrorType_TargetPermissionDenied,
    PcpErrorType_UnresolvedPrimPath,
    PcpErrorType_VariableExpressionError
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
    const PcpErrorType errorType;

    /// The site of the composed prim or property being computed when
    /// the error was encountered.  (Note that some error types
    /// contain an additional site to capture more specific information
    /// about the site of the error.)
    PcpSite rootSite;

protected:
    /// Constructor.
    explicit PcpErrorBase(PcpErrorType errorType);
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
    PCP_API ~PcpErrorArcCycle() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
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
    PCP_API ~PcpErrorArcPermissionDenied() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
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

// Forward declarations:
class PcpErrorArcToProhibitedChild;
typedef std::shared_ptr<PcpErrorArcToProhibitedChild> 
    PcpErrorArcToProhibitedChildPtr;

/// \class PcpErrorArcToProhibitedChild
///
/// Arcs that were not made between PcpNodes because the target is a prohibited
/// child prim of its parent due to relocations.
///
class PcpErrorArcToProhibitedChild : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorArcToProhibitedChildPtr New();
    /// Destructor.
    PCP_API ~PcpErrorArcToProhibitedChild() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
    /// The site where the invalid arc was expressed.
    PcpSite site;
    /// The target site of the invalid arc which is a prohibited child.
    PcpSite targetSite;
    /// The site of the node under targetSite that is a relocation source in its
    /// layer stack.
    PcpSite relocationSourceSite;
    /// The type of arc.
    PcpArcType arcType;
    
private:
    /// Constructor is private. Use New() instead.
    PcpErrorArcToProhibitedChild();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorCapacityExceeded;
typedef std::shared_ptr<PcpErrorCapacityExceeded> PcpErrorCapacityExceededPtr;

/// \class PcpErrorCapacityExceeded
///
/// Exceeded the capacity for composition arcs at a single site.
///
class PcpErrorCapacityExceeded : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorCapacityExceededPtr New(PcpErrorType errorType);
    /// Destructor.
    PCP_API ~PcpErrorCapacityExceeded() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
private:
    /// Constructor is private. Use New() instead.
    PcpErrorCapacityExceeded(PcpErrorType errorType);
};

///////////////////////////////////////////////////////////////////////////////

class PcpErrorInconsistentPropertyBase : public PcpErrorBase {
public:
    /// Destructor.
    PCP_API ~PcpErrorInconsistentPropertyBase() override;
    
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
    PcpErrorInconsistentPropertyBase(PcpErrorType errorType);
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
    PCP_API ~PcpErrorInconsistentPropertyType() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorInconsistentAttributeType() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorInconsistentAttributeVariability() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorInvalidPrimPath() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

    /// The site where the invalid arc was expressed.
    PcpSite site;

    /// The target prim path of the arc that is invalid.
    SdfPath primPath;

    /// The source layer of the spec that caused this arc to be introduced.
    /// This may be a sublayer of the site.
    SdfLayerHandle sourceLayer;

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
    PCP_API ~PcpErrorInvalidAssetPathBase() override;
    
    /// The site where the invalid arc was expressed.
    PcpSite site;

    /// The target prim path of the arc.
    SdfPath targetPath;

    /// The target asset path of the arc as authored.
    std::string assetPath;

    /// The resolved target asset path of the arc.
    std::string resolvedAssetPath;

    /// The source layer of the spec that caused this arc to be introduced.
    /// This may be a sublayer of the site.
    SdfLayerHandle sourceLayer;

    PcpArcType arcType;

    /// Additional provided error information.
    std::string messages;

protected:
    /// Constructor.
    PcpErrorInvalidAssetPathBase(PcpErrorType errorType);
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
    PCP_API ~PcpErrorInvalidAssetPath() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorMutedAssetPath() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorTargetPathBase() override;

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
    PcpErrorTargetPathBase(PcpErrorType errorType);
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
    PCP_API ~PcpErrorInvalidInstanceTargetPath() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorInvalidExternalTargetPath() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
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
    PCP_API ~PcpErrorInvalidTargetPath() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorInvalidSublayerOffset() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
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
/// References or payloads that use invalid layer offsets.
///
class PcpErrorInvalidReferenceOffset : public PcpErrorBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidReferenceOffsetPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidReferenceOffset() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
    /// The source layer of the spec that caused this arc to be introduced.
    SdfLayerHandle sourceLayer;

    /// The source path of the spec that caused this arc to be introduced.
    SdfPath sourcePath;

    /// Target asset path of the arc.
    std::string assetPath;

    /// Target prim path of the arc.
    SdfPath targetPath;

    /// The invalid layer offset expressed on the arc.
    SdfLayerOffset offset;

    PcpArcType arcType;

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
    PCP_API ~PcpErrorInvalidSublayerOwnership() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorInvalidSublayerPath() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
    SdfLayerHandle layer;
    std::string sublayerPath;
    std::string messages;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorInvalidSublayerPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorRelocationBase;
typedef std::shared_ptr<PcpErrorRelocationBase>
    PcpErrorRelocationBasePtr;

/// \class PcpErrorRelocationBase
///
/// Base class for composition errors related to relocates.
///
class PcpErrorRelocationBase : public PcpErrorBase {
public:
    /// Destructor.
    PCP_API ~PcpErrorRelocationBase() override;

protected:
    PcpErrorRelocationBase(PcpErrorType errorType);
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidAuthoredRelocation;
typedef std::shared_ptr<PcpErrorInvalidAuthoredRelocation>
    PcpErrorInvalidAuthoredRelocationPtr;

/// \class PcpErrorInvalidAuthoredRelocation
///
/// Invalid authored relocation found in a relocates field.
///
class PcpErrorInvalidAuthoredRelocation : public PcpErrorRelocationBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidAuthoredRelocationPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidAuthoredRelocation() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
    /// The source path of the invalid relocation.
    SdfPath sourcePath;
    /// The target path of the invalid relocation.
    SdfPath targetPath;
    /// The layer containing the authored relocates.
    SdfLayerHandle layer;
    /// The path to the prim where the relocates is authored.
    SdfPath owningPath;

    /// Additional messages about the error.
    std::string messages;

private:
    PcpErrorInvalidAuthoredRelocation();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidConflictingRelocation;
typedef std::shared_ptr<PcpErrorInvalidConflictingRelocation>
    PcpErrorInvalidConflictingRelocationPtr;

/// \class PcpErrorInvalidConflictingRelocation
///
/// Relocation conflicts with another relocation in the layer stack.
///
class PcpErrorInvalidConflictingRelocation : public PcpErrorRelocationBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidConflictingRelocationPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidConflictingRelocation() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

    /// The source path of the invalid relocation.
    SdfPath sourcePath;
    /// The target path of the invalid relocation.
    SdfPath targetPath;
    /// The layer containing the authored relocates.
    SdfLayerHandle layer;
    /// The path to the prim where the relocates is authored.
    SdfPath owningPath;

    /// The source path of the relocation this conflicts with.
    SdfPath conflictSourcePath;
    /// The target path of the relocation this conflicts with.
    SdfPath conflictTargetPath;
    /// The layer containing the authored relocation this conflicts with.
    SdfLayerHandle conflictLayer;
    /// The path to the prim where the relocation this conflicts with is authored.
    SdfPath conflictOwningPath;

    /// Enumeration of reasons a relocate can be in conflict with another 
    /// relocate.
    enum class ConflictReason {
        TargetIsConflictSource,
        SourceIsConflictTarget,
        TargetIsConflictSourceDescendant,
        SourceIsConflictSourceDescendant
    };

    /// The reason the relocate is a conflict.
    ConflictReason conflictReason;

private:
    PcpErrorInvalidConflictingRelocation();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorInvalidSameTargetRelocations;
typedef std::shared_ptr<PcpErrorInvalidSameTargetRelocations>
    PcpErrorInvalidSameTargetRelocationsPtr;

/// \class PcpErrorInvalidSameTargetRelocations
///
/// Multiple relocations in the layer stack have the same target.
///
class PcpErrorInvalidSameTargetRelocations : public PcpErrorRelocationBase {
public:
    /// Returns a new error object.
    static PcpErrorInvalidSameTargetRelocationsPtr New();
    /// Destructor.
    PCP_API ~PcpErrorInvalidSameTargetRelocations() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

    /// The target path of the multiple invalid relocations.
    SdfPath targetPath;

    /// Info about each relocate source that has the same target path.
    struct RelocationSource {
        /// The source path of the invalid relocation.
        SdfPath sourcePath;
        /// The layer containing the authored relocates.
        SdfLayerHandle layer;
        /// The path to the prim where the relocates is authored.
        SdfPath owningPath;
    };

    /// The sources of all relocates that relocate to the target path.
    std::vector<RelocationSource> sources;

private:
    PcpErrorInvalidSameTargetRelocations();
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
    PCP_API ~PcpErrorOpinionAtRelocationSource() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
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
    PCP_API ~PcpErrorPrimPermissionDenied() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
    /// The site where the invalid arc was expressed.
    PcpSite site;
    /// The private, invalid target of the arc.
    PcpSite privateSite;

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
    PCP_API ~PcpErrorPropertyPermissionDenied() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
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
    PCP_API ~PcpErrorSublayerCycle() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
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
    PCP_API ~PcpErrorTargetPermissionDenied() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;

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
    PCP_API ~PcpErrorUnresolvedPrimPath() override;
    /// Converts error to string message.
    PCP_API std::string ToString() const override;
    
    /// The site where the invalid arc was expressed.
    PcpSite site;

    /// The source layer of the spec that caused this arc to be introduced.
    /// This may be a sublayer of the site.
    SdfLayerHandle sourceLayer;
    
    /// The target layer of the arc.
    SdfLayerHandle targetLayer;

    /// The prim path that cannot be resolved on the target layer stack.
    SdfPath unresolvedPath;

    PcpArcType arcType;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorUnresolvedPrimPath();
};

///////////////////////////////////////////////////////////////////////////////

// Forward declarations:
class PcpErrorVariableExpressionError;
typedef std::shared_ptr<PcpErrorVariableExpressionError>
    PcpErrorVariableExpressionErrorPtr;

/// \class PcpErrorVariableExpressionError
///
/// Error when evaluating a variable expression.
///
class PcpErrorVariableExpressionError : public PcpErrorBase {
public:
    static PcpErrorVariableExpressionErrorPtr New();

    PCP_API ~PcpErrorVariableExpressionError() override;

    PCP_API std::string ToString() const override;

    /// The expression that was evaluated.
    std::string expression;

    /// The error generated during evaluation.
    std::string expressionError;

    /// The context where the expression was authored, e.g.
    /// "sublayer", "reference", etc.
    std::string context;

    /// The source layer where the expression was authored.
    SdfLayerHandle sourceLayer;

    /// The source path where the expression was authored. This
    /// may be the absolute root path.
    SdfPath sourcePath;

private:
    /// Constructor is private. Use New() instead.
    PcpErrorVariableExpressionError();
};

///////////////////////////////////////////////////////////////////////////////

/// Raise the given errors as runtime errors.
PCP_API
void PcpRaiseErrors(const PcpErrorVector &errors);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_ERRORS_H
