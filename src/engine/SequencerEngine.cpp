#include "engine/SequencerEngine.h"

namespace zikada {

SequencerEngine::SequencerEngine() = default;

void SequencerEngine::prepare(double sr, int blockSize)
{
    sampleRate = sr;
    juce::ignoreUnused(blockSize);
    reset();
}

void SequencerEngine::reset()
{
    sampleCounter = 0.0;
    currentStep = 0;
}

void SequencerEngine::setTempo(double bpm)
{
    tempoBPM = bpm;
    double beatDuration = 60.0 / tempoBPM;
    double stepDuration = beatDuration;
    
    switch (stepResolution)
    {
        case 0: stepDuration *= 0.5; break;
        case 1: stepDuration *= 1.0; break;
        case 2: stepDuration *= 2.0; break;
        default: break;
    }
    
    samplesPerStep = stepDuration * sampleRate;
}

void SequencerEngine::setStepResolution(int resolution)
{
    stepResolution = resolution;
    setTempo(tempoBPM);
}

void SequencerEngine::setPlaying(bool playing)
{
    isPlaying = playing;
}

void SequencerEngine::advance(int numSamples)
{
    if (!isPlaying || samplesPerStep <= 0.0)
        return;
    
    sampleCounter += numSamples;
    
    while (sampleCounter >= samplesPerStep)
    {
        sampleCounter -= samplesPerStep;
        currentStep = (currentStep + 1) % 16;
    }
}

}
