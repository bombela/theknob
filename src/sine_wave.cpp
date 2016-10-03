#include <cmath>

#include "sine_wave.hpp"

namespace {
double PI = 3.14159265;
};

SineWave::SineWave(double low, double high, double step)
    : low_(low), high_(high), step_(step) {}

double SineWave::Step() {
	degrees_ = degrees_ + step_;
	if (degrees_ >= 360.0) {
		degrees_ = 0;
	}
	double scale = sin(degrees_ * PI / 180.0);
	return low_ + scale * (high_ - low_);
}
