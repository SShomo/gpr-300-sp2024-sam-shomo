#version 450
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

vec3 sharpen[3] = {
	vec3( 0,-1, 0),
	vec3(-1, 5,-1), 
	vec3( 0,-1, 0) 
};
vec3 boxblur[3] = {
	vec3( 1, 1, 1),
	vec3( 1, 1, 1), 
	vec3( 1, 1, 1) 
};

vec3 edge[3] = {
	vec3( 0,-1, 0),
	vec3(-1, 4,-1), 
	vec3( 0,-1, 0) 
};

vec3 gasblur[3] = {
	vec3( 1, 2, 1),
	vec3( 2, 4, 2), 
	vec3( 1, 2, 1) 
};


in vec2 UV;
uniform sampler2D _ColorBuffer; 
uniform float _Blur;
uniform float _gamma;

uniform int _Kernal;
vec3 kernal[3];

void main(){
    if(_Kernal == 0)
        kernal = boxblur;
    else if (_Kernal == 1)
        kernal = sharpen;
    else if (_Kernal == 2)
        kernal = edge;
    else
        kernal = gasblur;

    vec2 texelSize = _Blur / textureSize(_ColorBuffer,0).xy;
    vec3 totalColor = vec3(0);
    for(int y = 0; y <= 2; y++){
       for(int x = 0; x <= 2; x++){
          vec2 offset = vec2(x,y) * texelSize;
            totalColor += texture(_ColorBuffer,UV + offset).rgb * kernal[x][y];
       }
    }
    if(_Kernal == 0)
        totalColor /= 9;
    else if(_Kernal == 3)
        totalColor /= 16;

    FragColor = vec4(pow(totalColor.rgb, vec3(1.0 / _gamma)),1.0);
}
