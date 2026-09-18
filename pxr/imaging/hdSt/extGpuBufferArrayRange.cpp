//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuBufferArrayRange.h"

#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/extGpuBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/diagnostic.h"

#include <algorithm>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

HdStExtGpuBufferArrayRange::HdStExtGpuBufferArrayRange(
    HdStResourceRegistry *resourceRegistry)
    : HdStBufferArrayRange(resourceRegistry)
    , _numElements(0)
    , _version(0)
    , _valid(false)
{
    HD_PERF_COUNTER_INCR(HdStPerfTokens->extGpuBufferAliasCount);
}

HdStExtGpuBufferArrayRange::~HdStExtGpuBufferArrayRange()
{
    HD_PERF_COUNTER_DECR(HdStPerfTokens->extGpuBufferAliasCount);
}

int
HdStExtGpuBufferArrayRange::_FindResource(TfToken const &name) const
{
    for (size_t i = 0; i < _resources.size(); ++i) {
        if (_resources[i].first == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void
HdStExtGpuBufferArrayRange::SetExternalResource(
    TfToken const &name,
    HdStExtGpuBufferDesc const &desc)
{
    const int stride =
        static_cast<int>(HdDataSizeOfTupleType(desc.tupleType));

    auto resource = std::make_shared<HdStBufferResource>(
        HdTokens->primvar,
        desc.tupleType,
        static_cast<int>(desc.byteOffset),
        stride);
    resource->SetAllocation(desc.GetHandle(), desc.numElements * stride);

    _resources.push_back(std::make_pair(name, resource));
    // Hold the buffer for as long as it is bound here. This reference is the
    // only thing keeping it out of the arena's reclaim path.
    _externalBuffers.push_back(desc.externalBuffer);

    _numElements = desc.numElements;
    _valid = true;
}

void
HdStExtGpuBufferArrayRange::ReleaseExternalResources()
{
    _resources.clear();
    // Dropping these is the whole of teardown: no arena call, no Hgi call. The
    // reference count falling is how the arena learns we are done, and it
    // reclaims the buffer later, once the GPU has retired the work that named
    // it.
    _externalBuffers.clear();
    _numElements = 0;
    _valid = false;
    IncrementVersion();
}

bool
HdStExtGpuBufferArrayRange::_UpdateResource(
    size_t index,
    HdStExtGpuBufferDesc const &desc)
{
    HdStBufferResourceSharedPtr const &resource = _resources[index].second;

    // The tuple type and the element offset are fixed at construction, so a
    // change in either cannot be applied here and needs a rebuild.
    if (desc.tupleType != resource->GetTupleType() ||
            static_cast<int>(desc.byteOffset) != resource->GetOffset()) {
        return false;
    }

    const int stride =
        static_cast<int>(HdDataSizeOfTupleType(desc.tupleType));
    resource->SetAllocation(desc.GetHandle(), desc.numElements * stride);
    // Swap the reference along with the handle. Holding the old buffer after
    // rebinding would pin memory nothing reads any more.
    _externalBuffers[index] = desc.externalBuffer;

    _numElements = desc.numElements;
    return true;
}

bool
HdStExtGpuBufferArrayRange::UpdateExternalResources(
    HdBufferSourceSharedPtrVector const &sources)
{
    // Validate everything before mutating anything: a rebind that fails
    // half-way would leave the range bound to a mix of old and new buffers,
    // and the caller's fallback is to rebuild from scratch, which assumes this
    // range was left alone.
    std::vector<HdStExtGpuBufferSource const *> extSources;
    extSources.reserve(sources.size());
    for (HdBufferSourceSharedPtr const &source : sources) {
        HdStExtGpuBufferSource const *extSource =
            HdSt_GetExtGpuBufferSource(source);
        if (!extSource) {
            return false;
        }
        const int index = _FindResource(source->GetName());
        if (index >= 0) {
            HdStBufferResourceSharedPtr const &resource =
                _resources[index].second;
            HdStExtGpuBufferDesc const &desc = extSource->GetDescriptor();
            if (desc.tupleType != resource->GetTupleType() ||
                    static_cast<int>(desc.byteOffset) !=
                        resource->GetOffset()) {
                return false;
            }
        }
        extSources.push_back(extSource);
    }

    for (HdStExtGpuBufferSource const *extSource : extSources) {
        HdStExtGpuBufferDesc const &desc = extSource->GetDescriptor();
        const int index = _FindResource(extSource->GetName());
        if (index >= 0) {
            // Checked above, so this cannot fail now.
            _UpdateResource(static_cast<size_t>(index), desc);
        } else {
            SetExternalResource(extSource->GetName(), desc);
        }
    }

    _valid = !_resources.empty();
    IncrementVersion();
    return true;
}

void
HdStExtGpuBufferArrayRange::GetArenas(
    std::vector<HgiExternalBufferArena *> *arenas) const
{
    if (!arenas) {
        return;
    }
    for (HgiExternalBufferSharedPtr const &buffer : _externalBuffers) {
        if (!buffer) {
            continue;
        }
        HgiExternalBufferArena *arena = buffer->GetArena();
        if (arena && std::find(arenas->begin(), arenas->end(), arena) ==
                arenas->end()) {
            arenas->push_back(arena);
        }
    }
}

bool
HdStExtGpuBufferArrayRange::IsValid() const
{
    return _valid;
}

bool
HdStExtGpuBufferArrayRange::IsAssigned() const
{
    // "Assigned" asks whether the range has storage behind it. It does -- the
    // producer's -- from the moment a resource is bound, which is the same
    // condition as validity here. There is no separate unassigned state,
    // because there is no allocation step to be waiting on.
    return _valid;
}

bool
HdStExtGpuBufferArrayRange::IsImmutable() const
{
    // Never immutable. Immutability is a promise Storm makes about buffers it
    // owns, so that it may share and cache them; the contents here belong to a
    // producer that may rewrite them between frames, which is precisely the
    // case this whole path exists to serve.
    return false;
}

bool
HdStExtGpuBufferArrayRange::RequiresStaging() const
{
    return false;
}

bool
HdStExtGpuBufferArrayRange::Resize(int /*numElements*/)
{
    // Unreachable by construction: an external source is never aggregated into
    // a memory manager's buffer array, which is what resizes ranges. Say so
    // rather than quietly accepting it -- a caller who got here has the range
    // in a place it does not belong, and the element count comes from the
    // producer through UpdateExternalResources regardless.
    TF_CODING_ERROR("Resize on an external GPU buffer array range: it owns no "
                    "storage to resize. The element count follows the "
                    "producer's buffer.");
    return false;
}

void
HdStExtGpuBufferArrayRange::CopyData(
    HdBufferSourceSharedPtr const & /*bufferSource*/)
{
    // Also unreachable by construction, and worth shouting about: silently
    // doing nothing here would mean a source the caller believed it had
    // written is quietly absent from the draw.
    TF_CODING_ERROR("CopyData on an external GPU buffer array range: it binds "
                    "the producer's buffer and has nowhere to copy into. An "
                    "external source must not be aggregated with ordinary "
                    "sources.");
}

VtValue
HdStExtGpuBufferArrayRange::ReadData(TfToken const & /*name*/) const
{
    TF_CODING_ERROR("ReadData on an external GPU buffer array range: reading "
                    "back a producer's buffer is not supported.");
    return VtValue();
}

int
HdStExtGpuBufferArrayRange::GetElementOffset() const
{
    return 0;
}

int
HdStExtGpuBufferArrayRange::GetByteOffset(
    TfToken const &resourceName) const
{
    const int index = _FindResource(resourceName);
    return index >= 0 ? _resources[index].second->GetOffset() : 0;
}

size_t
HdStExtGpuBufferArrayRange::GetNumElements() const
{
    return _numElements;
}

size_t
HdStExtGpuBufferArrayRange::GetVersion() const
{
    return _version.load();
}

void
HdStExtGpuBufferArrayRange::IncrementVersion()
{
    _version.fetch_add(1);
}

size_t
HdStExtGpuBufferArrayRange::GetMaxNumElements() const
{
    return _numElements;
}

HdBufferArrayUsageHint
HdStExtGpuBufferArrayRange::GetUsageHint() const
{
    return HdBufferArrayUsageHintBitsVertex |
           HdBufferArrayUsageHintBitsStorage;
}

void
HdStExtGpuBufferArrayRange::SetBufferArray(HdBufferArray * /*bufferArray*/)
{
    // An alias range belongs to no buffer array; being handed one means it
    // reached a memory manager, which is exactly what the aggregation path is
    // meant to prevent.
    TF_CODING_ERROR("SetBufferArray on an external GPU buffer array range: it "
                    "is not part of a buffer array.");
}

void
HdStExtGpuBufferArrayRange::DebugDump(std::ostream &out) const
{
    out << "HdStExtGpuBufferArrayRange:"
        << " valid=" << _valid
        << " numElements=" << _numElements
        << " resources=" << _resources.size()
        << "\n";
}

HdStBufferResourceSharedPtr
HdStExtGpuBufferArrayRange::GetResource() const
{
    if (_resources.size() == 1) {
        return _resources.front().second;
    }
    if (!_resources.empty()) {
        TF_CODING_ERROR("GetResource() called on an external GPU buffer array "
                        "range with several resources; use GetResource(name).");
    }
    return nullptr;
}

HdStBufferResourceSharedPtr
HdStExtGpuBufferArrayRange::GetResource(TfToken const &name)
{
    const int index = _FindResource(name);
    return index >= 0 ? _resources[index].second : nullptr;
}

HdStBufferResourceNamedList const &
HdStExtGpuBufferArrayRange::GetResources() const
{
    return _resources;
}

const void *
HdStExtGpuBufferArrayRange::_GetAggregation() const
{
    // Two alias ranges aggregate when they bind the same buffer. The first
    // buffer stands for the set: a producer builds its ranges with the same
    // ordered layout (positions, normals, uv, ...) every time, so agreeing on
    // the first implies agreeing on all of them.
    //
    // The buffer's address is the key, which is sound because this range holds
    // a strong reference -- the object cannot be destroyed and another land at
    // the same address while it is bound here. Reading the native handle
    // instead would be wrong: a producer that recycles an allocation hands out
    // the same handle for what is, to Storm, a different buffer.
    if (_externalBuffers.empty() || !_externalBuffers.front()) {
        // An empty range is its own aggregation island.
        return this;
    }

    HgiExternalBuffer const *buffer = _externalBuffers.front().get();
    TF_DEBUG(HDST_DRAW).Msg(
        "[ExternalGpuBAR] aggregation key: buffer=%p, resources=%zu, "
        "first=%s\n",
        static_cast<const void *>(buffer),
        _resources.size(),
        _resources.empty() ? "<none>" : _resources.front().first.GetText());

    return buffer;
}

PXR_NAMESPACE_CLOSE_SCOPE
