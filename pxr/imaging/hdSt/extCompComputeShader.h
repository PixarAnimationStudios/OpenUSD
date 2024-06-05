//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_COMP_COMPUTE_SHADER_H
#define PXR_IMAGING_HD_ST_EXT_COMP_COMPUTE_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_ExtCompComputeShaderSharedPtr =
    std::shared_ptr<class HdSt_ExtCompComputeShader>;

class HdExtComputation;

/// \class HdSt_ExtCompComputeShader
///
/// An internal representation of a compute shader in Storm that allows the
/// use of the code generation and resource binding system to generate a
/// shader program.
///
class HdSt_ExtCompComputeShader final : public HdStShaderCode {
public:
    HDST_API
    HdSt_ExtCompComputeShader(HdExtComputation const *extComp);

    HDST_API
    ~HdSt_ExtCompComputeShader() override;


    // ---------------------------------------------------------------------- //
    /// \name HdStShaderCode (pure) virtual interface                         //
    // ---------------------------------------------------------------------- //

    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;

    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder) override;

    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder) override;

    HDST_API
    void AddBindings(HdStBindingRequestVector *customBindings) override;

    HDST_API
    ID ComputeHash() const override;

    // ---------------------------------------------------------------------- //

    HDST_API
    SdfPath const& GetExtComputationId() const;

private:
    // No copying
    HdSt_ExtCompComputeShader(const HdSt_ExtCompComputeShader &) = delete;
    HdSt_ExtCompComputeShader &operator =(const HdSt_ExtCompComputeShader &)
        = delete;
    
    HdExtComputation const * const _extComp;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_COMP_COMPUTE_SHADER_H
