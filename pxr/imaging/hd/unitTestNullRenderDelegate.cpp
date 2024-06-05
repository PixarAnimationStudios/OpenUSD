//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/coordSys.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (print)
    (message)
);


////////////////////////////////////////////////////////////////
// Null Prims

class Hd_NullRprim final : public HdRprim {
public:
    Hd_NullRprim(TfToken const& typeId,
                 SdfPath const& id)
     : HdRprim(id)
     , _typeId(typeId)
    {

    }

    virtual ~Hd_NullRprim() = default;

    TfTokenVector const & GetBuiltinPrimvarNames() const override {
        static const TfTokenVector primvarNames;
        return primvarNames;
    }

    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits,
                      TfToken const   &reprToken) override
    {
        // A render delegate would typically pull values for each
        // dirty bit.  Some tests depend on this behaviour to either
        // update perf counters or test scene delegate getter workflow.
        SdfPath const& id = GetId();

        // PrimId dirty bit is internal to Hydra.

        if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
            GetExtent(delegate);
        }

        if (HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {
            delegate->GetDisplayStyle(id);
        }

        // Points is a primvar

        if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
            _SyncPrimvars(delegate, *dirtyBits);
        }

        // Material Id doesn't have a change tracker test
        if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
            delegate->GetMaterialId(id);
        }

        if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
            // The topology getter depends on prim type
            if (_typeId == HdPrimTypeTokens->mesh) {
                delegate->GetMeshTopology(id);
            } else if (_typeId == HdPrimTypeTokens->basisCurves) {
                delegate->GetBasisCurvesTopology(id);
            }
            // Other prim types don't have a topology
        }

        if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
            delegate->GetTransform(id);
        }

        if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
            delegate->GetVisible(id);
        }

        // Normals is a primvar

        if (HdChangeTracker::IsDoubleSidedDirty(*dirtyBits, id)) {
            delegate->GetDoubleSided(id);
        }

        if (HdChangeTracker::IsCullStyleDirty(*dirtyBits, id)) {
            delegate->GetCullStyle(id);
        }

        // Subddiv tags only apply to refined geom
        // if (HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id)) {
        //    delegate->GetSubdivTags(id);
        //}

        // Widths is a primvar

        if (HdChangeTracker::IsInstancerDirty(*dirtyBits, id)) {
            // Instancer Dirty doesn't have a corrispoinding scene delegate pull
        }

        // InstanceIndex applies to Instancer's not Rprim

        if (HdChangeTracker::IsReprDirty(*dirtyBits, id)) {
            delegate->GetReprSelector(id);
        }

        // RenderTag doesn't have a change tracker test
        if (*dirtyBits & HdChangeTracker::DirtyRenderTag) {
            delegate->GetRenderTag(id);
        }

        // DirtyComputationPrimvarDesc not used
        // DirtyCategories not used

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
    TfToken _typeId;

    void _SyncPrimvars(HdSceneDelegate *delegate,
                       HdDirtyBits      dirtyBits)
    {
        SdfPath const &id = GetId();
        for (size_t interpolation = HdInterpolationConstant;
                    interpolation < HdInterpolationCount;
                  ++interpolation) {
            HdPrimvarDescriptorVector primvars =
                    GetPrimvarDescriptors(delegate,
                            static_cast<HdInterpolation>(interpolation));

            size_t numPrimVars = primvars.size();
            for (size_t primVarNum = 0;
                        primVarNum < numPrimVars;
                      ++primVarNum) {
                HdPrimvarDescriptor const &primvar = primvars[primVarNum];

                if (HdChangeTracker::IsPrimvarDirty(dirtyBits,
                                                    id,
                                                    primvar.name)) {
                    GetPrimvar(delegate, primvar.name);
                }
            }
        }
    }

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

private:
    Hd_NullMaterial()                                  = delete;
    Hd_NullMaterial(const Hd_NullMaterial &)             = delete;
    Hd_NullMaterial &operator =(const Hd_NullMaterial &) = delete;
};

