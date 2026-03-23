#pragma once

#include "juce-extensions/audio/metering/LevelMeter.h"

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * A customized slider which applies given scale for
 */
class ScaledSlider : public juce::Slider
{
public:
    ScaledSlider() = default;

    explicit ScaledSlider (const juce::String& componentName) : Slider (componentName) {}

    ScaledSlider (const SliderStyle style, const TextEntryBoxPosition textBoxPosition) : Slider (style, textBoxPosition)
    {
    }

    /**
     * Constructor
     * @param componentName The name of the component.
     * @param scale The scale to use.
     */
    explicit ScaledSlider (const juce::String& componentName, const LevelMeter::Scale& scale) :
        Slider (componentName),
        mScale (scale)
    {
    }

    /**
     * Constructor.
     * @param style The style of the slider.
     * @param textBoxPosition The position of the textbox.
     * @param scale THe scale to apply.
     */
    ScaledSlider (const SliderStyle style, const TextEntryBoxPosition textBoxPosition, const LevelMeter::Scale& scale) :
        Slider (style, textBoxPosition),
        mScale (scale)
    {
    }

    double proportionOfLengthToValue (const double proportion) override
    {
        return mScale.calculateLevelDbForProportion (proportion);
    }

    double valueToProportionOfLength (const double value) override
    {
        return mScale.calculateProportionForLevelDb (value);
    }

private:
    const LevelMeter::Scale& mScale { LevelMeter::Scale::getDefaultScale() };
};
