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
#ifndef PXR_BASE_TF_TF_H
#define PXR_BASE_TF_TF_H

#include <pxr/base/tf/api.h>

#include <pxr/base/tf/mallocTag.h>
#include <pxr/base/tf/singleton.h>

#include <pxr/base/tf/fastCompression.h>

#include <pxr/base/tf/hash.h>
#include <pxr/base/tf/hashset.h>

#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/smallVector.h>

#include <pxr/base/tf/ostreamMethods.h>

#include <pxr/base/tf/callContext.h>

#include <pxr/base/tf/diagnosticLite.h>

#include <pxr/base/tf/diagnosticHelper.h>

#include <pxr/base/tf/diagnostic.h>

#include <pxr/base/tf/preprocessorUtils.h>
#include <pxr/base/tf/preprocessorUtilsLite.h>
#include <pxr/base/tf/safeTypeCompare.h>
#include <pxr/base/tf/templateString.h>
#include <pxr/base/tf/stringUtils.h>

#include <pxr/base/tf/pointerAndBits.h>
#include <pxr/base/tf/token.h>

#include <pxr/base/tf/typeFunctions.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/stopwatch.h>

#include <pxr/base/tf/debug.h>

#include <pxr/base/tf/enum.h>

#include <pxr/base/tf/cxxCast.h>

#include <pxr/base/tf/type.h>

#include <pxr/base/tf/type_Impl.h>

#include <pxr/base/tf/declarePtrs.h>

#include <pxr/base/tf/nullPtr.h>

#include <pxr/base/tf/refCount.h>
#include <pxr/base/tf/refBase.h>

#include <pxr/base/tf/expiryNotifier.h>

// #include <pxr/base/tf/refPtrTracker.h>

#include <pxr/base/tf/weakBase.h>
#include <pxr/base/tf/weakPtrFacade.h>

#include <pxr/base/tf/weakPtr.h>
#include <pxr/base/tf/anyWeakPtr.h>

#include <pxr/base/tf/error.h>
#include <pxr/base/tf/errorMark.h>
#include <pxr/base/tf/errorTransport.h>

#include <pxr/base/tf/diagnosticBase.h>

#include <pxr/base/tf/status.h>

#include <pxr/base/tf/diagnosticMgr.h>

#include <pxr/base/tf/anyUniquePtr.h>
#include <pxr/base/tf/atomicOfstreamWrapper.h>
#include <pxr/base/tf/atomicRenameUtil.h>
#include <pxr/base/tf/bigRWMutex.h>
#include <pxr/base/tf/bitUtils.h>

#include <pxr/base/tf/debugNotice.h>
#include <pxr/base/tf/denseHashMap.h>
#include <pxr/base/tf/denseHashSet.h>

#include <pxr/base/tf/dl.h>

#include <pxr/base/tf/envSetting.h>

#include <pxr/base/tf/exception.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/functionRef.h>
#include <pxr/base/tf/functionTraits.h>
#include <pxr/base/tf/getenv.h>

// #include <pxr/base/tf/instantiateSingleton.h>
// #include <pxr/base/tf/instantiateStacked.h>
// #include <pxr/base/tf/instantiateType.h>
#include <pxr/base/tf/iterator.h>

#include <pxr/base/tf/pyInterpreter.h>

#include <pxr/base/tf/pyCall.h>
#include <pxr/base/tf/pyLock.h>

#include <pxr/base/tf/pySafePython.h>

#include <pxr/base/tf/pyError.h>
#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyArg.h>
#include <pxr/base/tf/pyInvoke.h>
#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr/base/tf/pyUtils.h>

#include <pxr/base/tf/pyResultConversions.h>

#include <pxr/base/tf/pyObjWrapper.h>
#include <pxr/base/tf/pyTracing.h>

#include <pxr/base/tf/meta.h>


#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/patternMatcher.h>

#include <pxr/base/tf/regTest.h>
#include <pxr/base/tf/safeOutputFile.h>

#include <pxr/base/tf/scoped.h>
#include <pxr/base/tf/scopeDescription.h>
#include <pxr/base/tf/scopeDescriptionPrivate.h>
#include <pxr/base/tf/scriptModuleLoader.h>
#include <pxr/base/tf/setenv.h>

#include <pxr/base/tf/span.h>
#include <pxr/base/tf/spinRWMutex.h>
#include <pxr/base/tf/stacked.h>
#include <pxr/base/tf/stackTrace.h>
#include <pxr/base/tf/staticData.h>
#include <pxr/base/tf/staticTokens.h>

#include <pxr/base/tf/stl.h>

#include <pxr/base/tf/typeInfoMap.h>
#include <pxr/base/tf/warning.h>

// #include <pxr/base/tf/pxrLZ4/lz4.h>
// #include <pxr/base/tf/pxrDoubleConversion/bignum-dtoa.h>
// #include <pxr/base/tf/pxrDoubleConversion/bignum.h>
// #include <pxr/base/tf/pxrDoubleConversion/cached-powers.h>
// #include <pxr/base/tf/pxrDoubleConversion/diy-fp.h>
// #include <pxr/base/tf/pxrDoubleConversion/double-conversion.h>
// #include <pxr/base/tf/pxrDoubleConversion/fast-dtoa.h>
// #include <pxr/base/tf/pxrDoubleConversion/fixed-dtoa.h>
// #include <pxr/base/tf/pxrDoubleConversion/ieee.h>
// #include <pxr/base/tf/pxrDoubleConversion/strtod.h>
// #include <pxr/base/tf/pxrDoubleConversion/utils.h>
// #include <pxr/base/tf/pxrPEGTL/pegtl.h>
#include <pxr/base/tf/pxrTslRobinMap/robin_growth_policy.h>
#include <pxr/base/tf/pxrTslRobinMap/robin_hash.h>
#include <pxr/base/tf/pxrTslRobinMap/robin_map.h>
#include <pxr/base/tf/pxrTslRobinMap/robin_set.h>
#include <pxr/base/tf/pxrCLI11/CLI11.h>

#include <pxr/base/tf/pyPtrHelpers.h>

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/typeNotice.h>

#include <pxr/base/tf/noticeRegistry.h>

#include <pxr/base/tf/pyContainerConversions.h>

#include <pxr/base/tf/refPtr.h>

#endif // TF_H
