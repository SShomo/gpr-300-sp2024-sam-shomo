#version 450
//Vertex attributes
layout(location = 0) in vec3 gPosition;
layout(location = 1) in vec3 gNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in vec3 gAlbedo;

uniform mat4 _Model; 
uniform mat4 _ViewProjection;
uniform mat4 LightSpaceMatrix;

out Surface{
	vec3 WorldPos; //Vertex position in world space
	vec3 WorldNormal; //Vertex normal in world space
	vec2 TexCoord;
	vec4 FragPosLightSpace;
}vs_out;

void main(){
	//Transform vertex position to World Space.
	vs_out.WorldPos = vec3(_Model * vec4(gPosition,1.0));
	//Transform vertex normal to world space using Normal Matrix
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * gNormal;
	vs_out.TexCoord = vTexCoord;

	vs_out.FragPosLightSpace = LightSpaceMatrix * vec4(vs_out.WorldPos,1.0);

	gl_Position = _ViewProjection * _Model * vec4(gPosition,1.0);
}