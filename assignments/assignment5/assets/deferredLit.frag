#version 450
out vec4 FragColor; //The color of this fragment
in vec2 UV;

uniform vec3 _EyePos;
uniform vec3 _LightColor = vec3(1.0f);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);
uniform vec3 lightPos;

uniform layout(binding = 0) sampler2D _gPositions;
uniform layout(binding = 1) sampler2D _gNormals;
uniform layout(binding = 2) sampler2D _gAlbedo;

struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

struct PointLight{
	vec3 position;
	float radius;
	vec3 color;
};
#define MAX_POINT_LIGHTS 64
uniform PointLight _PointLights[MAX_POINT_LIGHTS];
PointLight _MainLight;


//Linear falloff
float attenuateLinear(float distance, float radius){
	return clamp((radius-distance)/radius, 0.0,1.0);
}
//Exponential falloff
float attenuateExponential(float distance, float radius){
	float i = clamp(1.0 - pow(distance/radius,4.0),0.0,1.0);
	return i * i;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 pos){
	vec3 diff = light.position - pos;

	vec3 toLight = normalize(diff);
	//TODO: Usual blinn-phong calculations for diffuse + specular
	float diffuseFactor = max(dot(normal, toLight),0.0);
	vec3 toEye = normalize(_EyePos - pos);
	vec3 halfway = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,halfway),0.0),_Material.Shininess);

	vec3 lightColor = (diffuseFactor + specularFactor) * light.color;
	//Attenuation
	float d = length(diff); //Distance to light
	lightColor *= attenuateExponential(d,light.radius); //See below for attenuation options
	return lightColor;
}


void main(){
	vec3 pos = texture(_gPositions, UV).xyz;
	vec3 normal = texture(_gNormals, UV).xyz;
	vec3 totalLight = vec3(0);

	_MainLight.position = vec3(lightPos);
	_MainLight.radius = 5;
	_MainLight.color = _LightColor;

	totalLight += calcPointLight(_MainLight,normal, pos);

	for(int i=0;i<MAX_POINT_LIGHTS;i++){
		totalLight+=calcPointLight(_PointLights[i],normal, pos);
	}

	vec3 albedo = texture(_gAlbedo, UV).xyz;
	FragColor = vec4(albedo * totalLight, 1);
}
