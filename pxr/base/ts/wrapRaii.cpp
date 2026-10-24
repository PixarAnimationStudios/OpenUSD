//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/raii.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
    class _PyAntiRegressionSelector
    {
    public:
        _PyAntiRegressionSelector(TsAntiRegressionMode mode)
            : _mode(mode)
        {
        }

        void Enter()
        {
            _selector.reset(new TsAntiRegressionAuthoringSelector(_mode));
        }

        void Exit()
        {
            _selector.reset();
        }

    private:
        const TsAntiRegressionMode _mode;
        std::unique_ptr<TsAntiRegressionAuthoringSelector> _selector;
    };

    class _PyBehaviorBlock
    {
    public:
        void Enter()
        {
            _block.reset(new TsEditBehaviorBlock());
        }

        void Exit()
        {
            _block.reset();
        }

    private:
        std::unique_ptr<TsEditBehaviorBlock> _block;
    };
}

void wrapRaii(nanobind::module_& m)
{
    namespace nb = nanobind;
    // Context-manager class that temporarily sets the current anti-regression
    // authoring mode for the calling thread.  Use in a 'with' statement.
    nb::class_<_PyAntiRegressionSelector>(
        m, "AntiRegressionAuthoringSelector")
        .def(nb::init<TsAntiRegressionMode>())
        .def("__enter__", [](nb::object self) {
            nb::cast<_PyAntiRegressionSelector&>(self).Enter();
            return self;
        })
        .def("__exit__", [](_PyAntiRegressionSelector& self, nb::args) {
            self.Exit();
        })
        ;

    // Context-manager class that temporarily prevents automatic behaviors when
    // editing splines.  Use in a 'with' statement.
    nb::class_<_PyBehaviorBlock>(
        m, "EditBehaviorBlock")
        .def(nb::init<>())
        .def("__enter__", [](nb::object self) {
            nb::cast<_PyBehaviorBlock&>(self).Enter();
            return self;
        })
        .def("__exit__", [](_PyBehaviorBlock& self, nb::args) {
            self.Exit();
        })
        ;
}
