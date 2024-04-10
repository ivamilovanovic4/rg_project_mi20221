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

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 1300;
const unsigned int SCR_HEIGHT = 900;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    glm::vec3 islandPosition = glm::vec3(12.0f, 0.0f, 1.0f);
    float islandScale = 0.2f;

    glm::vec3 spyroPosition = glm::vec3(15.498f, -1.85f, 5.23524f);
    float spyroScale = 1.0f;
    float spyroRotation = -85.0f;

    glm::vec3 portalPosition = glm::vec3 (17.13f, -1.94783f, 6.77324f);
    float portalScale = 0.04f;
    float portalRotation = 55.0f;

    glm::vec3 keyPosition = glm::vec3(8.97785f, -0.11684f, 1.30846f);
    float keyScale = 0.05f;
    float keyRotation = 55.0f;

    glm::vec3 chestPosition = glm::vec3 (10.9758f, 0.222281f, -0.0916667f);
    float chestScale = 0.08f;
    float chestRotate = 150.0f;

    unsigned int diamondNumber = 10;
    glm::vec3 diamondPosition = glm::vec3 (10.5021f, -0.873134f, 5.03972f);
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
    float diamondScale = 0.002f;


    PointLight pointLight;
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

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
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
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    //stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");

    // load models
    // -----------
    Model islandModel("resources/objects/island/island.obj");
    islandModel.SetShaderTextureNamePrefix("material.");

    Model spyroModel("resources/objects/spyro/spyro.obj");
    spyroModel.SetShaderTextureNamePrefix("material.");

    Model portalModel("resources/objects/portal/portal.obj");
    portalModel.SetShaderTextureNamePrefix("material.");

    Model keyModel("resources/objects/old_key/old_key.obj");
    keyModel.SetShaderTextureNamePrefix("material.");

    Model chestModel("resources/objects/chest/chest.obj");
    chestModel.SetShaderTextureNamePrefix("material.");

    Model diamondModel("resources/objects/diamond/diamond.obj");
    diamondModel.SetShaderTextureNamePrefix("material.h");
    ourShader.setBool("transparency", false);

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;



    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::sort(programState->diamondPositions.begin(), programState->diamondPositions.end(),
                  [cameraPosition = programState->camera.Position](const glm::vec3& a, const glm::vec3& b) {
                      float d1 = glm::distance(a, cameraPosition);
                      float d2 = glm::distance(b, cameraPosition);
                      return d1 > d2;
                  });

        ourShader.use();

        ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        ourShader.setVec3("dirLight.ambient", 0.5f, 0.5f, 0.5f);
        ourShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        ourShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model

        //ISLAND:
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,programState->islandPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3 (programState->islandScale));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        islandModel.Draw(ourShader);

        //SPYRO:
        model = glm::mat4 (1.0f);
        model = glm::translate(model, programState->spyroPosition);
        model = glm::scale(model, glm::vec3 (programState->spyroScale));
        model = glm::rotate(model, glm::radians(programState->spyroRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model);
        spyroModel.Draw(ourShader);

        //PORTAL:
        model = glm::mat4 (1.0f);
        model = glm::translate(model, programState->portalPosition);
        model = glm::scale(model, glm::vec3(programState->portalScale));
        model = glm::rotate(model, glm::radians(programState->portalRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model);
        portalModel.Draw(ourShader);

        //KEY:
        model = glm::mat4 (1.0f);
        model = glm::translate(model, programState->keyPosition);
        model = glm::scale(model, glm::vec3(programState->keyScale));
        model = glm::rotate(model, glm::radians(programState->keyRotation), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3 (0.0f, 0.0f, 1.0f));
        ourShader.setMat4("model", model);
        keyModel.Draw(ourShader);

        //CHEST:
        model = glm::mat4 (1.0f);
        model = glm::translate(model, programState->chestPosition);
        model = glm::scale(model, glm::vec3(programState->chestScale));
        model = glm::rotate(model, glm::radians(programState->chestRotate), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model);
        chestModel.Draw(ourShader);

        //Diamond:
        ourShader.setBool("transparency", true);
        for(unsigned int i=0; i<programState->diamondNumber; ++i) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, programState->diamondPositions[i]);
            model = glm::scale(model, glm::vec3(programState->diamondScale));
            model = glm::rotate(model, 2.0f * (float) glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
            ourShader.setMat4("model", model);
            diamondModel.Draw(ourShader);
        }
        ourShader.setBool("transparency", false);


        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
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
        ImGui::DragFloat3("Backpack position", (float*)&programState->islandPosition);
        ImGui::DragFloat("Backpack scale", &programState->islandScale, 0.05, 0.1, 4.0);

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