class Hd_NullLight final : public HdLight {
public:
    Hd_NullLight(SdfPath const& id) : HdLight(id) {}
    virtual ~Hd_NullLight() = default;

    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override
    {
        *dirtyBits = HdLight::Clean;
    }

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override
    {
        return HdLight::AllDirty;
    }

private:
    Hd_NullLight()                                 = delete;
    Hd_NullLight(const Hd_NullLight &)             = delete;
    Hd_NullLight &operator =(const Hd_NullLight &) = delete;
};

class Hd_NullCoordSys final : public HdCoordSys {
public:
    Hd_NullCoordSys(SdfPath const& id) : HdCoordSys(id) {}
    virtual ~Hd_NullCoordSys() = default;

    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override
    {
        *dirtyBits = HdCoordSys::Clean;
    };

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override {
        return HdCoordSys::AllDirty;
    }

private:
    Hd_NullCoordSys()                                  = delete;
    Hd_NullCoordSys(const Hd_NullCoordSys &)             = delete;
    Hd_NullCoordSys &operator =(const Hd_NullCoordSys &) = delete;
};

class Hd_NullCamera final : public HdCamera {
public:
    Hd_NullCamera(SdfPath const& id) : HdCamera(id) {}
    virtual ~Hd_NullCamera() override = default;

    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override
    {
        *dirtyBits = HdCamera::Clean;
    };

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override {
        return HdCamera::AllDirty;
    }

private:
    Hd_NullCamera()                                  = delete;
    Hd_NullCamera(const Hd_NullCamera &)             = delete;
    Hd_NullCamera &operator =(const Hd_NullCamera &) = delete;
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
    HdPrimTypeTokens->basisCurves,
    HdPrimTypeTokens->points
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->coordSys,
    HdPrimTypeTokens->domeLight,
    HdPrimTypeTokens->material
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
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
    return resourceRegistry;
}

HdRenderPassSharedPtr
Hd_UnitTestNullRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                                HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(
        new Hd_UnitTestNullRenderPass(index, collection));
}

HdInstancer *
Hd_UnitTestNullRenderDelegate::CreateInstancer(HdSceneDelegate *delegate,
                                               SdfPath const& id)
{
    return new HdInstancer(delegate, id);
}

void
Hd_UnitTestNullRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    delete instancer;
}


HdRprim *
Hd_UnitTestNullRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId)
{
    return new Hd_NullRprim(typeId, rprimId);
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
    } else if (typeId == HdPrimTypeTokens->domeLight) {
        return new Hd_NullLight(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->coordSys) {
        return new Hd_NullCoordSys(sprimId);
    } else if (typeId == HdPrimTypeTokens->camera) {
        return new Hd_NullCamera(sprimId);
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
    } else if (typeId == HdPrimTypeTokens->domeLight) {
        return new Hd_NullLight(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->coordSys) {
        return new Hd_NullCoordSys(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->camera) {
        return new Hd_NullCamera(SdfPath::EmptyPath());
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
    TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());

    return nullptr;
}

HdBprim *
Hd_UnitTestNullRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());

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

HdCommandDescriptors
Hd_UnitTestNullRenderDelegate::GetCommandDescriptors() const
{
    HdCommandArgDescriptor printArgDesc{ _tokens->message, VtValue("") };
    HdCommandArgDescriptors argDescs{ printArgDesc };

    HdCommandDescriptor commandDesc(_tokens->print, "Print command", argDescs);

    return { commandDesc };
}

bool
Hd_UnitTestNullRenderDelegate::InvokeCommand(
    const TfToken &command,
    const HdCommandArgs &args)
{
    if (command == _tokens->print) {
        HdCommandArgs::const_iterator it = args.find(_tokens->message);
        if (it == args.end()) {
            TF_WARN("No argument 'message' argument found.");
            return false;
        }
        VtValue message = it->second;
        std::cout << "Printing the message: " << message << std::endl;
        return true;
    }

    TF_WARN("Unknown command '%s'", command.GetText());
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
