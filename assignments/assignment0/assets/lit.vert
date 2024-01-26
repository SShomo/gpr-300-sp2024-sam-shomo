#version 450
//Vertex attributes
layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent; 

uniform mat4 _Model; 
uniform mat4 _ViewProjection;

out Surface{
	vec3 WorldPos; //Vertex position in world space
	vec3 WorldNormal; //Vertex normal in world space
	vec2 TexCoord;
	mat3 TBN;
}vs_out;

void main(){
	//Transform vertex position to World Space.
	vs_out.WorldPos = vec3(_Model * vec4(vPos,1.0));
	//Transform vertex normal to world space using Normal Matrix
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * vNormal;
	vs_out.TexCoord = vTexCoord;
	gl_Position = _ViewProjection * _Model * vec4(vPos,1.0);

   vec3 T = normalize(vec3(_Model * vec4(vTangent,   0.0)));
   vec3 N = normalize(vec3(_Model * vec4(vNormal,    0.0)));
   vec3 B = cross(N, T);

   T = normalize(T - dot(T,N)*N);
   vs_out.TBN = mat3(T, B, N); 
   
}
