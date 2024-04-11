#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

const unsigned int SCR_WIDTH = 1300;
const unsigned int SCR_HEIGHT = 900;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct Object {
    glm::vec3 position;
    float scale;
    float rotationX = 0;
    float rotationY = 0;
    float rotationZ = 0;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    Object island;
    Object spyro;
    Object portal;
    Object portalWater;
    Object key;
    Object chest;
    Object diamond;

    DirLight dirLight;
    PointLight pointLight;
    SpotLight spotLight;

    glm::vec3 sAmbient = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 sDiffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 sSpecular = glm::vec3( 1.0f, 1.0f, 1.0f);

    ProgramState()
            : camera(glm::vec3(14.4107f, 0.438836f, 19.0486f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void processLamp(GLFWwindow *window, SpotLight& spotLight);
void renderModel(glm::mat4& model, Object& object);
unsigned int loadTexture(char const * path);
unsigned int loadCubemap(vector<std::string> faces);

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //SHADERS::
    Shader ourShader("resources/shaders/light.vs", "resources/shaders/light.fs");
    Shader cubemapShader("resources/shaders/cubemap.vs", "resources/shaders/cubemap.fs");

    //MODELS:
    //ISLAND:
    Model islandModel("resources/objects/island/island.obj");
    islandModel.SetShaderTextureNamePrefix("material.");
    Object& islandObj = programState->island;
    islandObj.position = glm::vec3(12.0f, 0.0f, 1.0f);
    islandObj.scale = 0.2f;

    //SPYRO
    Model spyroModel("resources/objects/spyro/spyro.obj");
    spyroModel.SetShaderTextureNamePrefix("material.");
    Object& spyroObj = programState->spyro;
    spyroObj.position = glm::vec3(15.498f, -1.85f, 5.23524f);
    spyroObj.scale = 1.0f;
    spyroObj.rotationY = -85.0f;

    //PORTAL:
    Model portalModel("resources/objects/portal/portal.obj");
    portalModel.SetShaderTextureNamePrefix("material.");
    Object& portalObj = programState->portal;
    portalObj.position = glm::vec3 (17.13f, -1.94783f, 6.77324f);
    portalObj.scale = 0.04f;
    portalObj.rotationY = 55.0f;

    //KEY:
    Model keyModel("resources/objects/old_key/old_key.obj");
    keyModel.SetShaderTextureNamePrefix("material.");
    Object& keyObj = programState->key;
    keyObj.position = glm::vec3(8.97785f, -0.11684f, 1.30846f);
    keyObj.scale = 0.05f;
    keyObj.rotationX = 55.0;

    //CHEST:
    Model chestModel("resources/objects/chest/chest.obj");
    chestModel.SetShaderTextureNamePrefix("material.");
    Object& chestObj = programState->chest;
    chestObj.position = glm::vec3 (10.9758f, 0.222281f, -0.0916667f);
    chestObj.scale = 0.08f;
    chestObj.rotationY = 150.0f;

    //DIAMONDS:
    Model diamondModel("resources/objects/diamond/diamond.obj");
    diamondModel.SetShaderTextureNamePrefix("material.h");
    Object& diamondObj = programState->diamond;
    diamondObj.scale = 0.002f;
    const unsigned int diamondNumber = 10;
    vector<glm::vec3> diamondPositions = {
            glm::vec3 (10.5225f, -0.873134f, 5.12017f),
            glm::vec3 (10.1371f, -0.873134f, 5.24705f),
            glm::vec3 (9.76582f, -0.873134f, 5.18574f),
            glm::vec3 (8.45477f, -0.37f, 1.93658f),
            glm::vec3 (8.51452f, -0.37f, 2.37812f),
            glm::vec3 (12.587f, 0.18f, 3.19113f),
            glm::vec3 (12.1414f, 0.18f, 3.12796f),
            glm::vec3 (12.9761f, 0.18f, 2.97769f),
            glm::vec3 (13.8174f, -0.09f, -0.0203901f),
            glm::vec3 (14.0017f, -0.09f, 0.311945f)
    };

    //PORTAL WATER:
    float portalVertices[] = {
            //   x         y         z          normal                               texture coords
            0.4f,  0.5f,  0.0f,   0.0f, 0.0f, 1.0f,         1.0f, 1.0f,
            0.4f,  -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
            -0.4f,-0.5f,0.0f,   0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
            -0.4f,0.5f, 0.0f,   0.0f, 0.0f, 1.0f,       0.0f, 1.0f
    };
    unsigned int portalIndices[] = {
            0, 3, 1,
            1, 3, 2
    };
    unsigned int portalVBO, portalVAO, portalEBO;
    glGenVertexArrays(1, &portalVAO);
    glGenBuffers(1, &portalVBO);
    glGenBuffers(1, &portalEBO);

    glBindVertexArray(portalVAO);

    glBindBuffer(GL_ARRAY_BUFFER, portalVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(portalVertices), portalVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, portalEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(portalIndices), &portalIndices, GL_STATIC_DRAW);

    //position coords:
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //normal coords:
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    //texture coords:
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    unsigned int diffuseMap = loadTexture("resources/textures/portal_textures/water.jpg");
    unsigned int specularMap = loadTexture("resources/textures/portal_textures/specular_map.jpg");

    Object& portalWaterObj = programState->portalWater;
    portalWaterObj.position = glm::vec3(17.2534f, -0.958174f, 6.71231f);
    portalWaterObj.scale = 1.1f;
    portalWaterObj.rotationY = 45.0f;

    //CUBEMAP:
    float cubemapVertices[] = {
            -1.0f,  1.0f,   -1.0f,
            -1.0f,  -1.0f,  -1.0f,
            1.0f,   -1.0f,  -1.0f,
            1.0f,   -1.0f, -1.0f,
            1.0f,  1.0f,  -1.0f,
            -1.0f, 1.0f,  -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f,  -1.0f,
            -1.0f, 1.0f,  -1.0f,
            -1.0f, 1.0f,  1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f,  -1.0f, -1.0f,
            1.0f,  -1.0f, 1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  -1.0f,
            1.0f,  -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f,  -1.0f,
            1.0f,  1.0f,  -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f, 1.0f,  1.0f,
            -1.0f, 1.0f,  -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f,  -1.0f, -1.0f,
            1.0f,  -1.0f,-1.0f,
            -1.0f,-1.0f,1.0f,
            1.0f, -1.0f,1.0f
    };
    vector<std::string> faces {
            "resources/textures/cubemap/cloudtop_right.jpg",
            "resources/textures/cubemap/cloudtop_left.jpg",
            "resources/textures/cubemap/cloudtop_top.jpg",
            "resources/textures/cubemap/cloudtop_bottom.jpg",
            "resources/textures/cubemap/cloudtop_front.jpg",
            "resources/textures/cubemap/cloudtop_back.jpg"
    };
    unsigned int cubemapVAO, cubemapVBO;
    glGenVertexArrays(1, &cubemapVAO);
    glGenBuffers(1, &cubemapVBO);

    glBindVertexArray(cubemapVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubemapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubemapVertices), &cubemapVertices, GL_STATIC_DRAW);

    //position coords:
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int cubemapTexture = loadCubemap(faces);
    cubemapShader.use();
    cubemapShader.setInt("cubemap", 0);

    //LIGHTS:
    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3 (-0.00439722f, -0.544639f, -0.838659f);
    dirLight.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    dirLight.diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
    dirLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3 (10.9758f, 0.322281f, -0.0916667f);
    pointLight.ambient = glm::vec3(0.2, 0.2, 0.2);
    pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.7f;
    pointLight.quadratic = 1.8f;

    SpotLight& spotLight = programState->spotLight;
    spotLight.ambient = programState->sAmbient;
    spotLight.diffuse = programState->sDiffuse;
    spotLight.specular = programState->sSpecular;

    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;

    spotLight.cutOff = 12.5f;
    spotLight.outerCutOff = 15.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        processLamp(window, spotLight);
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::sort(diamondPositions.begin(), diamondPositions.end(),
                  [cameraPosition = programState->camera.Position](const glm::vec3& a, const glm::vec3& b) {
                      float d1 = glm::distance(a, cameraPosition);
                      float d2 = glm::distance(b, cameraPosition);
                      return d1 > d2;
                  });

        ourShader.use();

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        ourShader.setInt("transparency", 0);

        //SET LIGHTS:
        //DIRECTIONAL LIGHT:
        ourShader.setVec3("dirLight.direction", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);

        //POINTLIGHT:
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);

        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        //SPOTLIGHT:
        ourShader.setVec3("spotLight.position", programState->camera.Position);
        ourShader.setVec3("spotLight.direction", programState->camera.Front);

        ourShader.setVec3("spotLight.ambient", spotLight.ambient);
        ourShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        ourShader.setVec3("spotLight.specular", spotLight.specular);

        ourShader.setFloat("spotLight.constant", spotLight.constant);
        ourShader.setFloat("spotLight.linear", spotLight.linear);
        ourShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(spotLight.cutOff)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(spotLight.outerCutOff)));

        //TRANSFORMATIONS:
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        //RENDER ISLAND:
        glm::mat4 model = glm::mat4(1.0f);
        renderModel(model, islandObj);
        ourShader.setMat4("model", model);
        islandModel.Draw(ourShader);

        //RENDER SPYRO:
        renderModel(model, spyroObj);
        ourShader.setMat4("model", model);
        spyroModel.Draw(ourShader);

        //RENDER PORTAL:
        renderModel(model, portalObj);
        ourShader.setMat4("model", model);
        portalModel.Draw(ourShader);

        //RENDER KEY:
        renderModel(model, keyObj);
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3 (0.0f, 0.0f, 1.0f));
        ourShader.setMat4("model", model);
        keyModel.Draw(ourShader);

        //RENDER CHEST:
        renderModel(model, chestObj);
        ourShader.setMat4("model", model);
        chestModel.Draw(ourShader);

        //RENDER DIAMONDS:
        ourShader.setInt("transparency", 1);
        for(unsigned int i=0; i<diamondNumber; ++i) {
            diamondObj.position = diamondPositions[i];
            renderModel(model, diamondObj);
            model = glm::rotate(model, 2.0f * (float) glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
            ourShader.setMat4("model", model);
            diamondModel.Draw(ourShader);
        }
        ourShader.setInt("transparency", 0);

        //RENDER PORTAL WATER:
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW);
        glCullFace(GL_BACK);

        ourShader.setInt("transparency", 2);
        renderModel(model, portalWaterObj);
        ourShader.setMat4("model", model);
        ourShader.setInt("material.texture_diffuse1", 0);
        ourShader.setInt("material.texture_specular1", 1);
        ourShader.setFloat("material.shininess", 32);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);
        glBindVertexArray(portalVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ourShader.setInt("transparency", 0);
        glBindVertexArray(0);
        glDisable(GL_CULL_FACE);

        //CUBEMAP:
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        cubemapShader.use();
        view = glm::mat4 (glm::mat3 (programState->camera.GetViewMatrix()));
        cubemapShader.setMat4("view", view);
        cubemapShader.setMat4("projection", projection);

        glBindVertexArray(cubemapVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    glDeleteVertexArrays(1, &portalVAO);
    glDeleteVertexArrays(1, &cubemapVAO);
    glDeleteBuffers(1, &portalVBO);
    glDeleteBuffers(1, &cubemapVBO);
    glDeleteBuffers(1, &portalEBO);
    glDeleteShader(ourShader.ID);
    glDeleteShader(cubemapShader.ID);
    glDeleteTextures(1, &diffuseMap);
    glDeleteTextures(1, &specularMap);
    glDeleteTextures(1, &cubemapTexture);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP))
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN))
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT))
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT))
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

void processLamp(GLFWwindow *window, SpotLight& spotLight){
    if(glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) {
        spotLight.ambient = glm::vec3(0.0f);
        spotLight.diffuse = glm::vec3(0.0f);
        spotLight.specular = glm::vec3(0.0f);
    }
    else if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
            spotLight.ambient = programState->sAmbient;
            spotLight.diffuse = programState->sDiffuse;
            spotLight.specular = programState->sSpecular;
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void renderModel(glm::mat4& model, Object& object){
    model = glm::mat4 (1.0f);
    model = glm::translate(model, object.position);
    model = glm::scale(model, glm::vec3 (object.scale));
    if(object.rotationX != 0) {
        model = glm::rotate(model, glm::radians(object.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if(object.rotationY != 0) {
        model = glm::rotate(model, glm::radians(object.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if(object.rotationZ != 0) {
        model = glm::rotate(model, glm::radians(object.rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
    }
}

unsigned int loadTexture(char const * path){
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data){
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else{
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces){
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for(unsigned int i=0; i<faces.size(); i++){

        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if(data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else{
            std::cout << "Cubemap failed to load at path: " << faces[i] << '\n';
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
