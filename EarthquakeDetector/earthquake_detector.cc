#include <iostream>

#include "earthquake_detector.h"

namespace earthquake_detection_unit {

const double kScaleValue1Threshold = 0.144;
const double kScaleValue2Threshold = 0.281;
const double kScaleValue3Threshold = 0.418;
const double kScaleValue4Threshold = 0.555;
const double kScaleValue5Threshold = 0.692;
const double kScaleValue6Threshold = 0.829;
const double kScaleValue7Threshold = 0.966;
const double kScaleValue8Threshold = 1.103;
const double kScaleValue9Threshold = 1.24;
const int kAccelerometerTimeoutTotal_ms = 10000;
const int kAccelerometerTimeoutPeriod_ms = 100;
const int kAccelerometerTimeoutNumPeriods = kAccelerometerTimeoutTotal_ms / kAccelerometerTimeoutPeriod_ms;

EarthquakeDetector::EarthquakeDetector() {
    std::atomic<bool> shutdown(false);

    // Initialize digit display.
    digit_display = new DigitDisplay();

    worker_thread = std::thread(&EarthquakeDetector::Worker, this);
}

EarthquakeDetector::~EarthquakeDetector() {
    shutdown = true;
    worker_thread.join();

    // Shutdown digit display.
    delete digit_display;
}

void EarthquakeDetector::Worker() {
    while (!shutdown) {
        std::cout << "Launching vibration sensor to listen for a vibration." << std::endl;
        // First, wait for a vibration.
        vibration_sensor = new VibrationSensor();
        vibration_sensor->WaitForVibration();
        delete vibration_sensor;

        // After detecting a vibration, launch accelerometer.
        std::cout << "Vibration detected -- vibration sensor shutdown, launching accelerometer." << std::endl;
        accelerometer = new Accelerometer();
        shutdown_accelerometer_monitor.store(false, std::memory_order_relaxed);
        accelerometer_monitor_thread = std::thread(&EarthquakeDetector::AccelerometerMonitor, this);

        // Keep checking if we are still detecting significant shaking via the accelerometer.
        AccelerometerMonitor();

        // Flash magnitude.
        digit_display->FlashDisplay();
        // Reset digit display to display 0.
        digit_display->SetDigit(0);

        // Shutdown accelerometer for lack of activity.
        std::cout << "Shutting down accelerometer for inactivity." << std::endl;
        shutdown_accelerometer_monitor.store(true, std::memory_order_relaxed);
        accelerometer_monitor_thread.join();
        delete accelerometer;
    }
}

void EarthquakeDetector::AccelerometerMonitor() {
    // This function returns if we obtain kAccelerometerTimeoutNumPeriods
    // consecutive low readings from the accelerometer.
    int consecutive_readings = 0;
    while (consecutive_readings != kAccelerometerTimeoutNumPeriods) {
        DisplayMagnitude();
        if (accelerometer->GetCurrentReading() >= kScaleValue1Threshold) {
            consecutive_readings = 0;
        }
        else {
            ++consecutive_readings;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kAccelerometerTimeoutPeriod_ms));
    }
}

void EarthquakeDetector::DisplayMagnitude() {
    double acc_reading = accelerometer->GetHighestReading();

    uint8_t digit_to_display = 0;
    if (acc_reading >= kScaleValue9Threshold) {
        digit_to_display = 9;
    }
    else if (acc_reading >= kScaleValue8Threshold) {
        digit_to_display = 8;
    }
    else if (acc_reading >= kScaleValue7Threshold) {
        digit_to_display = 7;
    }
    else if (acc_reading >= kScaleValue6Threshold) {
        digit_to_display = 6;
    }
    else if (acc_reading >= kScaleValue5Threshold) {
        digit_to_display = 5;
    }
    else if (acc_reading >= kScaleValue4Threshold) {
        digit_to_display = 4;
    }
    else if (acc_reading >= kScaleValue3Threshold) {
        digit_to_display = 3;
    }
    else if (acc_reading >= kScaleValue2Threshold) {
        digit_to_display = 2;
    }
    else if (acc_reading >= kScaleValue1Threshold) {
        digit_to_display = 1;
    }

    if (digit_to_display != digit_display->GetCurrentDigit()) {
        accelerometer->Pause();
        digit_display->SetDigit(digit_to_display);
        accelerometer->Unpause();
    }
}

} // earthquake_detection_unit
