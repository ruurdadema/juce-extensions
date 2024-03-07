#pragma once

#include <cstdint>

#include "LevelPeakValue.h"
#include "rdk/util/SubscriberList.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <rdk/detail/NonCopyable.h>
#include <readerwriterqueue/readerwriterqueue.h>

/**
 * A level meter class which can be fed measurements from a realtime audio thread and be read from another (UI) thread.
 */
class LevelMeter : rdk::NonCopyable
{
public:
    /// The refresh rate of the meter.
    static constexpr int kRefreshRateHz = 30;

    /// The amount of time in milliseconds the peak hold has to wait before declining.
    static constexpr uint32_t kPeakHoldValueTimeMs = 2000;

    /// The level which triggers the overload indication
    static constexpr float kOverloadTriggerLevel = 1.f;

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
        /// The default minus infinity.
        constexpr static const double kDefaultMinusInfinityDb = -96.0;

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
        double mMinusInfinityDb { kDefaultMinusInfinityDb }; // TODO: I don't think we need this

        /// Stores al the levels for each division.
        std::vector<double> mDivisions;
    };

    /**
     * Baseclass for other classes which need to receives measurement updates.
     */
    class Subscriber
    {
    public:
        /**
         * Struct for holding measurement data per channel.
         */
        struct ChannelData
        {
            LevelPeakValue<double> peakLevel;
            LevelPeakValue<double> peakHoldLevel;
            bool overloaded = false;
        };

        /**
         * Constructor
         * @param scale The scale to use.
         */
        explicit Subscriber (const Scale& scale);

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
         * Called when all measurements have been process inside the timer callback.
         * Use this method to schedule any updates of UI.
         */
        virtual void measurementUpdatesFinished() {}

        /**
         * Resets the current data to zero (or -inf) and calls measurementUpdatesFinished() to allow the subscriber to
         * update itself.
         */
        void reset();

    protected:
        /**
         * Subscribes this subscriber to given level meter. This will unsubscribe a previous subscription.
         * @param levelMeter The level meter to subscribe to.
         */
        void subscribeToLevelMeter (LevelMeter& levelMeter);

        /**
         * Unsubscribes this subscriber from the current level meter. If not subscribed currently this method will have
         * no effect.
         */
        void unsubscribeFromLevelMeter();

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
         *
         * @param channelIndex The index of the channel to get the value for.
         * @return The current peak hold value for given channel index.
         */
        double getPeakHoldValue (int channelIndex);

        /**
         * @param channelIndex The channel index.
         * @return True if the signal was overloaded, or false if not.
         */
        [[nodiscard]] bool getOverloaded (int channelIndex) const;

        /**
         * Turns off the overloaded flag.
         */
        void resetOverloaded();

        /**
         * @return The current scale for this subscriber.
         */
        const Scale& getScale();

        /**
         * @return The amount of configured channels.
         */
        [[nodiscard]] int getNumChannels() const;

    private:
        const Scale& mScale;
        rdk::Subscription mSubscription;
        juce::Array<ChannelData> mChannelData;
    };

    LevelMeter();
    ~LevelMeter();

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
     */
    template <typename SampleType>
    void measureBlock (const SampleType* const* inputChannelData, int numChannels, int numSamples);

private:
    /**
     * A timer which is used by all instances of LevelMeter to synchronize all repaints. This keeps the meters steady.
     */
    class SharedTimer : private juce::Timer
    {
    public:
        SharedTimer() = default;
        ~SharedTimer() override
        {
            stopTimer(); // Paranoia.
        }

        /**
         * Subscribes given level meter to this timer.
         * @param levelMeter The level meter to subscribe.
         */
        void subscribe (LevelMeter& levelMeter)
        {
            // Set the timer going if we're about to subscribe the first subscriber.
            if (mSubscribers.getNumSubscribers() == 0)
                startTimerHz (kRefreshRateHz);

            levelMeter.mSharedTimerSubscription = mSubscribers.subscribe (&levelMeter);
        }

    private:
        rdk::SubscriberList<LevelMeter> mSubscribers;

        void timerCallback() override
        {
            // Stop timer if there are no subscribers.
            if (mSubscribers.getNumSubscribers() == 0)
                stopTimer();

            mSubscribers.call ([] (LevelMeter& s) {
                s.timerCallback();
            });
        }
    };

    /// Data cached from the call to prepareToPlay
    struct PreparedToPlayInfo
    {
        int numChannels = 0;
    } mPreparedToPlayInfo;

    /// Holds subscribers to this level meter.
    rdk::SubscriberList<Subscriber> mSubscribers;

    /// Holds the available measurements.
    moodycamel::ReaderWriterQueue<Measurement> mMeasurements { 100 }; // Arbitrary amount.

    /// Holds the globally shared timer.
    juce::SharedResourcePointer<SharedTimer> mSharedTimer;

    /// Holds the subscription to the shared timer.
    rdk::Subscription mSharedTimerSubscription;

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