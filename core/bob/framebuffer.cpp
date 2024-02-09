#include "framebuffer.h"
#include <glm/glm.hpp>
#include "external/glad.h"
#include "external/stb_image.h"

bob::Framebuffer bob::createFramebufferWithRBO(unsigned int width, unsigned int height, int colorFormat)
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

	//Render Buffer
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbo.width, fbo.height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	return fbo;
}

bob::Framebuffer bob::createFramebufferWithDepthBuffer(unsigned int width, unsigned int height, int colorFormat)
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

	return fbo;
}

bob::Framebuffer bob::createShadowMapFramebuffer(unsigned int width, unsigned int height, int colorFormat)
{
	unsigned int shadowMap;

	bob::Framebuffer fbo;
	fbo.width = width;
	fbo.height = height;

	glGenFramebuffers(1, &fbo.depthBuffer); //Create new frame buffer object
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.depthBuffer); // Bind the new FBO as the active one

	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT16, GL_FLOAT, 0);


	glTexStorage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 2048, 2048);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::printf("Error building Framebuffer!\n");
	}

	return fbo;
}
