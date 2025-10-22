#include "fluid.h"
#include <SFML/OpenGL.hpp>
#include <cmath>
#include <iostream>

using sf::Event;
using sf::Vector2f;
using sf::Vector2i;
using sf::Keyboard::isKeyPressed;
using sf::Keyboard::Key;

void Main::run()
{
    initialize();
    while (window.isOpen())
    {
        processEvents();
        debugClock.restart();
        if (!paused)
            update();
        updateTime = debugClock.restart().asMilliseconds();
        render();
        renderTime = debugClock.restart().asMilliseconds();
        float frameTime = updateTime + renderTime;
        currentFps = 1000.f / (frameTime);

        frameTimesHistory.push(frameTime);
        if (frameTimesHistory.size() > 120)
            frameTimesHistory.pop();

        frameCount++;
    }
}

void Main::initialize()
{
    sf::ContextSettings settings;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    window.create(
        sf::VideoMode({params.windowWidth, params.windowHeight}),
        "( > v < )",
        sf::Style::Default,
        sf::State::Windowed,
        settings);
    // get OpnGL version
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    particleBuffer = sf::RenderTexture({params.windowWidth, params.windowHeight});
    postBuffer = sf::RenderTexture({params.windowWidth, params.windowHeight});
    densityBuffer = sf::RenderTexture({params.windowWidth, params.windowHeight});
    densityBuffer2 = sf::RenderTexture({params.windowWidth, params.windowHeight});
    backgroundBuffer = sf::RenderTexture({params.windowWidth, params.windowHeight});
    window.setFramerateLimit(params.targetFps);

    ImGui::SFML::Init(window);
    ImGui::GetIO().IniFilename = nullptr; // disable saving .ini file
    ImGui::GetIO().LogFilename = nullptr; // disable logging to file

    particleSystem.initParticles(params.particleCount);
    particleTexture.loadFromFile("assets/textures/particle.png");
    densityTexture.loadFromFile("assets/textures/density.png");
    densityTexture.setSmooth(true);

    particleSystem.particleRadius = particleTexture.getSize().x / 2.0f;
    densityShader.loadFromFile("assets/shaders/density.frag", sf::Shader::Type::Fragment);

    rebuildGrid();
    timer.start();
}

void Main::update()
{
    for (int step = 0; step < params.stepCount; ++step)
    {
        particleSystem.updateParticles(params.timeScale * (renderTime + updateTime) / 100.0f / params.stepCount);
    }
}

void Main::render()
{
    ImGui::SFML::Update(window, deltaClock.restart());
    particleBuffer.clear(sf::Color::Transparent);
    densityBuffer.clear(sf::Color::Transparent);
    renderParticles();
    particleBuffer.display();
    densityBuffer.display();

    backgroundBuffer.clear(params.backgroundColor);
    postBuffer.clear(sf::Color::Transparent);
    if (params.debugMode)
        debugEffects();
    postEffects();
    backgroundBuffer.display();
    postBuffer.display();

    window.resetGLStates();
    showGui();
    window.clear();
    window.draw(sf::Sprite(postBuffer.getTexture()));
    ImGui::SFML::Render(window);
    window.display();
}

