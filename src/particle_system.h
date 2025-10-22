#include <vector>
#include <algorithm>
#include <tuple>
#include <functional>
#include <SFML/System.hpp>
#include "parameters.h"
#include "utils.h"

using sf::Vector2f;
using std::vector;

class ParticleSystem
{
public:
    vector<sf::Vector2f> particlePosition;
    vector<sf::Vector2f> particlePositionPredicted;
    vector<sf::Vector2f> particleVelocity;
    vector<float> particleDensity;
    vector<std::tuple<int, int>> particleCellIndices;
    vector<int> cellStartIndices;
    float particleRadius;
    float forceStrengthOriginal;

    DebugTimer debugTimerT;
    DebugTimer debugTimerS;

    Parameters &params;

    ParticleSystem(Parameters &params);
    void addParticle(Vector2f pos);
    void updateParticles(float timeStep);
    void clearParticles();
    void updateParticleCells();
    void initParticles(int count);
    void applyCentralForce(Vector2f center, float radius, float strength);
    void processVisosity(int index);
    void processParticleAboutToOutOfBounds(int index, float timeStep);
    void updateCellSizes();
    void adjustForceStrength(float density);
    float getDensityAt(Vector2f pos) const;
    sf::Vector2f getPushForce(int i) const;
    float gradientKernel(float distance) const;
    float getPushForceBetween(int i, int j) const;
    float densityKernel(float distance) const;
    float shortDistPushKernel(float distance) const;
    int getCellIndex(Vector2f pos) const;
    int getCellIndex(sf::Vector2i cellPos) const;
    vector<int> getParticlesWithRadius(Vector2f pos) const;
};
