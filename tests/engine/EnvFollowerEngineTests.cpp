#include "engine/EnvFollowerEngine.h"

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace zikada::tests {

void addEnvFollowerEngineTests(std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.push_back({"EnvFollowerEngine applies bipolar output mapping", []
    {
        EnvFollowerEngine engine;
        engine.prepare(1000.0);

        engine.setParameters(1.0f, 1.0f, false);
        const float unipolarSilence = engine.processSample(0.0f, 0.0f);

        engine.reset();
        engine.setParameters(1.0f, 1.0f, true);
        const float bipolarSilence = engine.processSample(0.0f, 0.0f);

        if (std::abs(unipolarSilence) > 0.0001f)
            throw std::runtime_error("unipolar silence must remain at zero");
        if (std::abs(bipolarSilence + 1.0f) > 0.0001f)
            throw std::runtime_error("bipolar silence must map the envelope floor to -1");
    }});
}

} // namespace zikada::tests
