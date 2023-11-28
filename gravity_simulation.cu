// gravity_simulation.cu

// #include <cstdio>
// __global__ void cuda_hello(){
//     printf("Hello World from GPU!\n");
// }

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>

using namespace std::literals;

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#define GLFW_DLL
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "particles.hh"

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

const char *vertex_shader_text =
    "#version 330 core\n"
    "uniform mat4 model;"
    "uniform mat4 view;"
    "uniform mat4 projection;"
    "layout (location = 0) in vec2 pos;\n"
    "layout (location = 1) in float sz;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * vec4(pos, 0.0, 1.0);\n"
    "    gl_PointSize = sz;\n"
    "}\n";

const char *fragment_shader_text =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    vec2 coord = gl_PointCoord - vec2(0.5);\n"
    "    if (length(coord) > 0.5) discard;\n"
    "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n";

unsigned int make_shader_program() {
    // Vertex shader.
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    int success;
    char info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        throw std::runtime_error("compiling vertex shader failed\n"s + info_log);
    }

    // Fragment shader.
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        throw std::runtime_error("compiling fragment shader failed\n"s + info_log);
    }

    // Shader program.
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, GL_LINK_STATUS, &success, info_log);
        throw std::runtime_error("linking shaders failed\n"s + info_log);
    }

    return shader_program;
}

const unsigned int SCR_WIDTH = 1500;
const unsigned int SCR_HEIGHT = 1200;

int main2() {
    // cuda_hello<<<1,1>>>();

    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Gravity Simulation", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }


    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");

    glfwSwapInterval(1);

    unsigned int shader_program = make_shader_program();
    glUseProgram(shader_program);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Matrixes
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    // projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    projection = glm::ortho(0.0F, (float)SCR_WIDTH, 0.0F, (float)SCR_HEIGHT, -1.0F, +1.0F);
    // view       = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, &view[0][0]);

    Particles particles = init_particle_grid(SCR_WIDTH, SCR_HEIGHT, /*radius=*/250, /*max_velocity=*/10, /*step=*/14);
    std::cout << particles.size() << " particles" << std::endl;
    bool flip = false;

    auto ts1 = std::chrono::system_clock::now();
    auto ts2 = ts1;
    while (!glfwWindowShouldClose(window))
    {
        // float ratio;
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, &model[0][0]);

        ts2 = std::chrono::system_clock::now();
        double delta = std::chrono::duration<double>(ts2-ts1).count();
        if (delta == 0.0) throw std::runtime_error("zero time passed");
        if (delta > 0.5) {
            std::cout << std::fixed << delta << "s hitch" << std::endl;
            delta = 0.5;
        }
        accelerate_particles(particles, delta, flip);
        move_particles(particles, delta, flip);
        draw_particles(particles, shader_program);

        glfwSwapBuffers(window);
        glfwPollEvents();

        flip = !flip;
        ts1 = std::move(ts2);
    }

    glfwTerminate();
    return 0;
}

int main() {
    try {
        main2();
    } catch(const std::exception& err) {
        std::cout << "EXCEPTION: " << err.what() << std::endl;
    } catch(...) {
        std::cout << "UNKNOWN EXCEPTION" << std::endl;
    }
}
