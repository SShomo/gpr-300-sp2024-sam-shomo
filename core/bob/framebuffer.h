#include <glm/glm.hpp>

namespace bob {
	struct Framebuffer {
		unsigned int fbo;
		unsigned int colorBuffer[8];
		unsigned int depthBuffer;
		unsigned int width;
		unsigned int height;
	};
	Framebuffer createFramebuffer(unsigned int width, unsigned int height, int colorFormat);
}