void Main::renderParticles()
{
    particleVertices.clear();
    densityVertices.clear();

    for (size_t i = 0; i < particleSystem.particlePosition.size(); ++i)
    {

        Vector2f &p = particleSystem.particlePosition[i];
        int s = params.densitySampleRadius;
        float d = densityTexture.getSize().x;
        sf::Color dColor = sf::Color::White;
        densityVertices.append(sf::Vertex{p + Vector2f(-s, -s), dColor, Vector2f(0, 0)});
        densityVertices.append(sf::Vertex{p + Vector2f(s, -s), dColor, Vector2f(d, 0)});
        densityVertices.append(sf::Vertex{p + Vector2f(-s, s), dColor, Vector2f(0, d)});
        densityVertices.append(sf::Vertex{p + Vector2f(s, -s), dColor, Vector2f(d, 0)});
        densityVertices.append(sf::Vertex{p + Vector2f(s, s), dColor, Vector2f(d, d)});
        densityVertices.append(sf::Vertex{p + Vector2f(-s, s), dColor, Vector2f(0, d)});

        if (!params.showParticles)
            continue;

        sf::Color pColor = lerpColor(
            sf::Color::Cyan,
            sf::Color::Red,
            std::atan(particleSystem.particleVelocity[i].length() / 20.0f) * 2 / 3.14159f);
        if (params.debugMode)
        {
            if ((particleSystem.particlePosition[i] - mousePosition).lengthSquared() < params.densitySampleRadius * params.densitySampleRadius)
            {
                pColor += sf::Color(0, 255, 0, 128);
            }
            if (particleSystem.particleVelocity[i].lengthSquared() > 500.0f * 500.0f)
            {
                pColor = sf::Color(255, 255, 255, 255);
            }
        }
        s = particleTexture.getSize().x / 2;
        particleVertices.append(sf::Vertex{p + Vector2f(-s, -s), pColor, Vector2f(0, 0)});
        particleVertices.append(sf::Vertex{p + Vector2f(s, -s), pColor, Vector2f(2 * s, 0)});
        particleVertices.append(sf::Vertex{p + Vector2f(-s, s), pColor, Vector2f(0, 2 * s)});
        particleVertices.append(sf::Vertex{p + Vector2f(s, -s), pColor, Vector2f(2 * s, 0)});
        particleVertices.append(sf::Vertex{p + Vector2f(s, s), pColor, Vector2f(2 * s, 2 * s)});
        particleVertices.append(sf::Vertex{p + Vector2f(-s, s), pColor, Vector2f(0, 2 * s)});
    }

    sf::RenderStates states;
    states.blendMode = sf::BlendAdd;
    states.texture = &densityTexture;
    densityBuffer.draw(densityVertices, states);

    states.texture = &particleTexture;
    states.blendMode = sf::BlendAlpha;
    states.shader = nullptr;
    particleBuffer.draw(particleVertices, states);
}

void Main::postEffects()
{
    // layers:
    // 0. background (grid)
    // 1. density (densityBuffer) using densityShader
    // 2. particles (particleBuffer)

    if (params.showGrid)
        backgroundBuffer.draw(gridVertices);

    sf::Sprite backgroundSprite(backgroundBuffer.getTexture());
    postBuffer.draw(backgroundSprite);

    if (params.showDensity)
    {
        sf::Sprite densitySprite(densityBuffer.getTexture());
        densityShader.setUniform("u_targetDensity", params.targetDensity);
        densityShader.setUniform("u_sampleRadius", params.densitySampleRadius);
        densityShader.setUniform("u_texture", sf::Shader::CurrentTexture);
        densityShader.setUniform("u_bgtexture", backgroundBuffer.getTexture());
        densityShader.setUniform("u_texture2", densityBuffer2.getTexture());
        densityShader.setUniform("u_solution", Vector2f(params.windowWidth, params.windowHeight));
        densityShader.setUniform("u_time", timer.getElapsedTime().asSeconds());
        postBuffer.draw(densitySprite, &densityShader);
        if (!paused)
        {
            densityBuffer2.clear(sf::Color::Transparent);
            densityBuffer2.draw(densitySprite);
            densityBuffer2.display();
        }
    }

    if (params.showParticles)
    {
        sf::Sprite particleSprite(particleBuffer.getTexture());
        postBuffer.draw(particleSprite);
    }
}

void Main::debugEffects()
{
    sf::CircleShape mouseCircle(params.densitySampleRadius);
    mouseCircle.setOutlineColor(sf::Color::Green);
    mouseCircle.setOutlineThickness(1.0f);
    mouseCircle.setFillColor(sf::Color(24, 140, 24, 48));
    Vector2i mousePos = sf::Mouse::getPosition(window);
    mouseCircle.setPosition(Vector2f(mousePos) - Vector2f(params.densitySampleRadius, params.densitySampleRadius));
    postBuffer.draw(mouseCircle);

    visualizeNeighbors();
}

