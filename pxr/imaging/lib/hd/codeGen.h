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
#ifndef HD_CODE_GEN_H
#define HD_CODE_GEN_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/resourceBinder.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <vector>
#include <sstream>

typedef boost::shared_ptr<class HdShader> HdShaderSharedPtr;
typedef boost::shared_ptr<class Hd_GeometricShader> Hd_GeometricShaderPtr;
typedef std::vector<HdShaderSharedPtr> HdShaderSharedPtrVector;

/// HdCodeGen
/// A utility class to compose glsl shader sources and compile them
/// upon request of HdShaderSpec.

class Hd_CodeGen
{
public:
    typedef size_t ID;

    /// Constructor.
    Hd_CodeGen(Hd_GeometricShaderPtr const &geometricShader,
               HdShaderSharedPtrVector const &shaders);

    /// Return the hash value of glsl shader to be generated.
    ID ComputeHash() const;

    /// Generate shader source and compile it.
    HdGLSLProgramSharedPtr Compile();

    /// Return the generated vertex shader source
    const std::string &GetVertexShaderSource() const { return _vsSource; }

    /// Return the generated tess control shader source
    const std::string &GetTessControlShaderSource() const { return _tcsSource; }

    /// Return the generated tess eval shader source
    const std::string &GetTessEvalShaderSource() const { return _tesSource; }

    /// Return the generated geometry shader source
    const std::string &GetGeometryShaderSource() const { return _gsSource; }

    /// Return the generated fragment shader source
    const std::string &GetFragmentShaderSource() const { return _fsSource; }

    /// Return the pointer of metadata to be populated by resource binder.
    Hd_ResourceBinder::MetaData *GetMetaData() { return &_metaData; }

private:
    enum { PRIM_OTHER, PRIM_TRI, PRIM_COARSE_QUAD, PRIM_REFINED_QUAD, PRIM_PATCH };

    void _GenerateDrawingCoord();
    void _GenerateConstantPrimVar();
    void _GenerateInstancePrimVar();
    void _GenerateElementPrimVar(int primType);
    void _GenerateVertexPrimVar(int primType);
    void _GenerateShaderParameters();

    Hd_ResourceBinder::MetaData _metaData;
    Hd_GeometricShaderPtr _geometricShader;
    HdShaderSharedPtrVector _shaders;

    // source buckets
    std::stringstream _genCommon, _genVS, _genTCS, _genTES, _genGS, _genFS;
    std::stringstream _procVS, _procTCS, _procTES, _procGS;

    // generated sources (for diagnostics)
    std::string _vsSource;
    std::string _tcsSource;
    std::string _tesSource;
    std::string _gsSource;
    std::string _fsSource;

};

#endif  // HD_CODE_GEN_H
