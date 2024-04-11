#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube cubemap;

void main(){
    vec4 result = texture(cubemap, TexCoords);
    FragColor = result;
}