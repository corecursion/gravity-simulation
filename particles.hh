// particles.hh

#pragma once

#include <cmath>
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "randomize.hh"

struct Particle {
    float sx1 = 0.0F;        // x position
    float sy1 = 0.0F;        // y position
    float vx = 0.0F;         // x velocity
    float vy = 0.0F;         // y velocity
    float d = 1.0F;          // diameter
    float sx2 = 0.0F;        // next x position
    float sy2 = 0.0F;        // next y position
    bool deleted = false;    // ignore this particle
};    // struct Particle

using Particles = std::vector<Particle>;

Particles init_particle_grid(size_t width, size_t height, size_t max_velocity, size_t step) {
    Particles ret;
    ret.reserve(((width*height)/step)/step);

    Randomize rize1(-max_velocity, +max_velocity);
    Randomize rize2(1, 3);

    for (size_t y = 0; y < height; y += step) {
        for (size_t x = 0; x < width; x += step) {
            Particle p;
            p.sx1=x+0.5F,
            p.sy1=y+0.5F,
            p.vx=rize1.get(),
            p.vy=rize1.get(),
            p.d=rize2.get(),
            p.sx2=p.sx1,
            p.sy2=p.sy1;
            p.deleted = false;
            ret.push_back(p);
        }
    }
    return ret;
}

void accelerate_particles(Particles& particles, float delta, bool flip) {
    for (size_t i1 = 0; i1 < particles.size(); ++i1) {
        Particle& p1 = particles[i1];
        if (p1.deleted) continue;
        float& p1sx1 = !flip ? p1.sx1 : p1.sx2;
        float& p1sy1 = !flip ? p1.sy1 : p1.sy2;
        for (size_t i2 = i1+1; i2 < particles.size(); ++i2) {
            Particle& p2 = particles[i2];
            if (p2.deleted) continue;
            float& p2sx1 = !flip ? p2.sx1 : p2.sx2;
            float& p2sy1 = !flip ? p2.sy1 : p2.sy2;

            const float xdistance = p2sx1-p1sx1;
            const float ydistance = p2sy1-p1sy1;
            const float quadrance = std::max((xdistance*xdistance)+(ydistance*ydistance), 3.0F);    // Don't divide by anything too close to zero.
            const float distance = sqrt(quadrance);

            // Collision detection.
            float r1 = p1.d/2.0F;
            float r2 = p2.d/2.0F;
            // For simplicity, the mass is assumed to be proportional to the area of the particle. (A = pi*r^2)
            const float mass1 = glm::pi<float>()*r1*r1;
            const float mass2 = glm::pi<float>()*r2*r2;
            if (r1+r2 >= distance) {
                // Combine two particles that are colliding.
                p2.deleted = true;
                // Calculate the weighted average of the positions of the two particles.
                glm::vec2 v(p2sx1-p1sx1, p2sy1-p1sy1);
                // Don't move the point more than the distance between the two points.
                // (Could have happened if the center of one point fell inside the other point.)
                v = glm::normalize(v)*std::min(p2.d/2.0F, distance);
                p1sx1 += v[0];
                p1sy1 += v[1];
                // Calculate the weighted average of the velocities of the two particles.
                glm::vec2 v1(p1.vx, p1.vy);
                glm::vec2 v2(p2.vx, p2.vy);
                glm::vec2 v3 = ((mass1*v1)+(mass2*v2))/(mass1+mass2);
                p1.vx = v3[0];
                p1.vy = v3[1];
                // Calculate the sum of the areas of the two particles.
                p1.d = sqrt((mass1+mass2)/glm::pi<float>())*2.0F;
            } else {
                // Apply the acceleration from the force felt between two particles.
                const float gforce = 100.0F*(mass1*mass2)/quadrance;    // TODO
                const float gacceleration1 = gforce/mass1;
                const float gacceleration2 = -(gforce/mass2);

                const float xacceleration1 = (gacceleration1*xdistance)/distance;
                const float yacceleration1 = (gacceleration1*ydistance)/distance;
                const float xacceleration2 = (gacceleration2*xdistance)/distance;
                const float yacceleration2 = (gacceleration2*ydistance)/distance;

                p1.vx += xacceleration1*delta;
                p1.vy += yacceleration1*delta;
                p2.vx += xacceleration2*delta;
                p2.vy += yacceleration2*delta;
            }
        }
    }
}

void move_particles(Particles& particles, float delta, bool flip) {
    for (auto& p : particles) {
        if (p.deleted) continue;
        float& sx1 = !flip ? p.sx1 : p.sx2;
        float& sy1 = !flip ? p.sy1 : p.sy2;
        float& sx2 = !flip ? p.sx2 : p.sx1;
        float& sy2 = !flip ? p.sy2 : p.sy1;
        sx2 = sx1 + (p.vx * delta);
        sy2 = sy1 + (p.vy * delta);
    }
}

void draw_particles(const Particles& particles, unsigned int shader_program) {
    std::vector<GLfloat> points;
    points.reserve(particles.size()*sizeof(GLfloat)*2);
    for (const auto& p : particles) {
        if (p.deleted) continue;
        points.push_back(p.sx1);
        points.push_back(p.sy1);
        points.push_back(p.d);
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

    // Configure the VAO and VBO.
    glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(GLfloat), &points[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)8);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glDrawArrays(GL_POINTS, 0, points.size()/3);

    // Clean up the VAO and VBO.
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}
