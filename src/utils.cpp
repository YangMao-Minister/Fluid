#include "utils.h"
#include <iostream>
#include <string>
#include <SFML/Graphics.hpp>
#include <algorithm>

void DebugTimer::reset()
{
    timeStart = std::chrono::high_resolution_clock::now();
}

void DebugTimer::printD(std::string msg)
{
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - timeStart);
    reset();
    std::cout << msg << elapsed.count() << "us(" << elapsed.count() / 1000.0f << " ms)" << std::endl;
}

sf::Color operator*(const sf::Color &color, float factor)
{
    return sf::Color(
        std::clamp((int)(color.r * factor), 0, 255),
        std::clamp((int)(color.g * factor), 0, 255),
        std::clamp((int)(color.b * factor), 0, 255),
        color.a);
}
