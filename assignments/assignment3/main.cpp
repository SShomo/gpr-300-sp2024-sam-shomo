#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include <ew/procGen.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <bob/framebuffer.h>

void resetCamera(ew::Camera* camera, ew::CameraController* controller);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

//Global state
ew::Transform monkeyTransform;
ew::Camera light;
ew::Camera camera;
ew::CameraController cameraController;

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

struct Flashlight {
	glm::vec3 dir = glm::vec3(0.005f, -1.0f, 0.0f);
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
}flashlight;

struct PointLight {
	glm::vec3 position;
	float radius;
	glm::vec4 color;
};

const int MAX_POINT_LIGHTS = 64;
PointLight pointLights[MAX_POINT_LIGHTS];

float blurEffect = 1.0f;
int kernal;
float minBias = 0.002;
float maxBias = 0.02;

int screenWidth = 1080;
int screenHeight = 720;

int shadowWidth = 2048;
int shadowHeight = 2048;
float prevFrameTime;
float gamma = 1.0f;
float deltaTime;
bob::Framebuffer gBuffer;
bob::Framebuffer shadowMap;

int main() {
	GLFWwindow* window = initWindow("Assignment 1", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	shadowMap = bob::createShadowMapFramebuffer(shadowWidth, shadowHeight, GL_RGB16F);
	gBuffer = bob::createGBuffer(screenWidth, screenHeight);
	bob::Framebuffer framebuffer = bob::createFramebufferWithRBO(screenWidth, screenHeight, GL_RGB16F);

	ew::Shader geoShader = ew::Shader("assets/lit.vert", "assets/geoPass.frag");
	ew::Shader ppShader = ew::Shader("assets/pp.vert", "assets/pp.frag");
	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader defShader = ew::Shader("assets/deferredLit.vert", "assets/deferredLit.frag");
	ew::Shader lightOrbShader = ew::Shader("assets/lightOrb.vert", "assets/lightOrb.frag");

	ew::Shader shadowShader = ew::Shader("assets/shadow.vert", "assets/shadow.frag");
	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(50, 50, 5));
	ew::Mesh sphereMesh = ew::Mesh(ew::createSphere(1.0f, 8));

	ew::Transform planeTrans;
	planeTrans.position = glm::vec3(0, -1, 0);
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");

	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	light.target = glm::vec3(0.0f, 0.0f, 0.0f);
	light.position = light.target - flashlight.dir * 5.0f;// -flashlight.dir * 5.0f;
	light.orthographic = true;
	light.orthoHeight = 5.0f;
	light.nearPlane = 0.01f;
	light.farPlane = 20.0f;
	light.aspectRatio = 1;

	for (int i = 0; i < 64; i++)
	{
		pointLights[i].position = glm::vec3(i, 0, i / 2);
		pointLights[i].radius = 1;
		//pointLights[i].color = glm::vec4(1 / (float)(rand() * 255), 1 / (rand() * 255), 1 / (rand() * 255), 1);
		pointLights[i].color = glm::vec4(1.0f, 0.5f, 0.31f, 1.0f);
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing

	GLuint colorTexture = ew::loadTexture("assets/foil_color.jpg");
	GLuint normalTexture = ew::loadTexture("assets/foil_normal_gl.jpg");

	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();
		shader.use();
		shader.setInt("_MainTex", 0);
		shader.setInt("normalMap", 1);
		shader.setInt("shadowMap", 2);

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		//monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));
		cameraController.move(window, &camera, deltaTime);

		light.position = light.target - flashlight.dir * 5.0f;

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		glViewport(0, 0, screenWidth, screenHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		defShader.use();

		for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
			//Creates prefix "_PointLights[0]." etc
			std::string prefix = "_PointLights[" + std::to_string(i) + "].";
			defShader.setVec3(prefix + "position", pointLights[i].position);
			defShader.setVec3(prefix + "color", pointLights[i].color);
			defShader.setFloat(prefix + "radius", pointLights[i].radius);
		}

		defShader.setVec3("_EyePos", camera.position);
		defShader.setFloat("_Material.Ka", material.Ka);
		defShader.setFloat("_Material.Kd", material.Kd);
		defShader.setFloat("_Material.Ks", material.Ks);

		defShader.setFloat("_Material.Shininess", material.Shininess);
		//defShader.setMat4("_Model", planeTrans.modelMatrix());
		//planeMesh.draw();

		//defShader.setMat4("_Model", monkeyTransform.modelMatrix());
		//monkeyModel.draw(); //Draws monkey model using current shader

		glBindTextureUnit(0, gBuffer.colorBuffer[0]);
		glBindTextureUnit(1, gBuffer.colorBuffer[1]);
		glBindTextureUnit(2, gBuffer.colorBuffer[2]);
		glBindTextureUnit(3, shadowMap.depthBuffer);

		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
		glViewport(0, 0, gBuffer.width, gBuffer.height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		geoShader.use();
		glBindTextureUnit(0, colorTexture);
		glBindTextureUnit(1, normalTexture);

		geoShader.setInt("_MainTex", 0);
		geoShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		geoShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw(); //Draws monkey model using current shader
		geoShader.setMat4("_Model", planeTrans.modelMatrix());
		planeMesh.draw();

		//glDisable(GL_DEPTH_TEST); //Depth testing
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.fbo); //Read from gBuffer 
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.fbo); //Write to current fbo
		glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glCullFace(GL_FRONT);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.fbo);
		glBindTexture(GL_TEXTURE_2D, shadowMap.depthBuffer);
		glViewport(0, 0, shadowWidth, shadowHeight);
		glClear(GL_DEPTH_BUFFER_BIT);


		glEnable(GL_DEPTH_TEST); //Depth testing
		
		shadowShader.use();
		shadowShader.setMat4("_ViewProjection", light.projectionMatrix() * light.viewMatrix());
		shadowShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw(); //Draws monkey model using current shader

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		glBindTextureUnit(0, colorTexture);
		glBindTextureUnit(1, normalTexture);
		glBindTextureUnit(2, shadowMap.depthBuffer);

		glViewport(0, 0, screenWidth, screenHeight);
		glClear(GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_BACK);

		shader.use();
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setVec3("_EyePos", camera.position);
		shader.setMat4("_LightSpaceMatrix", light.projectionMatrix() * light.viewMatrix());
		shader.setVec3("_LightDirection", flashlight.dir);

		shader.setVec3("_LightColor", flashlight.color);
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("minBias", minBias);
		shader.setFloat("maxBias", maxBias);
		shader.setFloat("_Material.Shininess", material.Shininess);
		shader.setMat4("_Model", planeTrans.modelMatrix());
		planeMesh.draw();

		shader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw(); //Draws monkey model using current shader
		cameraController.move(window, &camera, deltaTime);
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 0.5, 0.0));


		lightOrbShader.use();
		lightOrbShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			glm::mat4 m = glm::mat4(1.0f);
			m = glm::translate(m, pointLights[i].position);
			m = glm::scale(m, glm::vec3(0.2f)); //Whatever radius you want

			lightOrbShader.setMat4("_Model", m);
			lightOrbShader.setVec3("_Color", pointLights[i].color);
			sphereMesh.draw();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ppShader.use();
		ppShader.setFloat("_Blur", blurEffect);
		ppShader.setFloat("_gamma", gamma);
		ppShader.setInt("_Kernal", kernal);

		glBindTextureUnit(0, framebuffer.colorBuffer[0]);
		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);

	}

	if (ImGui::CollapsingHeader("Image Convolution")) {
		ImGui::SliderInt("Effect", &kernal, 0.0f, 3.0f); //0 will give the blur effect, 1 will give the sharpen effect, 2 will give edge effect, 3 will give Gaussian blur
		ImGui::SliderFloat("Intensity", &blurEffect, 0.0f, 8.0f);
		ImGui::SliderFloat("Gamma Correction", &gamma, 0.0f, 8.0f);

	}

	if (ImGui::CollapsingHeader("Lighting"))
	{
		ImGui::SliderFloat("Light Direction X", &flashlight.dir.x, -5.0f, 5.0f);
		ImGui::SliderFloat("Light Direction Y", &flashlight.dir.y, -5.0f, 5.0f);
		ImGui::SliderFloat("Light Direction Z", &flashlight.dir.z, -5.0f, 5.0f);
		ImGui::ColorEdit3("Light Color", &flashlight.color.r);

		ImGui::SliderFloat("minBias", &minBias, 0.0f, 1.0f);
		ImGui::SliderFloat("maxBias", &maxBias, 0.0f, 1.0f);
	}


	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}
	ImGui::End();

	ImGui::Begin("Shadow Map");
	ImGui::BeginChild("Shadow Map");

	ImGui::Begin("GBuffers"); {
		ImVec2 texSize = ImVec2(gBuffer.width / 4, gBuffer.height / 4);
		for (size_t i = 0; i < 3; i++)
		{
			ImGui::Image((ImTextureID)gBuffer.colorBuffer[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
		}}
	ImGui::End();


	ImVec2 windowSize = ImGui::GetWindowSize();

	ImGui::Image((ImTextureID)shadowMap.depthBuffer, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

