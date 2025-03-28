//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HGIMETAL_SHADERGENERATOR_H
#define PXR_IMAGING_HGIMETAL_SHADERGENERATOR_H

#include "pxr/imaging/hgi/shaderGenerator.h"
#include "pxr/imaging/hgiMetal/shaderSection.h"
#include "pxr/imaging/hgiMetal/api.h"

PXR_NAMESPACE_OPEN_SCOPE

//Shader program structure

//Global scope includes
//Global scope macros
//Global scope struct defs
//Global scope member decls
//Global scope function defs
//Stage program scope struct defs
//Stage program scope member decls
//Stage program scope function defs
//Stage program scope main program
//Stage function entryPoint definition
//Stage function implementation

class HgiMetal;

using HgiMetalShaderStageEntryPointUniquePtr =
    std::unique_ptr<class HgiMetalShaderStageEntryPoint>;

/// \class HgiMetalShaderGenerator
///
/// Takes in a descriptor and spits out metal code through it's Execute function
///

class HgiMetalShaderGenerator final: public HgiShaderGenerator
{
public:
    HGIMETAL_API
    HgiMetalShaderGenerator(
        HgiMetal const *hgi,
        const HgiShaderFunctionDesc &descriptor);

    HGIMETAL_API
    ~HgiMetalShaderGenerator() override;

    HGIMETAL_API
    HgiMetalShaderSectionUniquePtrVector* GetShaderSections();

    template<typename SectionType, typename ...T>
    SectionType * CreateShaderSection(T && ...t);

protected:
    HGIMETAL_API
    void _Execute(std::ostream &ss) override;

private:
    HgiMetalShaderStageEntryPointUniquePtr
    _BuildShaderStageEntryPoints(
        const HgiShaderFunctionDesc &descriptor);

    void _BuildKeywordInputShaderSections(const HgiShaderFunctionDesc &descriptor);

    HgiMetal const *_hgi;
    HgiMetalShaderSectionUniquePtrVector _shaderSections;
    HgiMetalShaderStageEntryPointUniquePtr _generatorShaderSections;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
