#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace zikada::tests {
void addRealtimeRingBufferTests(std::vector<std::pair<std::string, std::function<void()>>>& tests);
void addSliceEngineTests(std::vector<std::pair<std::string, std::function<void()>>>& tests);
void addStepSchedulerTests(std::vector<std::pair<std::string, std::function<void()>>>& tests);
void addLaneTransitionTests(std::vector<std::pair<std::string, std::function<void()>>>& tests);
void addSequencerStateTests(std::vector<std::pair<std::string, std::function<void()>>>& tests);
}

int main()
{
    std::vector<std::pair<std::string, std::function<void()>>> tests;
    zikada::tests::addRealtimeRingBufferTests(tests);
    zikada::tests::addSliceEngineTests(tests);
    zikada::tests::addStepSchedulerTests(tests);
    zikada::tests::addLaneTransitionTests(tests);
    zikada::tests::addSequencerStateTests(tests);

    int failures = 0;
    for (const auto& [name, test] : tests)
    {
        try
        {
            test();
            std::cout << "PASS " << name << "\n";
        }
        catch (const std::exception& e)
        {
            ++failures;
            std::cerr << "FAIL " << name << ": " << e.what() << "\n";
        }
    }

    return failures == 0 ? 0 : 1;
}
