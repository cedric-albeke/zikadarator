#include "engine/EnvelopeShape.h"

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(float actual, float expected, float tolerance, const char* message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(message);
}

} // namespace

namespace zikada::tests {

void addEnvelopeShapeTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"EnvelopeShape backs late advertised envelope presets", []
    {
        require(computeEnvelopeShape(13, 0.75f) > computeEnvelopeShape(13, 0.25f),
                "swell should rise smoothly through the step");
        requireNear(computeEnvelopeShape(14, 0.0f), 0.0f, 0.001f, "fade should start silent");
        require(computeEnvelopeShape(15, 0.0625f) > computeEnvelopeShape(15, 0.1875f) + 0.25f,
                "tremolo should create a clear amplitude pulse");
        require(computeEnvelopeShape(16, 0.125f) > computeEnvelopeShape(16, 0.375f) + 0.35f,
                "wobble should swing between high and low gain");
        require(computeEnvelopeShape(17, 0.02f) > 0.85f && computeEnvelopeShape(17, 0.8f) < 0.45f,
                "punch should hit hard then decay");
        require(computeEnvelopeShape(18, 0.01f) > 0.85f && computeEnvelopeShape(18, 0.25f) < 0.25f,
                "snap should be a very short transient");
        require(computeEnvelopeShape(19, 0.75f) > computeEnvelopeShape(19, 0.25f),
                "glide should rise toward the end of the step");
        requireNear(computeEnvelopeShape(20, 0.25f), 1.0f, 0.001f, "hold should stay fully open early");
        require(computeEnvelopeShape(20, 0.9f) < 0.4f,
                "hold should release near the end of the step");
    }});

    tests.push_back({"EnvelopeShape keeps all presets finite and bounded", []
    {
        for (int preset = 5; preset <= 20; ++preset)
        {
            for (int i = 0; i <= 16; ++i)
            {
                const float phase = static_cast<float>(i) / 16.0f;
                const float shape = computeEnvelopeShape(preset, phase);
                require(std::isfinite(shape), "envelope shape should stay finite");
                require(shape >= 0.0f && shape <= 1.0f, "envelope shape should stay in gain range");
            }
        }
    }});
}

} // namespace zikada::tests
