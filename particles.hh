// particles.hh
// Copyright (C) 2023 by Shawn Yarbrough

#pragma once

#include <cmath>
#include <future>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "csv_parser/csv_parser.h"

#include "randomize.hh"

constexpr float GRAVITY = 50.0F;
constexpr float SPIN = 37.0F;

using Collisions = std::unordered_map<size_t, std::unordered_set<size_t>>;

struct Particle;
using Particles = std::vector<Particle>;

struct Particle {
    size_t id{std::numeric_limits<size_t>::max()};
    glm::vec2 position{0, 0};
    glm::vec2 velocity{0, 0};
    glm::vec2 temporary_velocity{0, 0};
    float diameter{1};
    glm::vec4 color{1, 1, 1, 1};

    static inline glm::vec4 choose_color_from_size(float sz);
    static inline Particles init_particle_grid(size_t width, size_t height, int32_t radius, size_t max_velocity, size_t step);
    static inline void accelerate_particle(const Particle& ip1, const Particle& ip2, Particle& op1, const Particle& op2, Collisions& collisions, float delta);
    static inline Collisions accelerate_particle_block(const Particles& in_particles, Particles& out_particles, float delta, size_t block_size, size_t block_start);
    static inline Particles accelerate_particles(const Particles& in_particles, float delta);
    static inline void move_particles(Particles& particles, float delta);
    static inline void draw_particles(const Particles& particles, unsigned int shader_program);
};    // struct Particle

inline glm::vec4 Particle::choose_color_from_size(float sz) {
    if (sz <= 5) return glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    else if (sz <= 15) return glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    else if (sz <= 25) return glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    else if (sz <= 35) return glm::vec4(0.3f, 0.1f, 0.1f, 1.0f);
    else if (sz <= 45) return glm::vec4(0.4f, 0.1f, 0.1f, 1.0f);
    else if (sz <= 55) return glm::vec4(0.6f, 0.3f, 0.1f, 1.0f);
    else if (sz <= 75) return glm::vec4(0.8f, 0.4f, 0.1f, 1.0f);
    else if (sz <= 100) return glm::vec4(0.9f, 0.8f, 0.1f, 1.0f);
    else return glm::vec4(1.0f, 1.0f, 0.3f, 1.0f);
}

inline Particles Particle::init_particle_grid(size_t width, size_t height, int32_t radius, size_t max_velocity, size_t step) {
    Particles ret;
    ret.reserve(((width*height)/step)/step);

    Randomize rize1(-max_velocity, +max_velocity);    // particle velocities
    Randomize rize2(1, 3);    // particle sizes
    size_t next_id = 0;

    glm::vec2 center(0.0F, 0.0F);
    for (int32_t y = -radius; y < +radius; y += step) {
        for (int32_t x = -radius; x < +radius; x += step) {
            Particle p;
            p.id = next_id++;
            p.position = glm::vec2(x+0.5F, y+0.5F);
            glm::vec2 dcenter = center-p.position;
            float len = glm::length(dcenter);
            if (len > radius) continue;
            if (dcenter[0] != 0.0F || dcenter[1] != 0.0F) {
                auto ncenter = glm::normalize(dcenter);
                p.velocity = glm::vec2(-ncenter[1], ncenter[0]);
                p.velocity *= len/radius;
                p.velocity *= SPIN;
                p.velocity[0] += rize1.get();
                p.velocity[1] += rize1.get();
            } else {
                p.velocity = glm::vec2(0.0F, 0.0F);
            }
            p.diameter=rize2.get();
            p.color = Particle::choose_color_from_size(p.diameter);
            ret.push_back(p);
        }
    }
    return ret;
}

