#version 450
out vec4 FragColor; //The color of this fragment
in Surface{
	vec3 WorldPos; //Vertex position in world space
	vec3 WorldNormal; //Vertex normal in world space
	vec2 TexCoord;
	mat3 TBN;
	vec2 UV;
}fs_in;

in vec4 LightSpacePos;
float bias;

uniform sampler2D _MainTex; 
uniform sampler2D normalMap;
uniform vec3 _EyePos;
uniform vec3 _LightDirection;
uniform vec3 _LightColor;
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);
uniform sampler2D _ColorBuffer;
uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform float minBias;
uniform float maxBias;

struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

float ShadowCalculation(sampler2D shadowMap, vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z - bias;


	float totalShadow = 0;
	vec2 texelOffset = 1.0 /  textureSize(shadowMap,0);
	for(int y = -1; y <=1; y++){
		for(int x = -1; x <=1; x++){
			vec2 uv = fragPosLightSpace.xy + vec2(x * texelOffset.x, y * texelOffset.y);
			totalShadow+=step(texture(shadowMap,uv).r,currentDepth);
		}
	}
	totalShadow/=9.0;


    return totalShadow;
}  

void main(){
	//Make sure fragment normal is still length 1 after interpolation.
	vec3 color = texture(diffuseTexture, fs_in.TexCoord).rgb;
	vec3 normal = normalize(fs_in.WorldNormal);

	//vec3 toLight = -_LightDirection;
	vec3 toLight = normalize(_LightDirection - fs_in.WorldPos);

	bias = max(maxBias * (1.0 - dot(fs_in.WorldNormal, toLight)),minBias);
	//vec3 ambient = (_Material.Ka * _AmbientColor) * lightColor;

	normal = texture(normalMap, fs_in.TexCoord).rgb;
	normal = normal * 2.0 - 1.0;   
	normal = normalize(fs_in.TBN * normal);

	float diffuseFactor = max(dot(toLight, normal),0.0);
	vec3 diffuse = diffuseFactor * _LightColor;

	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);
	vec3 halfway = normalize(toLight + toEye);

	float specularFactor = pow(max(dot(normal,halfway),0.0),_Material.Shininess);

	//Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
	lightColor += _AmbientColor * _Material.Ka;

	//vec3 color = 1.0-texture(_ColorBuffer,fs_in.UV).rgb;
	float shadow = ShadowCalculation(shadowMap, LightSpacePos); 

	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rrr;

	vec3 lighting = lightColor * (1.0 - shadow);


	FragColor = vec4(objectColor * lighting,1.0);
}
