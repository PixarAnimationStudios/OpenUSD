//
// Copyright 2017 Autodesk
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
///
/// \file usdShapeRenderer.h
///

#ifndef PXRUSDMAYAGL_USDSHAPERENDERER_H
#define PXRUSDMAYAGL_USDSHAPERENDERER_H

#include "pxrUsdMayaGL/api.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Class to manage rendering of single Maya shape with a single
/// non-instanced transform.
class UsdShapeRenderer
{
public:
	/// \brief Construct a new uninitialized \c ShapeRenderer.
	PXRUSDMAYAGL_API
	UsdShapeRenderer();
	PXRUSDMAYAGL_API
	~UsdShapeRenderer();

	PXRUSDMAYAGL_API
	void PrepareForDelegate(
		HdRenderIndex *renderIndex,
		size_t baseKey,
		const UsdPrim& usdPrim,
		const SdfPathVector& excludePrimPaths,
		UsdTimeCode time,
		uint8_t refineLevel);

	PXRUSDMAYAGL_API
	bool IsPopulated() const { return _isPopulated; }
	PXRUSDMAYAGL_API
	void Populated() { _isPopulated = true; }

	PXRUSDMAYAGL_API
	void SetTransform(const GfMatrix4d& rootXform);

	PXRUSDMAYAGL_API
	const UsdPrim& GetRootPrim() const { return _rootPrim; }
	PXRUSDMAYAGL_API
	const SdfPathVector& GetExcludedPaths() const { return _excludedPaths; }
	PXRUSDMAYAGL_API
	UsdImagingDelegate* GetDelegate() const { return _delegate.get(); }
	PXRUSDMAYAGL_API
	const SdfPath& GetSdfPath() const { return _sharedId; }
	
private:
	SdfPath _sharedId;
	UsdPrim _rootPrim;
	SdfPathVector _excludedPaths;

	std::shared_ptr<UsdImagingDelegate> _delegate;

	bool _isPopulated;
	size_t _sdfKey;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYAGL_USDSHAPERENDERER_H
