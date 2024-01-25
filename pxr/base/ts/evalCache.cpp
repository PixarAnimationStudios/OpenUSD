//
// Copyright 2023 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/ts/evalCache.h"

#include "pxr/base/ts/data.h"
#include "pxr/base/ts/keyFrameUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

// static
Ts_UntypedEvalCache::SharedPtr
Ts_UntypedEvalCache::New(const TsKeyFrame &kf1, const TsKeyFrame &kf2)
{
    return Ts_GetKeyFrameData(kf1)->CreateEvalCache(Ts_GetKeyFrameData(kf2));
}

VtValue
Ts_UntypedEvalCache::EvalUncached(const TsKeyFrame &kf1,
                                    const TsKeyFrame &kf2,
                                    TsTime time)
{
    return Ts_GetKeyFrameData(kf1)->
        EvalUncached(Ts_GetKeyFrameData(kf2), time);
}

VtValue
Ts_UntypedEvalCache::EvalDerivativeUncached(const TsKeyFrame &kf1,
                                              const TsKeyFrame &kf2,
                                              TsTime time)
{
    return Ts_GetKeyFrameData(kf1)->
        EvalDerivativeUncached(Ts_GetKeyFrameData(kf2), time);
}


////////////////////////////////////////////////////////////////////////
// ::New implementations for Ts_EvalCache GfQuat specializations

std::shared_ptr<Ts_EvalCache<GfQuatd, true> >
Ts_EvalCache<GfQuatd, true>::New(const TsKeyFrame &kf1,
    const TsKeyFrame &kf2)
{
    return static_cast<const Ts_TypedData<GfQuatd>*>(
        Ts_GetKeyFrameData(kf1))->
            CreateTypedEvalCache(Ts_GetKeyFrameData(kf2));
}

std::shared_ptr<Ts_EvalCache<GfQuatf, true> >
Ts_EvalCache<GfQuatf, true>::New(const TsKeyFrame &kf1,
    const TsKeyFrame &kf2)
{
    return static_cast<const Ts_TypedData<GfQuatf>*>(
        Ts_GetKeyFrameData(kf1))->
            CreateTypedEvalCache(Ts_GetKeyFrameData(kf2));
}

PXR_NAMESPACE_CLOSE_SCOPE
