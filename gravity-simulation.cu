// gravity_simulation.cu
// Copyright (C) 2023 by Shawn Yarbrough

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

using namespace std::literals;

#include "graphics.hh"
#include "particles.hh"
#include "utility.hh"

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

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
    auto [window, shader_program] = graphics::setup_app_window(SCR_WIDTH, SCR_HEIGHT);

    Particles particles;
    if (argc > 1) {
        particles = load_particles_from_csv(argv[1]);
    } else {
        particles = Particle::init_particle_grid(SCR_WIDTH, SCR_HEIGHT, /*radius=*/1000, /*max_velocity=*/10, /*step=*/20);
    }
    std::cout << particles.size() << " particles" << std::endl;

    auto ts1 = std::chrono::system_clock::now();
    auto ts2 = ts1;
    while (!glfwWindowShouldClose(window))
    {
        graphics::center_app_window(window, shader_program);

        ts2 = std::chrono::system_clock::now();
        double delta = std::chrono::duration<double>(ts2-ts1).count();
        if (delta == 0.0) throw std::runtime_error("zero time passed");
        if (delta > 0.2) {
            std::cout << std::fixed << delta << "s hitch" << std::endl;
            delta = 0.2;
        }
        particles = Particle::accelerate_particles(particles, delta);
        Particle::move_particles(particles, delta);
        Particle::draw_particles(particles, shader_program);

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
