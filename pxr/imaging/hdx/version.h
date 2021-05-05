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
#ifndef PXR_IMAGING_HDX_VERSION_H
#define PXR_IMAGING_HDX_VERSION_H

// 1  -> 2 : split HdxRenderSetupTask out of HdxRenderTask
// 2  -> 3 : move simpleLightingShader to Hdx.
// 3  -> 4 : move camera and light to Hdx.
// 4  -> 5 : move drawTarget to Hdx.
// 5  -> 6 : change HdxShadowMatrixComputation signature.
// 6  -> 7 : make HdxShadowMatrixComputationSharedPtr std::shared_ptr instead of boost::shared_ptr
// 7  -> 8 : added another HdxShadowMatrixComputation signature.
#define HDX_API_VERSION  8

#endif // PXR_IMAGING_HDX_VERSION_H
