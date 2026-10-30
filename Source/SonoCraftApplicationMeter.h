// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
#pragma once
#include <atomic>
#include <algorithm>
#include <cmath>

// A measurement-only sink: no PCM queue, playback or network route. One producer
// accumulates peaks; the discovery thread consumes them once per display tick.
class SonoCraftApplicationMeter {
public:
    enum class State { starting, active, failed, stopped };
    struct Reading { float left = 0, right = 0; State state = State::starting; };
    std::atomic<State> state { State::starting };
    void feed(const float* left, const float* right, int samples) {
        if (state.load() != State::active) return;
        accumulate(leftPeak, peak(left, samples));
        accumulate(rightPeak, peak(right, samples));
    }
    Reading read() {
        const auto current = state.load();
        const auto left = leftPeak.exchange(0), right = rightPeak.exchange(0);
        return current == State::active ? Reading{left, right, current} : Reading{0, 0, current};
    }
private:
    std::atomic<float> leftPeak {0}, rightPeak {0};
    static float peak(const float* data, int samples) {
        float result = 0;
        for (int i = 0; data && i < samples; ++i)
            if (std::isfinite(data[i])) result = std::max(result, std::min(1.0f, std::abs(data[i])));
        return result;
    }
    static void accumulate(std::atomic<float>& target, float value) {
        auto previous = target.load();
        while (value > previous && !target.compare_exchange_weak(previous, value)) {}
    }
};
