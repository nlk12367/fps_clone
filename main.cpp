// Minimal FPS prototype made from scratch (no engine)
// Requires: OpenGL, GLFW, GLM (header-only)

// g++ main.cpp -lglfw -lGL -ldl -lX11 -lXxf86vm -lXrandr -lpthread -lXi -std=c++17 -o fps

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <cmath>

// ---------------------------- CAMERA ---------------------------------
glm::vec3 camPos(0.0f, 1.0f, 3.0f);
float yaw = -90.0f, pitch = 0.0f;
double lastX = 400, lastY = 300;
bool firstMouse = true;

glm::mat4 getViewMatrix() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::lookAt(camPos, camPos + glm::normalize(front), glm::vec3(0,1,0));
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos; 
    lastX = xpos; lastY = ypos;

    float sensitivity = 0.2f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw += xOffset;
    pitch += yOffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

// ---------------------------- ENEMIES ---------------------------------
struct Enemy {
    glm::vec3 pos;
    float speed = 1.0f;
    bool alive = true;
};
std::vector<Enemy> enemies;

void spawnWave(int count) {
    for (int i = 0; i < count; i++) {
        float x = (rand() % 20 - 10);
        float z = -20 - (rand() % 10);
        enemies.push_back({ glm::vec3(x,1,z) });
    }
}

// ---------------------------- SHADERS ---------------------------------
std::string loadFile(const char* path) {
    FILE* f = fopen(path, "rb");
    fseek(f,0,SEEK_END); long size = ftell(f); rewind(f);
    std::string s(size, '\0');
    fread(&s[0],1,size,f);
    fclose(f);
    return s;
}

GLuint compileShader(const char* path, GLenum type) {
    std::string src = loadFile(path);
    GLuint shader = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(shader,1,&c,NULL);
    glCompileShader(shader);

    int success; glGetShaderiv(shader,GL_COMPILE_STATUS,&success);
    if (!success) {
        char log[512]; glGetShaderInfoLog(shader,512,NULL,log);
        std::cout << "Shader err: " << log << "\n";
    }
    return shader;
}

GLuint makeShaderProgram() {
    GLuint vs = compileShader("shader.vert", GL_VERTEX_SHADER);
    GLuint fs = compileShader("shader.frag", GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glLinkProgram(prog);
    return prog;
}

// ---------------------------- MAIN ---------------------------------
int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "FPS Clone", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL();

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    GLuint shader = makeShaderProgram();

    float cubeVerts[] = {
        -0.5,-0.5,-0.5,   0.5,-0.5,-0.5,   0.5,0.5,-0.5,
        0.5,0.5,-0.5,   -0.5,0.5,-0.5,  -0.5,-0.5,-0.5,
        // (cube truncated – full cube requires all faces)
    };

    GLuint VAO,VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(cubeVerts),cubeVerts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);

    spawnWave(5);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_W)) camPos += glm::vec3(0,0,-0.1);
        if (glfwGetKey(window, GLFW_KEY_S)) camPos += glm::vec3(0,0, 0.1);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        glm::mat4 projection = glm::perspective(glm::radians(60.0f), 800/600.f, 0.1f, 100.f);
        glm::mat4 view = getViewMatrix();

        GLint locM = glGetUniformLocation(shader,"model");
        GLint locV = glGetUniformLocation(shader,"view");
        GLint locP = glGetUniformLocation(shader,"proj");
        glUniformMatrix4fv(locV,1,GL_FALSE,&view[0][0]);
        glUniformMatrix4fv(locP,1,GL_FALSE,&projection[0][0]);

        // ----------- Update + Draw Enemies -----------
        for (auto& e: enemies) {
            if (!e.alive) continue;
            glm::vec3 dir = glm::normalize(camPos - e.pos);
            e.pos += dir * e.speed * 0.01f;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), e.pos);
            glUniformMatrix4fv(locM,1,GL_FALSE,&model[0][0]);
            glDrawArrays(GL_TRIANGLES,0,36);

            if (glm::distance(e.pos, camPos) < 1.0f) {
                std::cout << "You died!\n";
                exit(0);
            }
        }

        // Respawn waves
        static float waveTimer = 0;
        waveTimer += 0.01f;
        if (waveTimer > 5) {
            spawnWave(5);
            waveTimer = 0;
        }

        glfwSwapBuffers(window);
    }

    return 0;
}
