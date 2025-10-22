#include <chrono>
#include <string>
#include <SFML/Graphics.hpp>

class DebugTimer
{
private:
    std::chrono::high_resolution_clock timer;
    std::chrono::_V2::system_clock::time_point timeStart;

public:
    DebugTimer()
    {
        timeStart = timer.now();
    };
    void reset();
    void printD(std::string msg);
};

sf::Color operator*(const sf::Color &color, float factor);
