// gravity_simulation.cu
// Copyright (C) 2023 by Shawn Yarbrough

// #include <cstdio>
// __global__ void cuda_hello(){
//     printf("Hello World from GPU!\n");
// }

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

using namespace std::literals;

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#define GLFW_DLL
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "particles.hh"
#include "utility.hh"

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

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

const char *fragment_shader_text =
    "#version 330 core\n"
    "in vec4 starcolor;\n"
    "out vec4 fragcolor;\n"
    "void main()\n"
    "{\n"
    "    vec2 coord = gl_PointCoord - vec2(0.5);\n"
    "    if (length(coord) > 0.5) discard;\n"
    "    fragcolor = starcolor;\n"
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

Particles load_particles_from_csv(const std::string& csv_filename) {
    Particles particles;

    Csv::Parser csv;
    std::vector<std::vector<Csv::CellReference>> cells;
    std::ifstream ifile(csv_filename, std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(ifile)), (std::istreambuf_iterator<char>()));
    csv.parseTo(data, cells);

    if (cells.size() > 0) {
        std::vector<std::string> headings;
        size_t cols = cells.size();
        size_t rows = cells[0].size();
        for (std::size_t col = 0; col < cols; ++col) {
            if (cells[col].size() != rows)
                throw std::runtime_error(".csv column #"+std::to_string(col+1)+" unexpected size");
            const auto& cell = cells[col][0];
            if (cell.getType() != Csv::CellType::String)
                throw std::runtime_error("unexpected type for string heading column #"+std::to_string(col+1));
            std::optional<std::string> s = cell.getCleanString().value();
            headings.push_back(utility::strip(s.value_or("")));
        }
        size_t next_id = 0;
        for (std::size_t row = 1; row < rows; ++row) {
            Particle p;
            p.id = next_id++;
            for (std::size_t col = 0; col < cols; ++col) {
                const auto& cell = cells[col][row];
                if (cell.getType() != Csv::CellType::Double)
                    throw std::runtime_error("unexpected type for number in column #"+std::to_string(col+1)+" row #"+std::to_string(row+1));
                double d = cell.getDouble().value();
                const std::string& heading = headings[col];
                if (heading == "xposition")
                    p.position[0] = d;
                else if (heading == "yposition")
                    p.position[1] = d;
                else if (heading == "xvelocity")
                    p.velocity[0] = d;
                else if (heading == "yvelocity")
                    p.velocity[1] = d;
                else if (heading == "diameter")
                    p.diameter = d;
                else
                    throw std::runtime_error("unexpected name for .csv col #"+std::to_string(col+1)+": "+heading);
            }
            particles.push_back(std::move(p));
        }
    }

    return particles;
}

int main2(int argc, char* argv[]) {
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
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(static_cast<float>(SCR_WIDTH)/2.0F, static_cast<float>(SCR_HEIGHT)/2.0F, 0.0F));
    // projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    projection = glm::ortho(0.0F, (float)SCR_WIDTH, 0.0F, (float)SCR_HEIGHT, -1.0F, +1.0F);
    // view       = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, &view[0][0]);

    Particles particles;
    if (argc > 1) {
        particles = load_particles_from_csv(argv[1]);
    } else if (particles.empty()) {
        particles = init_particle_grid(SCR_WIDTH, SCR_HEIGHT, /*radius=*/1000, /*max_velocity=*/10, /*step=*/20);
    }
    std::cout << particles.size() << " particles" << std::endl;

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
        if (delta > 0.2) {
            std::cout << std::fixed << delta << "s hitch" << std::endl;
            delta = 0.2;
        }
        particles = accelerate_particles(particles, delta);
        move_particles(particles, delta);
        draw_particles(particles, shader_program);

        glfwSwapBuffers(window);
        glfwPollEvents();

        ts1 = std::move(ts2);
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    try {
        return main2(argc, argv);
    } catch(const std::exception& err) {
        std::cout << "EXCEPTION: " << err.what() << std::endl;
        return 1;
    } catch(...) {
        std::cout << "UNKNOWN EXCEPTION" << std::endl;
        return 2;
    }
}
