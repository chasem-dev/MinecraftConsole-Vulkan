#version 450

layout(push_constant) uniform PushConstants {
  mat4 mvp;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = inColor;
  gl_Position = pc.mvp * vec4(inPosition, 1.0);
}