void Main::visualizeNeighbors()
{
    auto neighbors = particleSystem.getParticlesWithRadius(mousePosition);
    neighborCount = neighbors.size();
    for (int particleIndex : neighbors)
    {
        sf::CircleShape circle(10);
        circle.setPosition(particleSystem.particlePosition[particleIndex] - Vector2f(10, 10));
        circle.setFillColor(sf::Color(255, 255, 255, 128));
        particleBuffer.draw(circle);
    }
}

void Main::showGui()
{
    // ImGui::ShowDemoWindow();

    ImGui::Begin("Fluid Controls");
    ImGui::Checkbox("Debug Mode", &params.debugMode);

    if (params.debugMode)
    {
        ImGui::Text("FPS: %.1f", currentFps);
        ImGui::Text("Update time: %.2f ms", updateTime);
        ImGui::Text("Render time: %.2f ms", renderTime);
        ImGui::Text("Mouse Position: (%.1f, %.1f )", mousePosition.x, mousePosition.y);
        ImGui::Text("Mouse Density: %.4f", particleSystem.getDensityAt(mousePosition));
        ImGui::Text("Neighbor Count: %d", neighborCount);
        ImGui::Checkbox("Show Frame Time", &params.showFrameTime);
    }
    ImGui::SliderInt("Particle Count", &params.particleCount, 64, 4096);
    ImGui::SliderFloat("Time Scale", &params.timeScale, 0.1f, 2.0f);
    ImGui::SliderInt("Step Count", &params.stepCount, 1, 6);
    ImGui::SliderFloat("Target Density", &params.targetDensity, 0.1f, 1.0f);
    if (ImGui::SliderFloat("Force Strength", &params.forceStrength, 0.0f, 10.0f))
    {
        particleSystem.forceStrengthOriginal = params.forceStrength;
    };
    ImGui::SliderFloat("Viscosity", &params.viscosity, 0.0f, 2.0f);
    ImGui::SliderFloat("Gravity Strength", &params.gravityStrength, -5.0f, 5.0f);
    ImGui::SliderFloat("Interact Force Radius", &params.interactForceRadius, 50.0f, 250.0f);
    ImGui::SliderFloat("Interact Force Strength", &params.interactForceStrength, 1.0f, 50.0f);
    ImGui::SliderFloat("Moving Damping", &params.movingDamping, 0.0f, 0.8f);
    if (ImGui::SliderFloat("Density Sample Radius", &params.densitySampleRadius, 10.0f, 60.0f))
    {
        particleSystem.updateParticleCells();
        rebuildGrid();
    }
    ImGui::Checkbox("Enable Gravity", &params.enableGravity);
    ImGui::Checkbox("Enable Adjusting Force", &params.enableAdjustingForce);

    static float color[3] = {params.backgroundColor.r / 255.0f, params.backgroundColor.g / 255.0f, params.backgroundColor.b / 255.0f};
    if (ImGui::ColorEdit3("Background Color", color))
    {
        params.backgroundColor = sf::Color((int)(color[0] * 255), (int)(color[1] * 255), (int)(color[2] * 255));
        rebuildGrid();
    };

    if (ImGui::TreeNode("Layers"))
    {
        ImGui::Checkbox("Show Particles", &params.showParticles);
        if (ImGui::Checkbox("Show Grid", &params.showGrid))
            rebuildGrid();
        ImGui::Checkbox("Show Liquid Effects", &params.showDensity);
        ImGui::TreePop();
    }

    if (ImGui::Button("Reset"))
        particleSystem.initParticles(params.particleCount);
    ImGui::SameLine();
    if (ImGui::Button("Step"))
        update();
    ImGui::SameLine();
    ImGui::Checkbox("Paused", &paused);

    ImGui::End();

    if (params.showFrameTime)
    {
        float average = 0;
        for (int i = 0; i < frameTimesHistory.size(); ++i)
        {
            average += (&frameTimesHistory.front())[i];
        }
        average /= frameTimesHistory.size();
        ImGui::Begin("Frame Time");
        ImGui::Text("FPS: %.1f", currentFps);
        ImGui::Text("Average frame time: %.1f", average);
        ImGui::PlotLines("Frame times", &frameTimesHistory.front(), frameTimesHistory.size(), 0, nullptr, 0.0f, 100.0f, sf::Vector2f(300, 100));
        ImGui::End();
    }
}

