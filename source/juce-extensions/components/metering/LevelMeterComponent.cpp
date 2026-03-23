#include "LevelMeterComponent.h"

LevelMeterComponent::Options LevelMeterComponent::Options::getDefault()
{
    return {};
}

LevelMeterComponent::Options LevelMeterComponent::Options::withMaxChannels (int const newMaxChannels) const
{
    auto copy = *this;
    copy.maxChannels = newMaxChannels;
    return copy;
}

LevelMeterComponent::LevelMeterComponent (const LevelMeter::Scale& scale, [[maybe_unused]] const Options& options) :
    Subscriber (scale, options.maxChannels)
{
}

void LevelMeterComponent::measurementUpdatesFinished()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    bool isSilent { true };

    for (int ch = 0; ch < getNumChannels(); ++ch)
    {
        if (getScale().calculateProportionForLevel (std::max (getPeakValue (ch), getPeakHoldValue (ch))) > .001)
        {
            isSilent = false;
            break;
        }
    }

    if (!isSilent || !mWasSilent)
        repaint();

    mWasSilent = isSilent;
}

void LevelMeterComponent::levelMeterPrepared ([[maybe_unused]] int numChannels)
{
    JUCE_ASSERT_MESSAGE_THREAD;
}

void LevelMeterComponent::setOptions (const LevelMeterComponent::Options& options)
{
    mOptions = options;
    repaint();
}

void LevelMeterComponent::paint (juce::Graphics& g)
{
    auto isHorizontal = getWidth() > getHeight();

    auto bounds = getLocalBounds();
    auto meterBounds = bounds.toFloat();

    auto numChannels = getNumChannels();

    auto barSeparationSpace = 1.f;
    auto totalSize = isHorizontal ? meterBounds.getHeight() : meterBounds.getWidth();
    float const barSize = (totalSize - (barSeparationSpace * static_cast<float> (numChannels - 1))) /
                          static_cast<float> (numChannels);

    const auto& scale = getScale();

    // Draw level bars and peak hold values.
    for (int ch = 0; ch < numChannels; ch++)
    {
        auto const peak = getPeakValue (ch);
        auto const peakProportion = scale.calculateProportionForLevel (peak);

        auto const peakHold = getPeakHoldValue (ch);
        auto const peakHoldProportion = scale.calculateProportionForLevel (peakHold);

        if (ch > 0)
        {
            isHorizontal ? meterBounds.removeFromTop (barSeparationSpace)
                         : meterBounds.removeFromLeft (barSeparationSpace);
        }

        auto barBounds = isHorizontal ? meterBounds.removeFromTop (barSize) : meterBounds.removeFromLeft (barSize);

        if (isHorizontal)
        {
            if (peakHold >= LevelMeterConstants::kOverloadTriggerLevel)
            {
                g.setColour (juce::Colours::red);
                g.fillRect (barBounds.withLeft (barBounds.getWidth() - kOverloadAreaSize));
            }

            g.setColour (juce::Colours::darkgreen);
            g.fillRect (barBounds.withWidth (
                (meterBounds.getWidth() - static_cast<float> (kOverloadAreaSize)) *
                static_cast<float> (peakProportion)));

            g.setColour (juce::Colours::darkgreen.brighter());
            g.drawVerticalLine (
                juce::roundToInt (
                    ((meterBounds.getWidth() - static_cast<float> (kOverloadAreaSize)) *
                     static_cast<float> (peakHoldProportion))),
                0,
                barBounds.getBottom());
        }
        else
        {
            if (peakHold >= LevelMeterConstants::kOverloadTriggerLevel)
            {
                g.setColour (juce::Colours::red);
                g.fillRect (barBounds.withBottom (kOverloadAreaSize));
            }

            g.setColour (juce::Colours::darkgreen);
            g.fillRect (barBounds.withTrimmedTop (
                meterBounds.getHeight() -
                (meterBounds.getHeight() - kOverloadAreaSize) * static_cast<float> (peakProportion)));

            g.setColour (juce::Colours::darkgreen.brighter());
            g.drawHorizontalLine (
                juce::roundToInt (
                    meterBounds.getHeight() -
                    (meterBounds.getHeight() - kOverloadAreaSize) * static_cast<float> (peakHoldProportion)),
                barBounds.getX(),
                barBounds.getRight());
        }
    }

    g.setColour (juce::Colours::black);
    g.drawRect (bounds);
}

void LevelMeterComponent::updateWithMeasurement (const LevelMeter::Measurement& measurement)
{
    Subscriber::updateWithMeasurement (measurement);
}
