#include <SFML/Graphics.hpp>
struct Parameters
{
    static Parameters DEFAULT;
    unsigned windowWidth = 1200;
    unsigned windowHeight = 800;
    int targetFps = 60;
    int particleCount = 1200;
    float particleMass = 100.0f;
    float timeScale = 1.0f;
    int stepCount = 2;
    float targetDensity = 0.1f;
    float forceStrength = 6.0f;
    float viscosity = 1.2f;
    float interactForceRadius = 120.0f;
    float interactForceStrength = 8.0f;
    float densitySampleRadius = 50.0f;
    float collisionDamping = 0.3f;
    float movingDamping = 0.1f;
    float gravityStrength = 1.0f;
    sf::Color backgroundColor = sf::Color(21, 5, 30);
    bool debugMode = false;
    bool enableGravity = true;
    bool showParticles = false;
    bool showGrid = true;
    bool showDensity = true;
    bool showFrameTime = false;
    bool enableAdjustingForce = false;
};
