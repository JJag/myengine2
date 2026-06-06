#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    float time;
};

layout (location = 0) out vec4 v_color;


void main()
{
    gl_Position = vec4(a_position, 1.0f);
    gl_Position.x += sin(time) / 2.f;
    gl_Position.y += cos(time) / 5.f;

    v_color = a_color;
}