#pragma once

#include "LevelPeakValue.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>

#include <readerwriterqueue/readerwriterqueue.h>

/**
 * A level meter class which can be fed measurements from a realtime audio thread and be read from another (UI) thread.
 */
class LevelMeter
{
public:
    /**
     * A unit of measurement for a specific channel.
     */
    struct Measurement
    {
        int channelIndex = 0;
        double peakLevel = 0.0;
    };

    /**
     * Class for representing ;a scale alongside a meter or slider.
     */
    class Scale
    {
    public:
        /**
         * Constructor.
         * @param minusInfinityDb Minus infinity in decibels.
         * @param divisions The points (in decibels) for all divisions, starting with the lowest levels.
         */
        Scale (double minusInfinityDb, std::initializer_list<double> divisions);

        /**
         * Calculates the proportion [0.0, 1.0] for given level.
         * @param level The level [-1.0, 1.0].
         * @return The proportion belonging to given level.
         */
        [[nodiscard]] double calculateProportionForLevel (double level) const;

        /**
         * Calculates the proportion [0.0, 1.0] for given level.
         * @param levelDb The level in decibels [-inf, 0.0].
         * @return The proportion belonging to given level.
         */
        [[nodiscard]] double calculateProportionForLevelDb (double levelDb) const;

        /**
         * Calculates the level belonging to given proportion.
         * @param proportion The proportion to calculate the level for.
         * @return The calculatedProportion.
         */
        [[nodiscard]] double calculateLevelDbForProportion (double proportion) const;

        /**
         * @return The current divisions.
         */
        [[nodiscard]] const std::vector<double>& getDivisions() const;

        /**
         * @return The current configured minus infinity.
         */
        [[nodiscard]] double getMinusInfinityDb() const;

        /**
         * @return Returns a default scale.
         */
        static const Scale& getDefaultScale();

    private:
        /// Used for runtime minus infinity configuration.
        // TODO: I don't think we need this, we should use the lowest value from the scale.
        double mMinusInfinityDb { LevelMeterConstants::kDefaultMinusInfinityDb };

        /// Stores al the levels for each division.
        std::vector<double> mDivisions;
    };

    /**
     * Baseclass for other classes which need to receive measurement updates.
     */
    class Subscriber
    {
    public:
        static constexpr int kDefaultMaxChannels = 64;

        /**
         * Struct for holding measurement data per channel.
         */
        struct ChannelData
        {
            LevelPeakValue<double> peakLevel;
            LevelPeakValue<double> peakHoldLevel;
            bool overloaded = false;
        };

        Subscriber() = delete;

        virtual ~Subscriber();

        /**
         * Constructor
         * @param scale The scale to use.
         * @param maxChannels Defines the max number of channels to display. If a meter has more channels then all
         * channels will be folded into a single mono channel. The ensures that the meter will not display more channels
         * then it can visually handle.
         */
        explicit Subscriber (const Scale& scale, int maxChannels = kDefaultMaxChannels);

        JUCE_DECLARE_NON_COPYABLE (Subscriber)
        JUCE_DECLARE_NON_MOVEABLE (Subscriber)

        /**
         * Prepared this subscriber for the amount of given channels.
         * @param numChannels Number of channels to prepare for.
         */
        void prepareToPlay (int numChannels);

        /**
         * Adds a measurement which will update the channel data.
         * @param measurement The measurement to add.
         */
        virtual void updateWithMeasurement (const Measurement& measurement);

        /**
         * Called when all measurements have been processed inside the timer callback.
         * Use this method to schedule any updates of UI.
         */
        virtual void measurementUpdatesFinished() {}

        /**
         * Resets the current data to zero (or -inf) and calls measurementUpdatesFinished() to allow the subscriber to
         * update itself.
         */
        void reset();

        /**
         * Called when the level meter was prepared. use this to configure the visual representation of the level meter.
         * @param numChannels Number of channels.
         */
        virtual void levelMeterPrepared (int numChannels) = 0;

        /**
         * @param channelIndex The index of the channel to get the value for.
         * @return The current peak value for given channel index.
         */
        double getPeakValue (int channelIndex);