void Main::processEvents()
{
    while (const std::optional<Event> event = window.pollEvent())
    {
        ImGui::SFML::ProcessEvent(window, *event);
        
        if (event->is<Event::Closed>())
        window.close();
        if (auto resizeEvent = event->getIf<Event::Resized>())
        {
            int height = (int)resizeEvent->size.y;
            int width = (int)resizeEvent->size.x;
            params.windowWidth = width;
            params.windowHeight = height;
            sf::FloatRect visibleArea({0, 0}, {(float)width, (float)height});
            window.setView(sf::View(visibleArea));
            particleBuffer.resize({width, height});
            postBuffer.resize({width, height});
            densityBuffer.resize({width, height});
            densityBuffer2.resize({width, height});
            backgroundBuffer.resize({width, height});
            particleSystem.updateCellSizes();
            particleSystem.updateParticleCells();
            rebuildGrid();
        }
        if (auto keyPressedEvent = event->getIf<Event::KeyPressed>())
        {
            switch (keyPressedEvent->code)
            {
                case sf::Keyboard::Key::Escape:
                window.close();
                break;
                case sf::Keyboard::Key::Space:
                paused = !paused;
                break;
                case sf::Keyboard::Key::Enter:
                paused = false;
                update();
                render();
                paused = true;
                break;
                case sf::Keyboard::Key::R:
                particleSystem.initParticles(params.particleCount);
                break;
                default:
                break;
            }
        }
    }
    
    mousePosition = Vector2f(sf::Mouse::getPosition(window));
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
        particleSystem.applyCentralForce(mousePosition, params.interactForceRadius, params.interactForceStrength);
    }
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
    {
        particleSystem.applyCentralForce(mousePosition, params.interactForceRadius, -params.interactForceStrength);
    }
}

void Main::rebuildGrid()
{
    sf::Color gridColor = params.backgroundColor * 4;
    gridVertices.clear();
    for (float x = 0; x < params.windowWidth; x += params.densitySampleRadius)
    {
        gridVertices.append(sf::Vertex{sf::Vector2f(x, 0), gridColor});
        gridVertices.append(sf::Vertex{sf::Vector2f(x, params.windowHeight), gridColor});
    }
    for (float y = 0; y < params.windowHeight; y += params.densitySampleRadius)
    {
        gridVertices.append(sf::Vertex{sf::Vector2f(0, y), gridColor});
        gridVertices.append(sf::Vertex{sf::Vector2f(params.windowWidth, y), gridColor});
    }
}

sf::Color hsvToRgb(float h, float s, float v)
{
    int i = static_cast<int>(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    float r, g, b;
    switch (i % 6)
    {
    case 0:
        r = v, g = t, b = p;
        break;
    case 1:
        r = q, g = v, b = p;
        break;
    case 2:
        r = p, g = v, b = t;
        break;
    case 3:
        r = p, g = q, b = v;
        break;
    case 4:
        r = t, g = p, b = v;
        break;
    case 5:
        r = v, g = p, b = q;
        break;
    default:
        r = g = b = 0; // Should never happen
        break;
    }
    return sf::Color(r * 255, g * 255, b * 255);
}

sf::Color Main::lerpColor(const sf::Color &a, const sf::Color &b, float t)
{
    return sf::Color(
        (a.r + (b.r - a.r) * t),
        (a.g + (b.g - a.g) * t),
        (a.b + (b.b - a.b) * t),
        (a.a + (b.a - a.a) * t));
}

sf::Color Main::reserveColor(const sf::Color &color)
{
    return sf::Color(
        255 - color.r,
        255 - color.g,
        255 - color.b,
        color.a);
}
