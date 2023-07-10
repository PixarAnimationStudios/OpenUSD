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

#include "camera.h"

#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/quaternion.h>
#include <pxr/base/gf/frustum.h>

// GLFW definitions
#ifndef GLFW_PRESS
#define GLFW_PRESS 1
#endif

#ifndef GLFW_MOUSE_BUTTON_RIGHT
#define GLFW_MOUSE_BUTTON_RIGHT 1
#endif

pxr::GfVec3d rotate(pxr::GfQuaternion const& q, pxr::GfVec3d const& v)
{
    pxr::GfVec3d uv = pxr::GfCross(q.GetImaginary(), v);
    pxr::GfVec3d uuv = pxr::GfCross(q.GetImaginary(), uv);
    return v + ((uv * q.GetReal()) + uuv) * 2;
}

Camera::Camera()
        :
        rotateSpeed(1.f),
        zoomSpeed(1.2f),
        panSpeed(0.1f),
        dynamicDampingFactor(0.2f),
        minDistance(0.f),
        maxDistance(std::numeric_limits<float>::infinity()),
        viewMatrix(1.f),
        target(0.f),
        eye(0.f),
        lastPos(0.f),
        rotStart(0.f),
        rotEnd(0.f),
        up(0.f,1.f,0.f),
        position(0.f, 0.f, 1.f),
        panStart(0.f),
        panEnd(0.f),
        zoom(0.f),
        state(CAM_STATE::NONE),
        prevState(CAM_STATE::NONE)

{}

void Camera::update()
{
    eye = position - target;
    if (state == CAM_STATE::ROTATE)
        rotateCamera();
    else if (state == CAM_STATE::PAN)
        panCamera();
    else
        zoomCamera();

    position = target + eye;

    checkDistances();

    viewMatrix.SetLookAt(position, target, up);

    auto distance = lastPos - position;
    if (distance.GetLength() > 0.f)
        lastPos = position;
}

void Camera::rotateCamera()
{
    double angle = acos( pxr::GfDot(rotStart,rotEnd) / rotStart.GetLength() / rotEnd.GetLength() );

    if( !std::isnan(angle) && angle != 0.f )
    {

        pxr::GfVec3d axis = pxr::GfGetNormalized(pxr::GfCross( rotStart, rotEnd ));
        if(std::isnan(axis[0]) || std::isnan(axis[1]) || std::isnan(axis[2]))
            return;

        angle *= rotateSpeed;

        pxr::GfRotation rotation = pxr::GfRotation(axis, pxr::GfRadiansToDegrees(-angle));

        eye = rotate( rotation.GetQuaternion() ,  eye);

        up =  rotate(rotation.GetQuaternion() , up);
        rotEnd = rotate( rotation.GetQuaternion() , rotEnd);

        rotation = pxr::GfRotation(axis, angle * (dynamicDampingFactor - 1.f));
        rotStart = rotate(rotation.GetQuaternion(),rotStart);
    }
}

void Camera::zoomCamera()
{
    float factor = 1.f + (float)(-zoom) * zoomSpeed;
    if (factor != 1.f && factor > 0.f)
    {
        eye = eye * (float)factor;
        zoom += (float)(-zoom) * dynamicDampingFactor;
    }
}

void Camera::panCamera()
{
    pxr::GfVec2d mouseChange = panEnd - panStart;

    if (mouseChange.GetLengthSq() != 0.f)
    {
        mouseChange *= eye.GetLengthSq() * panSpeed;
        pxr::GfVec3d pan =pxr::GfGetNormalized(pxr::GfCross(eye, up));

        pan *= mouseChange[0];
        pxr::GfVec3d camUpClone = pxr::GfGetNormalized(up);

        camUpClone *=  mouseChange[1];
        pan += camUpClone;
        position += pan;

        target +=  pan;
        panStart += (panEnd - panStart) * dynamicDampingFactor;
    }
}

void Camera::checkDistances()
{
    if ( position.GetLengthSq() > maxDistance * maxDistance)
        position = pxr::GfGetNormalized(position) * maxDistance;

    if (eye.GetLengthSq() < minDistance * minDistance)
    {
        eye = pxr::GfGetNormalized(eye) * minDistance;
        position = target + eye;
    }
}

pxr::GfVec3d Camera::getMouseProjectionOnBall(int clientX, int clientY)
{
    auto mouseOnBall = pxr::GfVec3d(
            ((float)clientX - (float)screenDimensions[2] * 0.5f) / (float)(screenDimensions[2] * 0.5f),
            ((float)clientY - (float)screenDimensions[3] * 0.5f) / (float)(screenDimensions[3] * 0.5f),
            0.f );

    double length = mouseOnBall.GetLength();

    if (length > 1.0)
        mouseOnBall = pxr::GfGetNormalized(mouseOnBall);
    else
        mouseOnBall[2] = sqrt(1.0 - length * length);

    eye = target -  position;

    pxr::GfVec3d upClone = up;
    upClone = pxr::GfGetNormalized(upClone);
    pxr::GfVec3d projection = upClone * mouseOnBall[1];

    pxr::GfVec3d cross = pxr::GfGetNormalized( pxr::GfCross(up,eye));

    cross *= mouseOnBall[0];
    projection +=cross;

    pxr::GfVec3d eyeClone = pxr::GfGetNormalized(eye);

    projection += eyeClone * mouseOnBall[2];

    return projection;
}

void Camera::mouseDown(int button, int action, int mods, int xpos, int ypos)
{
    if( action == GLFW_PRESS )
    {
        if(button == GLFW_MOUSE_BUTTON_RIGHT)
            state = CAM_STATE::PAN;
        else
            state = CAM_STATE::ROTATE;
    }else
        state = CAM_STATE::NONE;

    if (state == CAM_STATE::ROTATE)
    {
        rotStart = getMouseProjectionOnBall(xpos, ypos);
        rotEnd = rotStart;
    }
    else if (state == CAM_STATE::PAN)
    {
        panStart = getMouseOnScreen(xpos, ypos);
        panEnd = panStart;
    }
}

void Camera::mouseWheel(double xoffset ,double yoffset)
{
    if (yoffset != 0.0)
    {
        yoffset /= 3.0;
        zoom += (float)yoffset * 0.05f;
    }
}

void Camera::mouseMove(int xpos,int ypos)
{
    if( state == CAM_STATE::ROTATE )
        rotEnd = getMouseProjectionOnBall(xpos,ypos);
    else if( state == CAM_STATE::PAN )
        panEnd = getMouseOnScreen(xpos,ypos);
}

pxr::GfVec2d Camera::getMouseOnScreen(int clientX, int clientY)
{
    return pxr::GfVec2d(
            (double)(clientX - screenDimensions[0]) / screenDimensions[2],
            (double)(clientY - screenDimensions[1]) / screenDimensions[3] );
}

void Camera::mouseUp()
{
    state = CAM_STATE::NONE;
}

bool Camera::sphere(double d) {
    diameter = d;
    zoomSpeed = diameter * 0.01;
    panSpeed = zoomSpeed * 0.001;
    return true;
}

const pxr::GfMatrix4d Camera::getProjectionMatrix() {
    pxr::GfFrustum frustum;
    frustum.SetPerspective(fov * (180/3.14), screenDimensions[2] / screenDimensions[3], diameter / 100, diameter * 10);
    return frustum.ComputeProjectionMatrix();
}
