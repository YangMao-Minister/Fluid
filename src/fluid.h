#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#include "particle_system.h"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <vector>
#include <string>
#include <queue>

class Main
{
    // rendering and event handling
public:
    Main(Parameters &params) : particleSystem(params), params(params) {}
    void run();

private:
    void processEvents();
    void update();
    void initialize();
    void render();
    void postEffects();
    void debugEffects();
    void renderParticles();
    void showGui();
    void visualizeNeighbors();
    void rebuildGrid();
    sf::Color hsvToRgb(float h, float s, float v);
    sf::Color lerpColor(const sf::Color &a, const sf::Color &b, float t);
    sf::Color reserveColor(const sf::Color &color);

    sf::RenderWindow window;
    sf::RenderTexture particleBuffer;
    sf::RenderTexture postBuffer;
    sf::RenderTexture densityBuffer;
    sf::RenderTexture densityBuffer2;
    sf::RenderTexture backgroundBuffer;
    sf::VertexArray particleVertices{sf::PrimitiveType::Triangles};
    sf::VertexArray densityVertices{sf::PrimitiveType::Triangles};
    sf::VertexArray gridVertices{sf::PrimitiveType::Lines};
    sf::Texture particleTexture;
    sf::Texture densityTexture;
    Parameters &params;
    sf::Clock deltaClock;
    sf::Clock debugClock;
    sf::Clock timer;
    Vector2f mousePosition;
    sf::Shader densityShader;
    ParticleSystem particleSystem;
    float currentFps = 0;
    float renderTime = 0;
    float updateTime = 0;
    bool paused = false;
    size_t frameCount = 0;
    std::queue<float, std::deque<float>> frameTimesHistory;
    int neighborCount = 0;
};