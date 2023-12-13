// graphics.hh
// Copyright (C) 2023 by Shawn Yarbrough

#pragma once

#include <iostream>
#include <string>
#include <tuple>

using namespace std::literals;

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#define GLFW_DLL
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace graphics {

static inline void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static inline void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

inline const char *vertex_shader_text =
    "#version 330 core\n"
    "uniform mat4 model;"
    "uniform mat4 view;"
    "uniform mat4 projection;"
    "layout (location = 0) in vec2 pos;\n"
    "layout (location = 1) in float sz;\n"
    "out vec4 starcolor;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * view * model * vec4(pos, 0.0, 1.0);\n"
    "    gl_PointSize = sz;\n"
    "    if (sz <= 5) starcolor = vec4(0.1f, 0.1f, 0.1f, 1.0f);\n"
    "    else if (sz <= 15) starcolor = vec4(0.2f, 0.2f, 0.2f, 1.0f);\n"
    "    else if (sz <= 25) starcolor = vec4(0.3f, 0.3f, 0.3f, 1.0f);\n"
    "    else if (sz <= 35) starcolor = vec4(0.3f, 0.1f, 0.1f, 1.0f);\n"
    "    else if (sz <= 45) starcolor = vec4(0.4f, 0.1f, 0.1f, 1.0f);\n"
    "    else if (sz <= 55) starcolor = vec4(0.6f, 0.3f, 0.1f, 1.0f);\n"
    "    else if (sz <= 75) starcolor = vec4(0.8f, 0.4f, 0.1f, 1.0f);\n"
    "    else if (sz <= 100) starcolor = vec4(0.9f, 0.8f, 0.1f, 1.0f);\n"
    "    else starcolor = vec4(1.0f, 1.0f, 0.3f, 1.0f);\n"
    "}\n";

inline const char *fragment_shader_text =
    "#version 330 core\n"
    "in vec4 starcolor;\n"
    "out vec4 fragcolor;\n"
    "void main()\n"
    "{\n"
    "    vec2 coord = gl_PointCoord - vec2(0.5);\n"
    "    if (length(coord) > 0.5) discard;\n"
    "    fragcolor = starcolor;\n"
    "}\n";

inline unsigned int make_shader_program() {
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

inline std::tuple<GLFWwindow*, unsigned int> setup_app_window(const unsigned int SCR_WIDTH, const unsigned int SCR_HEIGHT) {
    GLFWwindow* window;
    unsigned int shader_program;

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

    shader_program = make_shader_program();
    glUseProgram(shader_program);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Matrixes
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(static_cast<float>(SCR_WIDTH)/2.0F, static_cast<float>(SCR_HEIGHT)/2.0F, 0.0F));
    projection = glm::ortho(0.0F, (float)SCR_WIDTH, 0.0F, (float)SCR_HEIGHT, -1.0F, +1.0F);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, &view[0][0]);

    return {window, shader_program};
}

inline void center_app_window(GLFWwindow* window, unsigned int shader_program) {
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, &model[0][0]);
}

}    // namespace graphics
