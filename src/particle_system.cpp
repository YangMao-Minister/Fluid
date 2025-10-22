#include "particle_system.h"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <omp.h>

ParticleSystem::ParticleSystem(Parameters &params) : params(params)
{
    forceStrengthOriginal = params.forceStrength;
}

void ParticleSystem::updateParticles(float timeStep)
{
    debugTimerT.reset();
    debugTimerS.reset();
    updateParticleCells();
    debugTimerS.printD("  Update Particle Cells :");

    // std::cout << "UPC\n";
    // printDuration();

    // std::cout << "UDB\n";
    // printDuration();

    // #pragma omp parallel for
    for (int i = 0; i < particlePosition.size(); ++i)
    {
        particlePosition[i] += particleVelocity[i] * timeStep;
        particlePositionPredicted[i] = particlePosition[i] + particleVelocity[i] * timeStep;
        processParticleAboutToOutOfBounds(i, timeStep);
    }
    debugTimerS.printD("  Update Particle Position :");

    // #pragma omp parallel for
    for (int i = 0; i < particlePosition.size(); ++i)
    {
        particleDensity[i] = std::clamp(getDensityAt(particlePositionPredicted[i]), 0.001f, 2.0f);
        if (params.enableAdjustingForce)
            adjustForceStrength(particleDensity[i]);
    }
    debugTimerS.printD("  Update Particle Density :");

    // #pragma omp parallel for
    for (int i = 0; i < particlePosition.size(); ++i)
    {
        if (params.enableGravity)
        {
            particleVelocity[i] += Vector2f(0.0f, params.gravityStrength);
        }

        sf::Vector2f force = getPushForce(i);

        Vector2f &velocityI = particleVelocity[i];
        velocityI += -force / particleDensity[i] * timeStep;
    }
    debugTimerS.printD("  Update Particle Force :");

    for (int i = 0; i < particlePosition.size(); ++i)
    {
        Vector2f &velocityI = particleVelocity[i];
        Vector2f velocityLossed = velocityI.length() == 0 ? Vector2f(0.0f, 0.0f) : velocityI.normalized();
        velocityI -= velocityLossed * params.movingDamping * 0.01f * velocityI.lengthSquared() * timeStep;
        if (velocityI.lengthSquared() > 500.0f * 500.0f)
        {
            std::cerr << "  - Particle " + std::to_string(i) + " 's velocity is too high: " << velocityI.length() << std::endl;
            velocityI *= 0.2f;
        }
    }
    debugTimerS.printD("  Update Particle Velocity :");

    // #pragma omp parallel for
    for (int i = 0; i < particlePosition.size(); ++i)
    {
        processVisosity(i);
    }
    debugTimerS.printD("  Update Particle Visosity :");
    debugTimerT.printD("Update Particles:");

    // std::cout << "UP\n";
    // printDuration();
}

void ParticleSystem::processParticleAboutToOutOfBounds(int index, float timeStep)
{
    Vector2f nextPosition = particlePositionPredicted[index];
    if (nextPosition.y + particleRadius > params.windowHeight || nextPosition.y - particleRadius < 0)
    {
        int b = (nextPosition.y + particleRadius > params.windowHeight ? 1 : 0);
        float f = 2 * b * params.windowHeight - nextPosition.y;
        // float err = std::max(std::abs(f - (float)params.windowHeight * b) * 0.005f, 1.0f);
        particlePosition[index].y = std::clamp(f, particleRadius, (float)params.windowHeight - particleRadius);
        particleVelocity[index].y *= -params.collisionDamping;
    }
    if (nextPosition.x + particleRadius > params.windowWidth || nextPosition.x - particleRadius < 0)
    {
        int b = (nextPosition.x + particleRadius > params.windowWidth ? 1 : 0);
        float f = 2 * b * params.windowWidth - nextPosition.x;
        // float err = std::max(std::abs(f - (float)params.windowWidth * b) * 0.005f, 1.0f);
        particlePosition[index].x = std::clamp(f, particleRadius, (float)params.windowWidth - particleRadius);
        particleVelocity[index].x *= -params.collisionDamping;
    }
}

float ParticleSystem::getDensityAt(Vector2f pos) const
{
    // if (pos.x < 0 || pos.x >= params.windowWidth || pos.y < 0 || pos.y >= params.windowHeight)
    //     return -1.0f;
    // return forceFieldImage.getPixel({pos.x, pos.y}).r / 255.0f; // get the density from the texture (0-255)
    float density = 0.0f;
    for (int neighbor : getParticlesWithRadius(pos))
    {
        sf::Vector2f r = pos - particlePositionPredicted[neighbor];
        float distanceSquared = r.lengthSquared();
        density += densityKernel(std::sqrt(distanceSquared)) * params.particleMass;
    }
    return density;
}

