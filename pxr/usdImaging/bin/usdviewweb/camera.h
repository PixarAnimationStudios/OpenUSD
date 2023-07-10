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

#ifndef PXR_USD_IMAGING_USD_CAMERA_H
#define PXR_USD_IMAGING_USD_CAMERA_H

#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/matrix4d.h>

#include <iostream>

class Camera
{
public:
	Camera();
	virtual ~Camera(){}

	void setPosition(pxr::GfVec3d cameraPosition) { position = cameraPosition; }
	const pxr::GfVec3d &getPosition() const { return position; }

	void setTarget(pxr::GfVec3d cameraTarget) { target = cameraTarget; }
	const pxr::GfVec3d &getTarget() const { return target; }

	void setViewport(pxr::GfVec4d screenDims) { screenDimensions = screenDims; }

	const pxr::GfMatrix4d &getViewMatrix() const { return viewMatrix; }
    const pxr::GfMatrix4d getProjectionMatrix();
    pxr::GfMatrix4d pickingMatrix(double x, double y) const;


    bool sphere(double d);

	void update();
	void mouseUp();
	void mouseDown(int button, int action, int mods,int xpos,int ypos);
	void mouseMove(int xpos, int ypos);
	void mouseWheel(double xoffset ,double yoffset);

	float rotateSpeed;
	float zoomSpeed;
	float panSpeed;
	float dynamicDampingFactor;
	float minDistance;
	float maxDistance;

protected:
	pxr::GfVec3d getMouseProjectionOnBall(int clientX, int clientY);
	pxr::GfVec2d getMouseOnScreen(int clientX, int clientY);
	void rotateCamera();
	void zoomCamera();
	void panCamera();
	void checkDistances();

	enum CAM_STATE{
		NONE=0,
		ROTATE,
		PAN
	};

	pxr::GfMatrix4d viewMatrix;
	pxr::GfVec4d screenDimensions;

	pxr::GfVec3d target, eye, lastPos, rotStart, rotEnd, up, position;
	pxr::GfVec2d panStart, panEnd;
	float zoom;
	CAM_STATE state, prevState;

private:
    double diameter = 0;
    double distance = 0;
    double fov = 45;
};

#endif