#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

struct CameraComponent {
    float fov = 80.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.f;
    float aspectRatio = 1.778f; // default 16:9
    bool isOrthographic = false;

    // Orthographic specifics
    float orthoLeft = -1.0f;
    float orthoRight = 1.0f;
    float orthoTop = 1.0f;
    float orthoBottom = -1.0f;

    bool primary = false; // is this the main camera?
};

#endif