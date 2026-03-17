#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "camera.glsl"

layout(push_constant) uniform PushConstants{
    mat4 model;
};

layout(set = 2, binding = 0) uniform JointBlockObject {
    mat4 joints[256];
} JointBlock;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inWeights;
layout(location = 5) in uvec4 inJointIDs;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vViewPos;
layout(location = 2) out vec2 vTexCoord;

void main() {
    mat4 skinMat =
          inWeights.x * JointBlock.joints[inJointIDs.x]
        + inWeights.y * JointBlock.joints[inJointIDs.y]
        + inWeights.z * JointBlock.joints[inJointIDs.z]
        + inWeights.w * JointBlock.joints[inJointIDs.w];

    vec4 skinnedPos = skinMat * vec4(inPosition, 1.0);
    vec3 skinnedNormal = mat3(skinMat) * inNormal;

    mat4 mvMat = CameraInfo.view * model;
    vec4 viewPos = mvMat * skinnedPos;
    vViewPos = viewPos.xyz;
    
    vNormal = normalize(mat3(mvMat) * skinnedNormal);
    vTexCoord = inTexCoord;

    gl_Position = CameraInfo.projection * viewPos;
}