sf::Vector2f ParticleSystem::getPushForce(int index) const
{
    sf::Vector2f pos = particlePositionPredicted[index];
    sf::Vector2f force = Vector2f(0.0f, 0.0f);
    const float maxForce = 1000.0f; // max force to prevent explosion

    for (int neighbor : getParticlesWithRadius(pos))
    {
        sf::Vector2f r = pos - particlePositionPredicted[neighbor];
        if (neighbor == index || particleDensity[neighbor] == 0.0f || r.lengthSquared() == 0.0f)
            continue;
        float distance = r.length();
        float scale = gradientKernel(distance);
        Vector2f f = r / distance *
                     (getPushForceBetween(index, neighbor) + shortDistPushKernel(distance)) *
                     scale *
                     params.particleMass / particleDensity[neighbor];
        force += f;
    }

    if (force.lengthSquared() > maxForce * maxForce)
    {
        std::cerr << "  - Particle " + std::to_string(index) + "'s force is too high: " << force.length() << std::endl;
        force = force.normalized() * maxForce;
        return force;
    }
    return force;
}

float ParticleSystem::densityKernel(float dist) const
{
    float a = std::max(params.densitySampleRadius - dist, 0.0f);
    float v = 3.14159f * std::pow(params.densitySampleRadius, 4) / 6;
    return a * a / v;
}

float ParticleSystem::gradientKernel(float dist) const
{
    float v = 3.14159f * std::pow(params.densitySampleRadius, 4);
    return 12 * (dist - params.densitySampleRadius) / v;
}

float ParticleSystem::shortDistPushKernel(float dist) const
{
    float v = 3.14159f * std::pow(params.densitySampleRadius, 4) / 6;
    float a = 16 * (dist - params.densitySampleRadius);
    return -a * a / v;
}

float ParticleSystem::getPushForceBetween(int i, int j) const
{
    float f1 = (particleDensity[i] - params.targetDensity) * params.forceStrength * 1000.0f;
    float f2 = (particleDensity[j] - params.targetDensity) * params.forceStrength * 1000.0f;
    return (f1 + f2) / 2.0f;
}

void ParticleSystem::addParticle(Vector2f pos)
{
    particlePosition.push_back(pos);
    particleVelocity.push_back(Vector2f(0.0f, 0.0f));
    particleDensity.push_back(0.0f);
    particlePositionPredicted.push_back(Vector2f(0.0f, 0.0f));
    particleCellIndices.push_back({particlePosition.size() - 1, -1});
}

void ParticleSystem::initParticles(int count)
{
    particlePosition.clear();
    particleVelocity.clear();
    particlePositionPredicted.clear();
    particleDensity.clear();
    particleCellIndices.clear();

    // compute grid dimensions (cols x rows) consistently
    int cols = static_cast<int>(params.windowWidth / params.densitySampleRadius) + 1;
    int rows = static_cast<int>(params.windowHeight / params.densitySampleRadius) + 1;
    cellStartIndices.clear();
    cellStartIndices.resize(cols * rows, -1);

    float particleSpacing = 15.0f;

    int w = std::sqrt(count);
    float x0 = params.windowWidth / 2.0f - w * particleSpacing / 2.0f;
    float y0 = params.windowHeight / 2.0f - w * particleSpacing / 2.0f;
    for (int i = 0; i < count; ++i)
    {
        float x = x0 + (i % w) * particleSpacing;
        float y = y0 + (i / w) * particleSpacing;
        addParticle(Vector2f(x, y));
    }

    for (size_t i = 0; i < particlePosition.size(); ++i)
    {
        particleDensity[i] = getDensityAt(particlePosition[i]);
    }

    params.forceStrength = forceStrengthOriginal;
}

void ParticleSystem::updateParticleCells()
{
    // Reset cellStartIndices for all cells
    std::fill(cellStartIndices.begin(), cellStartIndices.end(), -1);

    // Update particle cell indices
    // Ensure particleCellIndices has the right size before writing
    if (particleCellIndices.size() != particlePosition.size())
        particleCellIndices.resize(particlePosition.size(), {-1, -1});

    for (int i = 0; i < particlePosition.size(); ++i)
    {
        particleCellIndices[i] = {i, getCellIndex(particlePosition[i])};
    }

    // Sort particleCellIndices by cell index
    std::sort(particleCellIndices.begin(), particleCellIndices.end(), [](const auto &a, const auto &b)
              { return std::get<1>(a) < std::get<1>(b); });

    // Update cell start indices
    for (size_t i = 0; i < particlePosition.size(); ++i)
    {
        int cellIndex = std::get<1>(particleCellIndices[i]);
        int cellIndexPrev = (i > 0) ? std::get<1>(particleCellIndices[i - 1]) : -1;
        if (cellIndex != cellIndexPrev)
        {
            // guard against invalid cellIndex
            if (cellIndex >= 0 && cellIndex < static_cast<int>(cellStartIndices.size()))
                cellStartIndices[cellIndex] = static_cast<int>(i);
            else
                ; // ignore invalid cellIndex
        }
    }
}

