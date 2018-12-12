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
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/strategyBase.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////
// Null aggregation strategy

class Hd_NullStrategy final : public HdAggregationStrategy
{
public:
    class _BufferArray : public HdBufferArray
    {
    public:
        _BufferArray(TfToken const &role,
                     HdBufferSpecVector const &bufferSpecs,
                     HdBufferArrayUsageHint usageHint)
            : HdBufferArray(role,
                            TfToken("perfToken"),
                            usageHint),
            _bufferSpecs(bufferSpecs)
        {}

        virtual ~_BufferArray() {}

        virtual bool GarbageCollect() { return true; }

        virtual void DebugDump(std::ostream &out) const {}

        bool Resize(int numElements) { return true; }

        virtual void
        Reallocate(std::vector<HdBufferArrayRangeSharedPtr> const &,
                   HdBufferArraySharedPtr const &) {
        }

        virtual size_t GetMaxNumElements() const {
            TF_VERIFY(false, "unimplemented");
            return 0;
        }

        HdBufferSpecVector GetBufferSpecs() const {
            return _bufferSpecs;
        }

        HdBufferSpecVector _bufferSpecs;
    };

    class _BufferArrayRange : public HdBufferArrayRange
    {
    public:
        _BufferArrayRange() : _bufferArray(nullptr), _numElements(0) {
        }

        virtual bool IsValid() const {
            return _bufferArray;
        }

        virtual bool IsAssigned() const {
            return _bufferArray;
        }

        virtual bool IsImmutable() const {
            return _bufferArray && _bufferArray->IsImmutable();
        }   

        virtual bool Resize(int numElements) {
            _numElements = numElements;
            return _bufferArray->Resize(numElements);
        }

        virtual void CopyData(HdBufferSourceSharedPtr const &bufferSource) {
            /* NOTHING */
        }

        virtual VtValue ReadData(TfToken const &name) const {
            return VtValue();
        }

        virtual int GetOffset() const {
            return 0;
        }

        virtual int GetIndex() const {
            return 0;
        }

        virtual size_t GetNumElements() const {
            return _numElements;
        }

        virtual int GetCapacity() const {
            TF_VERIFY(false, "unimplemented");
            return 0;
        }

        virtual size_t GetVersion() const {
            return 0;
        }

        virtual void IncrementVersion() {
        }

        virtual size_t GetMaxNumElements() const {
            return _bufferArray->GetMaxNumElements();
        }

        virtual void SetBufferArray(HdBufferArray *bufferArray) {
            _bufferArray = static_cast<_BufferArray *>(bufferArray);
        }

        /// Debug dump
        virtual void DebugDump(std::ostream &out) const {
            out << "Hd_NullStrategy::_BufferArray\n";
        }

        virtual void GetBufferSpecs(HdBufferSpecVector *bufferSpecs) const {
        }

        virtual HdBufferArrayUsageHint GetUsageHint() const {
            return _bufferArray->GetUsageHint();
        }

    protected:

        virtual const void *_GetAggregation() const {
            return _bufferArray;
        }

    private:
        _BufferArray * _bufferArray;
        size_t _numElements;
    };


    
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint) override
    {
        return boost::make_shared<Hd_NullStrategy::_BufferArray>(
                role, bufferSpecs, usageHint);
    }

    virtual HdBufferArrayRangeSharedPtr CreateBufferArrayRange() override
    {
        return HdBufferArrayRangeSharedPtr(new _BufferArrayRange());
    }

    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint) const override
    {
        // Always returns different value
        static std::atomic_uint id(0);
        AggregationId hash = id++;  // Atomic
        return hash;
    }

    virtual HdBufferSpecVector GetBufferSpecs(
        HdBufferArraySharedPtr const &bufferArray) const override
    {
        const auto ba =
            boost::static_pointer_cast<_BufferArray>(bufferArray);
        return ba->GetBufferSpecs();
    }

    virtual size_t GetResourceAllocation(
        HdBufferArraySharedPtr const &bufferArray, 
        VtDictionary &result) const override
    {
        return 0;
    }
};

////////////////////////////////////////////////////////////////
// Null Prims

