#include "ScaleComponent.h"
#include "LevelMeterComponent.h"

void ScaleComponent::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const bool isHorizontal = getWidth() > getHeight();

    const auto& divisions = mScale.getDivisions();

    for (auto division = divisions.begin(); division != divisions.end(); ++division)
    {
        constexpr auto scaleLineLength = 6.f;
        constexpr auto scaleNumberWidth = 30;

        const bool isFirst = division == divisions.begin();

        if (isHorizontal)
        {
            const auto xPos = b.getX() + (b.getWidth() - LevelMeterComponent::kOverloadAreaSize) *
                                             mScale.calculateProportionForLevelDb (*division);

            if (!isFirst)
            {
                g.drawVerticalLine (juce::roundToInt (xPos), 0.f, scaleLineLength);
                g.drawText (
                    juce::String (*division),
                    juce::roundToInt (xPos) - scaleNumberWidth / 2,
                    juce::roundToInt (scaleLineLength),
                    scaleNumberWidth,
                    juce::roundToInt (b.getHeight()),
                    juce::Justification::centredTop);
            }
        }
        else
        {

            const auto yPos = b.getBottom() - (b.getHeight() - LevelMeterComponent::kOverloadAreaSize) *
                                                  mScale.calculateProportionForLevelDb (*division);

            if (!isFirst)
            {
                constexpr auto scaleNumberHeight = 20;
                g.drawHorizontalLine (juce::roundToInt (yPos), 0.f, scaleLineLength);
                g.drawText (
                    juce::String (*division),
                    juce::roundToInt (scaleLineLength),
                    juce::roundToInt (yPos) - scaleNumberHeight / 2,
                    scaleNumberWidth,
                    scaleNumberHeight,
                    juce::Justification::centred);
            }
        }
    }
}