int ParticleSystem::getCellIndex(Vector2f pos) const
{
    int col = static_cast<int>(pos.x / params.densitySampleRadius);
    int row = static_cast<int>(pos.y / params.densitySampleRadius);
    int cols = static_cast<int>(params.windowWidth / params.densitySampleRadius) + 1;
    return col + row * cols;
}

int ParticleSystem::getCellIndex(sf::Vector2i cellPos) const
{
    int cols = static_cast<int>(params.windowWidth / params.densitySampleRadius) + 1;
    return cellPos.x + cellPos.y * cols;
}

vector<int> ParticleSystem::getParticlesWithRadius(Vector2f pos) const
{
    vector<int> neighbors;
    sf::Vector2i centerCell = sf::Vector2i((int)(pos.x / params.densitySampleRadius), (int)(pos.y / params.densitySampleRadius));

    sf::Vector2i offset[9] = {
        {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    int cols = static_cast<int>(params.windowWidth / params.densitySampleRadius) + 1;
    int rows = static_cast<int>(params.windowHeight / params.densitySampleRadius) + 1;

    for (auto &off : offset)
    {
        sf::Vector2i neighborCell = centerCell + off;
        if (neighborCell.x < 0 || neighborCell.y < 0 || neighborCell.x >= cols || neighborCell.y >= rows)
            continue;
        int cellIndex = getCellIndex(neighborCell);
        if (cellIndex < 0 || cellIndex >= static_cast<int>(cellStartIndices.size()))
            continue;
        int startIndex = cellStartIndices[cellIndex];
        if (startIndex == -1)
            continue;
        for (int i = startIndex; i < static_cast<int>(particlePosition.size()) && std::get<1>(particleCellIndices[i]) == cellIndex; ++i)
        {
            int particleIndex = std::get<0>(particleCellIndices[i]);
            if (particleIndex < 0 || particleIndex >= static_cast<int>(particlePosition.size()))
                continue;
            float distanceSquared = (particlePosition[particleIndex] - pos).lengthSquared();
            if (distanceSquared < params.densitySampleRadius * params.densitySampleRadius)
            {
                neighbors.push_back(particleIndex);
            }
        }
    }
    return neighbors;
}

void ParticleSystem::applyCentralForce(Vector2f center, float radius, float strength)
{
    for (size_t i = 0; i < particlePosition.size(); ++i)
    {
        sf::Vector2f dir = particlePosition[i] - center;
        float dist = dir.length();
        if (dist < radius && dist > 0.01f)
        {
            dir /= dist;
            float force = strength * (1.1f - dist / radius);
            particleVelocity[i] += dir * force / particleDensity[i];
        }
    }
}

void ParticleSystem::processVisosity(int index)
{
    Vector2f force = Vector2f(0.0f, 0.0f);
    for (int neighbor : getParticlesWithRadius(particlePosition[index]))
    {
        if (neighbor == index)
            continue;
        sf::Vector2f r = particlePosition[index] - particlePosition[neighbor];
        float f = densityKernel(std::sqrt(r.lengthSquared()));
        force += (particleVelocity[neighbor] - particleVelocity[index]) * f;
    }
    particleVelocity[index] += force * 10.0f * params.viscosity / particleDensity[index];
}

void ParticleSystem::updateCellSizes()
{
    int cols = static_cast<int>(params.windowWidth / params.densitySampleRadius) + 1;
    int rows = static_cast<int>(params.windowHeight / params.densitySampleRadius) + 1;
    cellStartIndices.resize(cols * rows, -1);
}

void ParticleSystem::adjustForceStrength(float density)
{
    float derr = std::clamp((density - params.targetDensity) / params.targetDensity, -1.0f, 1.0f);
    float ferr = std::clamp((params.forceStrength - forceStrengthOriginal) / forceStrengthOriginal, -1.0f, 1.0f);
    if (abs(derr) > 0.5f)
    {
        params.forceStrength = std::clamp(params.forceStrength + forceStrengthOriginal * ((1.0f - derr) * 0.1f), 0.1f * forceStrengthOriginal, 2.0f * forceStrengthOriginal);
        return;
    }
    params.forceStrength *= (1 - ferr * 0.1f);
}