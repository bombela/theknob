class SineWave {
public:
	SineWave(double low, double high, double step = 1.0);
	double Step();

private:
	double step_;
	double degrees_;
	double low_;
	double high_;
};
