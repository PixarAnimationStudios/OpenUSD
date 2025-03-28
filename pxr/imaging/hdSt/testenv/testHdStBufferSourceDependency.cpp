//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class ComputationBase : public HdNullBufferSource
{
public:
    virtual int GetResult() const = 0;
};

using ComputationSharedPtr = std::shared_ptr<ComputationBase>;

class CpuComputation : public ComputationBase
{
public:
    CpuComputation(int source) : _source(source), _result(0) { }

    virtual bool Resolve() {
        if (!_TryLock()) return false;

        _result = _source + 1;

        _SetResolved();
        return true;
    }
    virtual int GetResult() const {
        return _result;
    }

protected:
    virtual bool _CheckValid() const {
        return true;
    }

private:
    int _source;
    int _result;
};

class CpuDependentComputation : public ComputationBase
{
public:
    CpuDependentComputation(ComputationSharedPtr const &other) :
        _other(other), _result(0) {
    }
    virtual bool Resolve() {
        if (_other && !_other->IsResolved()) return false;
        if (!_TryLock()) return false;

        _result = _other->GetResult() + 10;

        _SetResolved();
        return true;
    }
    virtual int GetResult() const {
        return _result;
    }

protected:
    virtual bool _CheckValid() const {
        return true;
    }

private:
    ComputationSharedPtr _other;
    int _result;
};

int main()
{
    TfErrorMark mark;

    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfSharedGLContextScopeHolder sharedContext;

    HgiUniquePtr const hgi = Hgi::CreatePlatformDefaultHgi();
    HdDriver driver{HgiTokens->renderDriver, VtValue(hgi.get())};
    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> const index(
        HdRenderIndex::New(&renderDelegate, {&driver}));
    HdStResourceRegistrySharedPtr const registry =
        std::static_pointer_cast<HdStResourceRegistry>(
            index->GetResourceRegistry());

    {
        // just a computation.
        ComputationSharedPtr comp(new CpuComputation(100));
        registry->AddSource(comp);
        registry->Commit();
        TF_VERIFY(comp->GetResult() == 101);
    }

    {
        // two computations, has to run sequentially.
        ComputationSharedPtr comp1(new CpuComputation(100));
        ComputationSharedPtr comp2(new CpuDependentComputation(comp1));
        registry->AddSource(comp1);
        registry->AddSource(comp2);
        registry->Commit();
        TF_VERIFY(comp1->GetResult() == 101);
        TF_VERIFY(comp2->GetResult() == 111);
    }

    {
        // two computations, has to run sequentially.
        // registring order shouldn't be a problem.
        ComputationSharedPtr comp1(new CpuComputation(100));
        ComputationSharedPtr comp2(new CpuDependentComputation(comp1));
        registry->AddSource(comp2);
        registry->AddSource(comp1);
        registry->Commit();
        TF_VERIFY(comp1->GetResult() == 101);
        TF_VERIFY(comp2->GetResult() == 111);
    }

    {
        // three computations, has to run sequentially.
        ComputationSharedPtr comp1(new CpuComputation(100));
        ComputationSharedPtr comp2(new CpuDependentComputation(comp1));
        ComputationSharedPtr comp3(new CpuDependentComputation(comp2));
        registry->AddSource(comp1);
        registry->AddSource(comp2);
        registry->AddSource(comp3);
        registry->Commit();
        TF_VERIFY(comp1->GetResult() == 101);
        TF_VERIFY(comp2->GetResult() == 111);
        TF_VERIFY(comp3->GetResult() == 121);
    }

    {
        // many computations, can be run in parallel once comp1 has finished.
        ComputationSharedPtr comp1(new CpuComputation(100));
        registry->AddSource(comp1);
        std::vector<ComputationSharedPtr> comps;
        for (int i = 0; i < 100; ++i) {
            ComputationSharedPtr comp(new CpuDependentComputation(comp1));
            registry->AddSource(comp);
            comps.push_back(comp);
        }
        registry->Commit();

        TF_VERIFY(comp1->GetResult() == 101);
        for (int i = 0; i < 100; ++i) {
            TF_VERIFY(comps[i]->GetResult() == 111);
        }
    }

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