inline void Particle::accelerate_particle(const Particle& ip1, const Particle& ip2, Particle& op1, const Particle& op2, Collisions& collisions, float delta) {
    const float xdistance = ip2.position[0]-ip1.position[0];
    const float ydistance = ip2.position[1]-ip1.position[1];
    const float quadrance = (xdistance*xdistance)+(ydistance*ydistance);
    const float distance = sqrt(quadrance);

    // Collision detection.
    float r1 = ip1.diameter/2.0F;
    float r2 = ip2.diameter/2.0F;
    // For simplicity, the mass is assumed to equal the area of the particle. (A=pi*r^2)
    const float mass1 = glm::pi<float>()*r1*r1;
    const float mass2 = glm::pi<float>()*r2*r2;

    if (distance <= r1+r2) {
        // Collision.
        // Two particles that are touching each other will move in the same direction.
        // TODO: Note that a chain of touching particles won't sync up correctly.
        // Calculate the weighted average of the velocities of the two particles.
        // op1.velocity = /*op2.velocity =*/ ((mass1*ip1.velocity)+(mass2*ip2.velocity))/(mass1+mass2);
        collisions[op1.id].insert(op2.id);
    } else {
        // Apply the acceleration from the force felt between two particles.
        const float quadrance2 = std::max(quadrance, 3.0F);    // Don't divide by a number too close to zero.
        const float gforce = GRAVITY*(mass1*mass2)/quadrance2;    // TODO
        const float gacceleration1 = gforce/mass1;
        const float gacceleration2 = -(gforce/mass2);

        const float xacceleration1 = (gacceleration1*xdistance)/distance;
        const float yacceleration1 = (gacceleration1*ydistance)/distance;
        const float xacceleration2 = (gacceleration2*xdistance)/distance;
        const float yacceleration2 = (gacceleration2*ydistance)/distance;

        op1.velocity[0] += xacceleration1*delta;
        op1.velocity[1] += yacceleration1*delta;
        // op2.velocity[0] += xacceleration2*delta;
        // op2.velocity[1] += yacceleration2*delta;
    }
}

inline Collisions Particle::accelerate_particle_block(const Particles& in_particles, Particles& out_particles, float delta, size_t block_size, size_t block_start) {
    Collisions collisions;
    for (size_t i1 = block_start; i1 < block_start+block_size && i1 < in_particles.size() && i1 < out_particles.size(); ++i1) {
        const Particle& ip1 = in_particles[i1];
        Particle &op1 = out_particles[i1];
        for (size_t i2 = 0; i2 < in_particles.size() && i2 < out_particles.size(); ++i2) {
            if (i1 == i2) continue;
            const Particle& ip2 = in_particles[i2];
            Particle &op2 = out_particles[i2];
            accelerate_particle(ip1, ip2, op1, op2, collisions, delta);
        }
    }
    return collisions;
}