        /**
         * @param channelIndex The index of the channel to get the value for.
         * @return The current peak hold value for given channel index.
         */
        double getPeakHoldValue (int channelIndex);

        /**
         * @param channelIndex The channel index.
         * @return True if the signal was overloaded at some point in history, or false if not. Use resetOverloaded() to
         * reset the value.
         */
        [[nodiscard]] bool isOverloaded (int channelIndex) const;

        /**
         * Turns off the overloaded flag.
         */
        void resetOverloaded();

        /**
         * @return The current scale for this subscriber.
         */
        [[nodiscard]] const Scale& getScale() const;

        /**
         * @return The amount of configured channels.
         */
        [[nodiscard]] int getNumChannels() const;

        /**
         * Sets the return rate of the peak value and peak hold value.
         */
        void setReturnRate (double returnRateDbPerSecond);

        /**
         * Sets the peak holds time of the peak hold value.
         * @param peakHoldTimeMs The time to hold in milliseconds.
         */
        void setPeakHoldTimeMs (uint32_t peakHoldTimeMs);

        /**
         * Sets given level meter as source, unsubscribing from the previously set meter (if any) and subscribing to the
         * new one.
         * @param levelMeter The level meter to subscribe to.
         */
        void setLevelMeter (LevelMeter* levelMeter);

    private:
        const Scale& mScale;
        juce::Array<ChannelData> mChannelData;
        double mReturnRateDbPerSecond = LevelMeterConstants::kDefaultReturnRate;
        int mMaxChannels = kDefaultMaxChannels;
        LevelMeter* mLevelMeter { nullptr };
    };

    LevelMeter();
    ~LevelMeter();

    JUCE_DECLARE_NON_COPYABLE (LevelMeter)
    JUCE_DECLARE_NON_MOVEABLE (LevelMeter)

    /**
     * Prepares the meter for the amount of channels given.
     * @param numChannels Number of channels to prepare for.
     */
    void prepareToPlay (int numChannels);

    /**
     * Measures a block of audio and sends the measurement to a queue.
     * Calling this method is realtime safe as long as being called from a single thread.
     * When the queue is full the measurement will be lost.
     * @tparam SampleType The type of the audio sample.
     * @param audioBuffer The audio buffer to take the measurement from.
     */
    template <typename SampleType>
    void measureBlock (const juce::AudioBuffer<SampleType>& audioBuffer);

    /**
     * Measures a block of audio and sends the measurement to a queue.
     * Calling this method is realtime safe as long as being called from a single thread.
     * When the queue is full the measurement will be lost.
     * @tparam SampleType The type of the audio sample.
     * @param inputChannelData The audio data to take the measurement from.
     * @param numChannels The number of channels.
     * @param numSamples The number of samples in the block.
     */
    template <typename SampleType>
    void measureBlock (const SampleType* const* inputChannelData, int numChannels, int numSamples);

private:
    /// Used to share a single timer across all instances of LevelMeter to synchronize them all.
    /// Should probably be replaced with a JUCE animation callback at some point.
    struct SharedTimer : juce::Timer
    {
        juce::ListenerList<LevelMeter> subscribers;

        void timerCallback() override
        {
            // Stop timer if there are no subscribers.
            if (subscribers.isEmpty())
                stopTimer();

            subscribers.call ([] (LevelMeter& s) {
                s.timerCallback();
            });
        }
    };

    /// Data cached from the call to prepareToPlay
    struct PreparedToPlayInfo
    {
        int numChannels = 2;
    } mPreparedToPlayInfo;

    /// Holds the globally shared timer.
    juce::SharedResourcePointer<SharedTimer> mSharedTimer;

    /// Holds subscribers to this level meter.
    juce::ListenerList<Subscriber> mSubscribers;

    /// Holds the available measurements.
    moodycamel::ReaderWriterQueue<Measurement> mMeasurements { 128 }; // Arbitrary amount.

    /**
     * Pushes a single measurement into the queue.
     * @param measurement The measurement to push.
     */
    void pushMeasurement (Measurement&& measurement);

    /**
     * Called by the shared timer.
     */
    void timerCallback();
};
