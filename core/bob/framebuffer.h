#include <glm/glm.hpp>

namespace bob {
	struct Framebuffer {
		unsigned int fbo;
		unsigned int colorBuffer[8];
		unsigned int depthBuffer;
		unsigned int width;
		unsigned int height;
	};
	Framebuffer createFramebufferWithRBO(unsigned int width, unsigned int height, int colorFormat);
	Framebuffer createFramebufferWithDepthBuffer(unsigned int width, unsigned int height, int colorFormat);
	Framebuffer createShadowMapFramebuffer(unsigned int width, unsigned int height, int colorFormat);
}
