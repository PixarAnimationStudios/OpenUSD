//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_INSTANCER_H
#define PXR_IMAGING_PLUGIN_LOFI_INSTANCER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include "pxr/imaging/plugin/LoFi/vertexArray.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/drawItem.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class LoFiInstancer
///
class LoFiInstancer final : public HdInstancer
{
public:
    /// Constructor.
    ///   \param delegate The scene delegate backing this instancer's data.
    ///   \param id The unique id of this instancer.
    ///   \param parentInstancerId The unique id of the parent instancer,
    ///                            or an empty id if not applicable.
    LoFiInstancer(HdSceneDelegate* delegate, SdfPath const& id);

    /// Destructor.
    ~LoFiInstancer();

    /// Computes all instance transforms for the provided prototype id,
    /// taking into account the scene delegate's instancerTransform and the
    /// instance primvars "instanceTransform", "translate", "rotate", "scale".
    /// Computes and flattens nested transforms, if necessary.
    ///   \param prototypeId The prototype to compute transforms for.
    ///   \return One transform per instance, to apply when drawing.
    VtMatrix4dArray ComputeInstanceTransforms(SdfPath const &prototypeId);

    bool HaveColors() const {return _colors.size()>0;};
    const VtArray<GfVec3f>& GetColors() const {return _colors;};

private:
    // Checks the change tracker to determine whether instance primvars are
    // dirty, and if so pulls them. Since primvars can only be pulled once,
    // and are cached, this function is not re-entrant. However, this function
    // is called by ComputeInstanceTransforms, which is called (potentially)
    // by LoFiMesh::Sync(), which is dispatched in parallel, so it needs
    // to be guarded by _instanceLock.
    //
    // Pulled primvars are cached in _primvarMap.
    void _GetPrimvarDatas(const TfToken& primvarName, const VtValue& value,
        LoFiAttributeChannel& channel, uint32_t& numElements);
    void _SyncPrimvars();
    //void _SyncColors();
    

    // Mutex guard for _SyncPrimvars().
    std::mutex _instanceLock;

    // Map of the latest primvar data for this instancer, keyed by
    // primvar name. Primvar values are VtValue, an any-type; they are
    // interpreted at consumption time (here, in ComputeInstanceTransforms).
    TfHashMap<TfToken,
              LoFiVertexBuffer*,
              TfToken::HashFunctor> _primvarMap;

    VtArray<GfVec3f>        _positions;
    VtArray<GfVec4f>        _rotations;
    VtArray<GfVec3f>        _scales;
    VtArray<GfVec3f>        _colors;
    VtArray<GfMatrix4d>     _xforms;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_INSTANCER_H
