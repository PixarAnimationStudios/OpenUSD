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
#include "pxrUsdMayaGL/usdShapeRenderer.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdShapeRenderer::UsdShapeRenderer()
    : _isPopulated(false)
	, _sdfKey(0)
{
}

UsdShapeRenderer::~UsdShapeRenderer()
{
}

void
UsdShapeRenderer::PrepareForDelegate(
	HdRenderIndex *renderIndex,
	size_t baseKey,
	const UsdPrim& usdPrim,
	const SdfPathVector& excludePrimPaths,
	UsdTimeCode time,
	uint8_t refineLevel
	)
{
	boost::hash_combine(baseKey, usdPrim);
	boost::hash_combine(baseKey, excludePrimPaths);

	if (baseKey != _sdfKey)
	{
		_sdfKey = baseKey;
		// Create a simple hash string to put into a flat SdfPath "hierarchy".
		// This is much faster than more complicated pathing schemes.
		//
		std::string idString = TfStringPrintf("/x%zx", _sdfKey);
		_sharedId = SdfPath(idString);
		_rootPrim = usdPrim;
		_excludedPaths = excludePrimPaths;

		_delegate.reset(new UsdImagingDelegate(renderIndex, _sharedId));

		_isPopulated = false;
	}

	_delegate->SetRefineLevelFallback(refineLevel);

	// Will only react if time actually changes.
	_delegate->SetTime(time);

	_delegate->SetRootCompensation(_rootPrim.GetPath());
}

void
UsdShapeRenderer::SetTransform(const GfMatrix4d& rootXform)
{
	if (! _delegate)
		return;
	_delegate->SetRootTransform(rootXform);
}

PXR_NAMESPACE_CLOSE_SCOPE