inline Particles Particle::accelerate_particles(const Particles& in_particles, float delta) {
    // auto ts1 = std::chrono::system_clock::now();
    Particles out_particles;
    out_particles.reserve(in_particles.size());

    for (const Particle& p : in_particles) {
        out_particles.emplace_back(p);
    }

    // Iterate over the set of particle pairs. O(n^2) time complexity because each particle must accelerate every other particle.
    size_t thread_count = std::thread::hardware_concurrency();
    size_t block_size = in_particles.size()/thread_count;
    if (in_particles.size()%thread_count != 0)
        ++block_size;
    Collisions collisions;
    std::vector<std::future<Collisions>> threads;
    threads.reserve(thread_count);
    auto ts2 = std::chrono::system_clock::now();
    for (size_t t = 0; t < thread_count; ++t) {
        threads.push_back(std::async(accelerate_particle_block, std::cref(in_particles), std::ref(out_particles), delta, block_size, t*block_size));
    }
    // auto ts3 = std::chrono::system_clock::now();
    // std::cout << "async time " << std::chrono::duration<double>(ts3-ts2).count() << "s" << std::endl;
    for (auto& f : threads) {
        Collisions collisions2 = f.get();
        for (const auto& item : collisions2) {
            const size_t& id1 = item.first;
            for (const size_t& id2 : item.second)
                collisions[id1].insert(id2);
        }
    }
    // auto ts4 = std::chrono::system_clock::now();
    // std::cout << "thread get time " << std::chrono::duration<double>(ts4-ts3).count() << "s" << std::endl;

    // Iterate over the set of collisions.
    // auto ts5 = std::chrono::system_clock::now();
    std::vector<uint8_t> deleted(out_particles.size(), false);
    size_t deleted_count = 0;
    while (!collisions.empty()) {
        // Locate and remove a connected set of touching particles.
        std::unordered_set<size_t> colliding_ids;
        std::queue<size_t> q;
        q.push(collisions.begin()->first);
        while (!q.empty()) {
            size_t id = q.front();
            q.pop();
            colliding_ids.insert(id);
            if (collisions.contains(id)) {
                for (size_t touching_id : collisions[id])
                    q.push(touching_id);
                collisions.erase(id);
            }
        }

        // Calculate the total mass, the center of mass position, and
        // the combined velocity vector, of the touching particles.
        float total_mass = 0.0F;
        glm::vec2 position{0.0F, 0.0F};
        glm::vec2 velocity{0.0F, 0.0F};
        for (size_t id : colliding_ids) {
            const Particle& ip = in_particles[id];
            float radius = ip.diameter/2.0F;
            const float mass = glm::pi<float>()*radius*radius;
            total_mass += mass;
            position += ip.position*mass;
            velocity += mass*ip.velocity;
        }
        position /= total_mass;
        velocity /= total_mass;
        for (size_t id : colliding_ids)
            deleted[id] = true;
        deleted[*colliding_ids.begin()] = false;
        deleted_count += colliding_ids.size()-1;
        Particle& op = out_particles[*colliding_ids.begin()];
        op.position = position;
        op.velocity = velocity;
        op.diameter = sqrt(total_mass/glm::pi<float>())*2.0F;    // A=pi*r^2
        op.color = Particle::choose_color_from_size(op.diameter);
    }

    // Remove any deleted particles and renumber the particle ID's.
    Particles temp_particles;
    temp_particles.reserve(out_particles.size()-deleted_count);
    size_t next_id = 0;
    for (const Particle& op : out_particles)
        if (!deleted[op.id]) {
            temp_particles.emplace_back(op);
            temp_particles.back().id = next_id++;
        }
    out_particles.swap(temp_particles);
    if (out_particles.size() != in_particles.size()) {
        std::cout << out_particles.size() << " particles" << std::endl;
    }

    // auto ts6 = std::chrono::system_clock::now();
    // std::cout << "collision time " << std::chrono::duration<double>(ts6-ts5).count() << "s" << std::endl;
    // std::cout << "acceleration time " << std::chrono::duration<double>(ts6-ts1).count() << "s\n" << std::endl;
    return out_particles;
}

inline void Particle::move_particles(Particles& particles, float delta) {
    for (auto& p : particles) {
        p.position[0] += ((p.velocity[0]+p.temporary_velocity[0]) * delta);
        p.position[1] += ((p.velocity[1]+p.temporary_velocity[1]) * delta);
        p.temporary_velocity = glm::vec2(0.0F, 0.0F);
    }
}

inline void Particle::draw_particles(const Particles& particles, unsigned int shader_program) {
    std::vector<GLfloat> memory;
    constexpr size_t stride = 7;    // The number of floats pushed in the following loop.
    memory.reserve(particles.size()*sizeof(GLfloat)*stride);
    for (const auto& p : particles) {
        memory.push_back(p.position[0]);
        memory.push_back(p.position[1]);
        memory.push_back(p.diameter);
        memory.push_back(p.color[0]);
        memory.push_back(p.color[1]);
        memory.push_back(p.color[2]);
        memory.push_back(p.color[3]);
    }

    // Vertex Array Object.
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Vertex Buffer Object.
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // Configure the VAO and VBO.
    glBufferData(GL_ARRAY_BUFFER, memory.size()*sizeof(GLfloat), &memory[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride*sizeof(GLfloat), (void*)(0*sizeof(GLfloat)));
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glDrawArrays(GL_POINTS, 0, memory.size()/stride);

    // Clean up the VAO and VBO.
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}