class Hd_NullRprim final : public HdRprim {
public:
    Hd_NullRprim(SdfPath const& id, SdfPath const& instancerId)
     : HdRprim(id, instancerId)
    {

    }

    virtual ~Hd_NullRprim() = default;

    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits,
                      TfToken const   &reprToken) override
    {
        *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
    }


    virtual HdDirtyBits GetInitialDirtyBitsMask() const override
    {
        // Set all bits except the varying flag
        return  (HdChangeTracker::AllSceneDirtyBits) &
               (~HdChangeTracker::Varying);
    }

    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override
    {
        return bits;
    }


protected:
    virtual void _InitRepr(TfToken const &reprToken,
                           HdDirtyBits *dirtyBits) override
    {
        _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                                _ReprComparator(reprToken));
        if (it == _reprs.end()) {
            _reprs.emplace_back(reprToken, HdReprSharedPtr());
        }
    }

private:
    Hd_NullRprim()                                 = delete;
    Hd_NullRprim(const Hd_NullRprim &)             = delete;
    Hd_NullRprim &operator =(const Hd_NullRprim &) = delete;
};

class Hd_NullMaterial final : public HdMaterial {
public:
    Hd_NullMaterial(SdfPath const& id) : HdMaterial(id) {}
    virtual ~Hd_NullMaterial() = default;

    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override
    {
        *dirtyBits = HdMaterial::Clean;
    };

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override {
        return HdMaterial::AllDirty;
    }

    virtual void Reload() override {};

private:
    Hd_NullMaterial()                                  = delete;
    Hd_NullMaterial(const Hd_NullMaterial &)             = delete;
    Hd_NullMaterial &operator =(const Hd_NullMaterial &) = delete;
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
    HdPrimTypeTokens->basisCurves,
    HdPrimTypeTokens->points
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->material
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->texture
};

const TfTokenVector &
Hd_UnitTestNullRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

const TfTokenVector &
Hd_UnitTestNullRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

const TfTokenVector &
Hd_UnitTestNullRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdRenderParam *
Hd_UnitTestNullRenderDelegate::GetRenderParam() const
{
    return nullptr;
}

HdResourceRegistrySharedPtr
Hd_UnitTestNullRenderDelegate::GetResourceRegistry() const
{
    static HdResourceRegistrySharedPtr resourceRegistry(new HdResourceRegistry);
    resourceRegistry->SetNonUniformAggregationStrategy(
        new Hd_NullStrategy());
    resourceRegistry->SetNonUniformImmutableAggregationStrategy(
        new Hd_NullStrategy());
    resourceRegistry->SetUniformAggregationStrategy(
        new Hd_NullStrategy());
    resourceRegistry->SetShaderStorageAggregationStrategy(
        new Hd_NullStrategy());
    resourceRegistry->SetSingleStorageAggregationStrategy(
        new Hd_NullStrategy());
    return resourceRegistry;
}

HdRenderPassSharedPtr
Hd_UnitTestNullRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                                HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(
        new Hd_UnitTestNullRenderPass(index, collection));
}

HdRprim *
Hd_UnitTestNullRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId,
                                    SdfPath const& instancerId)
{
    return new Hd_NullRprim(rprimId, instancerId);
}

void
Hd_UnitTestNullRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    delete rPrim;
}

HdSprim *
Hd_UnitTestNullRenderDelegate::CreateSprim(TfToken const& typeId,
                                           SdfPath const& sprimId)
{
    if (typeId == HdPrimTypeTokens->material) {
        return new Hd_NullMaterial(sprimId);
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

HdSprim *
Hd_UnitTestNullRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->material) {
        return new Hd_NullMaterial(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}


void
Hd_UnitTestNullRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    delete sPrim;
}

HdBprim *
Hd_UnitTestNullRenderDelegate::CreateBprim(TfToken const& typeId,
                                    SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->texture) {
        return new HdTexture(bprimId);
    } else  {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }


    return nullptr;
}

HdBprim *
Hd_UnitTestNullRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->texture) {
        return new HdTexture(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
Hd_UnitTestNullRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}

void
Hd_UnitTestNullRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
