#version 450
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 UV;
uniform sampler2D _ColorBuffer;

void main(){
	vec2 texelSize = 1.0 / textureSize(_ColorBuffer,0).xy;
    vec3 totalColor = vec3(0);
    for(int y = -2; y <= 2; y++){
       for(int x = -2; x <= 2; x++){
          vec2 offset = vec2(x,y) * texelSize;
          totalColor += texture(_ColorBuffer,UV + offset).rgb;
       }
    }
    totalColor/=(5 * 5);
    FragColor = vec4(totalColor,1.0);
}
