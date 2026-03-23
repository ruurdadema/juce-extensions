#pragma once

#include "juce-extensions/audio/metering/LevelMeter.h"

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Component which shows a level meter with a certain scale.
 */
class LevelMeterComponent : public juce::Component, public LevelMeter::Subscriber
{
public:
    /// The size of the overload area.
    static constexpr int kOverloadAreaSize = 10;

    /**
     * Options to configure the behaviour of this meter.
     */
    struct Options
    {
        /// The start point of yellow.
        double yellowStartPointDb = -12.0;

        /// The start point of overload (red).
        double overloadStartPointDb = -1.0;

        /// The maximum number of channels to display. If a meter has more channels then all channels will be folded
        /// into a single mono channel.
        int maxChannels = kDefaultMaxChannels;

        /**
         * @returns The default options.
         */
        static Options getDefault();

        [[nodiscard]] Options withMaxChannels (int newMaxChannels) const;
    };

    /**
     * Constructor.
     * @param scale Scale to use.
     * @param options The meter component options.
     */
    explicit LevelMeterComponent (
        const LevelMeter::Scale& scale = LevelMeter::Scale::getDefaultScale(),
        const Options& options = Options::getDefault());

    /**
     * Sets options for this meter.
     * @param options The new options to set.
     */
    void setOptions (const Options& options);

    // MARK: juce::Component overrides -
    void paint (juce::Graphics& g) override;

private:
    /// The amount of room left around the meter on the main axis.
    static constexpr int kMargin = 10;

    /// The options for configuring this meter.
    Options mOptions;

    bool mWasSilent { false };

    // MARK: LevelMeter::Subscriber overrides -
    void updateWithMeasurement (const LevelMeter::Measurement& measurement) override;
    void measurementUpdatesFinished() override;
    void levelMeterPrepared (int numChannels) override;
};
