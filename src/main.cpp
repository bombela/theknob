
#include <Arduino.h>

#define LED_1 D5
#define INFO_PIN D8
#define INFO_PIN2 D7

struct UpDown {
	int uvalue = 0;
	int umin = 0;
	int umax = 255;
	int ustep = 1;

	UpDown() = default;
	UpDown(int umin, int umax=255, int ustep=1):
		umin(umin), umax(umax), ustep(ustep) {}

	int iter() {
		uvalue += ustep;
		if (uvalue <= umin || uvalue >= umax) {
			ustep = -ustep;
		}
		uvalue = max(umin, min(uvalue, umax));
		return uvalue;
	}
};

template<int PIN_A, int PIN_B, int MIN_VAL, int MAX_VAL, int STEP=1>
class RotaryEncoder {
public:
	void setup() {
		pinMode(PIN_A, INPUT_PULLUP);
		pinMode(PIN_B, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(PIN_A), read_state, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_B), read_state, CHANGE);
	}

	int value() {
		return valueref();
	}

private:
	static int& valueref() {
		static int value = 0;
		return value;
	}

	static void read_state() {
		// all 4 steps for single step rotation are stored serially.
		//                          AB AB AB AB
		// Clockwise state        : 00 01 11 10
		// Counter-clockwise state: 10 11 01 00
		static uint8_t full_step = 0;

		// 0 is HIGH, 1 is LOW.
		const uint8_t a = !digitalRead(PIN_A);
		const uint8_t b = !digitalRead(PIN_B);

		const uint8_t new_state = (a << 1 | b);

		// No change with most recent state? We ignore.
		if ((full_step & 0x03) == new_state) return;

		// Appending new_state to the history.
		full_step = (full_step << 2) | new_state;

		// If we get one of the two valid states, we rotate!
		// To work reliably, the rotary encoder must be hardware debounced,
		// else we will never match anything here.
		switch (full_step) {
			case 0b00011110:
				valueref() = min(valueref() + STEP, MAX_VAL);
				break;
			case 0b10110100:
				valueref() = max(valueref() - STEP, MIN_VAL);
				break;
		};
	}
};

UpDown ud_led1(0, 1023);
RotaryEncoder<D5, D6, 0, 1023> re;

void setup() {
	Serial.begin(9600);
	Serial.printf("SETUP START\n");
	//pinMode(LED_1, OUTPUT);
	//analogWrite(LED_1, 0);
	pinMode(INFO_PIN, OUTPUT);
	analogWrite(INFO_PIN, 0);
	pinMode(INFO_PIN2, OUTPUT);
	digitalWrite(INFO_PIN2, 0);

	re.setup();
	Serial.printf("SETUP DONE\n");
}

int last_val = 0;

void loop() {
	//analogWrite(LED_1, ud_led1.iter());

	int val = re.value();
	if (val != last_val) {
		Serial.printf("-> %d\n", re.value());
	}
	last_val = val;

	delay(10);
}
