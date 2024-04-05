#include <stdio.h>
#include <math.h>
#include <vector>

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

struct Node {
	glm::mat4 localTransform;
	glm::mat4 globalTransform;
	unsigned int parentIndex;
	ew::Transform trans;
};

struct Hierarchy {
	std::vector<Node*> nodes;
	//unsigned int nodeCount;
};
void SolveFK(Hierarchy h);

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
bob::Framebuffer shadowMap;
int main() {
	GLFWwindow* window = initWindow("Assignment 1", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	shadowMap = bob::createShadowMapFramebuffer(shadowWidth, shadowHeight, GL_RGB16F);
	bob::Framebuffer framebuffer = bob::createFramebufferWithRBO(screenWidth, screenHeight, GL_RGB16F);
	ew::Shader ppShader = ew::Shader("assets/pp.vert", "assets/pp.frag");
	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader shadowShader = ew::Shader("assets/shadow.vert", "assets/shadow.frag");
	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));
	ew::Transform planeTrans;
	planeTrans.position = glm::vec3(0, -1, 0);
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");

	camera.position = glm::vec3(0.0f, 10.0f, 18.0f);
	//camera.target = glm::vec3(0.0f, 10.0f, 15.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	light.target = glm::vec3(0.0f, 0.0f, 0.0f);
	light.position = light.target - flashlight.dir * 5.0f;// -flashlight.dir * 5.0f;
	light.orthographic = true;
	light.orthoHeight = 5.0f;
	light.nearPlane = 0.01f;
	light.farPlane = 20.0f;
	light.aspectRatio = 1;

	//Animation Stuff
	Hierarchy hier;

	Node head;
	Node torso;
	Node lShoulder;
	Node lElbow;
	Node lHand;
	Node rShoulder;
	Node rElbow;
	Node rHand;
	
	head.trans.position = glm::vec3(0, 2.2, 0);
	head.trans.scale = glm::vec3(0.7, 0.7, 0.7);

	torso.trans.position = glm::vec3(0, 1, 0);
	torso.trans.scale = glm::vec3(2, 2, 2);

	lShoulder.trans.position = glm::vec3(-1.3, 1, 0);
	lShoulder.trans.scale = glm::vec3(0.7, 0.7, 0.7);

	rShoulder.trans.position = glm::vec3(1.3, 1, 0);
	rShoulder.trans.scale = glm::vec3(0.7, 0.7, 0.7);

	lElbow.trans.position = glm::vec3(-1.3, 0, 0);

	rElbow.trans.position = glm::vec3(1.3, 0, 0);

	lHand.trans.position = glm::vec3(-1, -1, 0);
	lHand.trans.scale = glm::vec3(0.7, 0.7, 0.7);

	rHand.trans.position = glm::vec3(1, -1, 0);
	rHand.trans.scale = glm::vec3(0.7, 0.7, 0.7);

	hier.nodes.push_back(&torso);  //0
	hier.nodes.push_back(&head); // 1
	hier.nodes.push_back(&lShoulder); //2
	hier.nodes.push_back(&lElbow); //3
	hier.nodes.push_back(&lHand); //4
	hier.nodes.push_back(&rShoulder); //5
	hier.nodes.push_back(&rElbow); //6
	hier.nodes.push_back(&rHand); //7

	torso.parentIndex = -1;
	head.parentIndex = 0;

	lShoulder.parentIndex = 0;
	lElbow.parentIndex = 2;
	lHand.parentIndex = 3;

	rShoulder.parentIndex = 0;
	rElbow.parentIndex = 5;
	rHand.parentIndex = 6;


	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing

	GLuint brickTexture = ew::loadTexture("assets/foil_normal_gl.jpg");
	GLuint colorTexture = ew::loadTexture("assets/foil_color.jpg");

	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		//head.trans.position = glm::vec3(0, sin(deltaTime * 2) * 5, 0);
		lShoulder.trans.rotation = glm::rotate(lShoulder.trans.rotation, deltaTime / 2, glm::vec3(0.2, 0, 0.0));
		rShoulder.trans.rotation = glm::rotate(rShoulder.trans.rotation, -deltaTime / 2, glm::vec3(0.2, 0, 0.0));

		lElbow.trans.rotation = glm::rotate(lElbow.trans.rotation, deltaTime, glm::vec3(0, 0, 0.5));
		rElbow.trans.rotation = glm::rotate(rElbow.trans.rotation, -deltaTime, glm::vec3(0, 0, 0.5));

		lHand.trans.rotation = glm::rotate(lHand.trans.rotation, deltaTime * 2, glm::vec3(0, 0.5, 0));
		rHand.trans.rotation = glm::rotate(rHand.trans.rotation, -deltaTime * 2, glm::vec3(0, 0.5, 0));

		for each (Node *node in hier.nodes)
		{
			node->localTransform = node->trans.modelMatrix();
		}

		SolveFK(hier);

		cameraController.move(window, &camera, deltaTime);

		light.position = light.target - flashlight.dir * 5.0f;
		glCullFace(GL_FRONT);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.fbo);
		glBindTexture(GL_TEXTURE_2D, shadowMap.depthBuffer);
		glViewport(0, 0, shadowWidth, shadowHeight);

		shadowShader.use();
		shadowShader.setMat4("_ViewProjection", light.projectionMatrix() * light.viewMatrix());
		shadowShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw(); //Draws monkey model using current shader


		glCullFace(GL_BACK);
		glBindTextureUnit(0, brickTexture);
		glBindTextureUnit(1, colorTexture);
		glBindTextureUnit(2, shadowMap.depthBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		glViewport(0, 0, screenWidth, screenHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);

		shader.use();

		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setVec3("_EyePos", camera.position);
		//shader.setMat4("_LightSpaceMatrix", light.projectionMatrix() * light.viewMatrix());
		//shader.setVec3("_LightDirection", flashlight.dir);

		shader.setInt("normalMap", 0);
		shader.setInt("_MainTex", 1);
		shader.setInt("shadowMap", 2);

		shader.setVec3("_LightColor", flashlight.color);
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("minBias", minBias);
		shader.setFloat("maxBias", maxBias);
		shader.setFloat("_Material.Shininess", material.Shininess);

		//Draws Head
		shader.setMat4("_Model", head.globalTransform);
		monkeyModel.draw(); 
		torso.trans.rotation = glm::rotate(torso.trans.rotation, deltaTime, glm::vec3(0.0, 0.5, 0.0));
		//Draws Torso
		shader.setMat4("_Model", torso.globalTransform);
		monkeyModel.draw(); 
		//Draw Left Shoulder
		shader.setMat4("_Model", lShoulder.globalTransform);
		monkeyModel.draw(); 
		//Draw Right Shoulder
		shader.setMat4("_Model", rShoulder.globalTransform);
		monkeyModel.draw();
		//Draw Left Elbow
		shader.setMat4("_Model", lElbow.globalTransform);
		monkeyModel.draw();
		//Draw Right Elbow
		shader.setMat4("_Model", rElbow.globalTransform);
		monkeyModel.draw();
		//Draw Left Hand
		shader.setMat4("_Model", lHand.globalTransform);
		monkeyModel.draw();
		//Draw Right Hand
		shader.setMat4("_Model", rHand.globalTransform);
		monkeyModel.draw();

		cameraController.move(window, &camera, deltaTime);
		//headTrans.position = glm::translate(head.localTransform, glm::vec3(0.5f, 0.5, 0));

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
	ImVec2 windowSize = ImGui::GetWindowSize();

	ImGui::Image((ImTextureID)shadowMap.depthBuffer, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void SolveFK(Hierarchy hier)
{
	for each (Node * node in hier.nodes)
	{
		if (node->parentIndex == -1)
			node->globalTransform = node->localTransform;
		else
			node->globalTransform = hier.nodes[node->parentIndex]->globalTransform * node->localTransform;
	}
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

