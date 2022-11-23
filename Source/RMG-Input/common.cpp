#include "common.hpp"

#include <algorithm>
#include <cmath>

#define MAX_DIAGONAL_VALUE 69
#define DEADZONE_VALUE 7

double simulateDeadzone(double n64InputAxis, double maxAxis, int deadzone, double axisRange)
{
    double axisAbsolute = std::abs(n64InputAxis);

    if (axisAbsolute < deadzone)
    {
        axisAbsolute = 0; // No input when inside deadzone
    }
    else
    {
        // Create linear scaling factor from 0 at inner deadzone to MAX_AXIS at outer limit
        axisAbsolute = (axisAbsolute - deadzone) * maxAxis / axisRange / axisAbsolute;
    }

    return axisAbsolute;
}

// Credit: MerryMage
void simulateOctagon(double inputX, double inputY, double deadzoneFactor, double scalingFactor, int& outputX, int& outputY)
{
    double maxAxis = scalingFactor * N64_AXIS_PEAK;
    double maxDiagonal = scalingFactor * MAX_DIAGONAL_VALUE;
    int deadzone = static_cast<int>(deadzoneFactor * N64_AXIS_PEAK);
    double axisRange = maxAxis - deadzone;
    // scale to [-maxAxis, maxAxis]
    double ax = inputX * maxAxis;
    double ay = inputY * maxAxis;

    // check whether (ax, ay) is within the circle of radius MAX_AXIS
    if (double len = std::sqrt(ax*ax + ay*ay); len <= maxAxis)
    {
        // scale inputs
        ax *= simulateDeadzone(ax, maxAxis, deadzone, axisRange);
        ay *= simulateDeadzone(ay, maxAxis, deadzone, axisRange);
    }
    else
    {
        // scale ax and ay to stay on the same line, but at the edge of the circle
        len = maxAxis / len;
        ax *= len;
        ay *= len;
    }

    // bound diagonals to an octagonal range [-69, 69]
    if (ax != 0.0 && ay != 0.0)
    {
        double slope = ay / ax;
        double edgex = copysign(maxAxis / (std::abs(slope) + (maxAxis - maxDiagonal) / maxDiagonal), ax);
        double edgey = copysign(std::min(std::abs(edgex * slope), maxAxis / (1.0 / std::abs(slope) + (maxAxis - maxDiagonal) / maxDiagonal)), ay);
        edgex = edgey / slope;

        double scale = std::sqrt(edgex*edgex + edgey*edgey) / maxAxis;
        ax *= scale;
        ay *= scale;
    }

    outputX = static_cast<int>(ax);
    outputY = static_cast<int>(ay);
}
