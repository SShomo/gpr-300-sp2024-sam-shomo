#include "framebuffer.h"
#include <glm/glm.hpp>
#include "external/glad.h"
#include "external/stb_image.h"

bob::Framebuffer bob::createFramebuffer(unsigned int width, unsigned int height, int colorFormat)
{
	bob::Framebuffer fbo;
	fbo.width = width;
	fbo.height = height;

	glCreateFramebuffers(1, &fbo.fbo); //Create new frame buffer object
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo); // Bind the new FBO as the active one

	//Color Buffer

	glGenTextures(1, &fbo.colorBuffer[0]);
	glBindTexture(GL_TEXTURE_2D, fbo.colorBuffer[0]);
	glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, fbo.width, fbo.height);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo.colorBuffer[0], 0);


	//Depth Buffer
	glGenTextures(1, &fbo.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, fbo.depthBuffer);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, fbo.width, fbo.height);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo.depthBuffer, 0);

	//Render Buffer
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbo.width, fbo.height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	return fbo;
}
