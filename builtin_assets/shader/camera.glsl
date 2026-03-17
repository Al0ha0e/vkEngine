#ifndef CAMERA_H
#define CAMERA_H

layout(set = 0, binding = 0) uniform CameraBlockObject {
    float near;
    float far;
    float fov;
    float aspect;
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} CameraInfo;

#endif