#include <ESP8266WiFi.h>
#include <Arduino.h>

#include "sine_wave.hpp"

#define SW_LED_PWM    D7
#define SW_LED_VBOOST D3
#define SW_STATE      D2
#define ROTENC_A      D5
#define ROTENC_B      D6

struct UpDown {
	int uvalue = 0;
	int umin = 0;
	int umax = 255;
	int ustep = 1;

	UpDown() = default;
	UpDown(int umin, int umax = 255, int ustep = 1)
	    : umin(umin), umax(umax), ustep(ustep) {}

	int iter() {
		uvalue += ustep;
		if (uvalue <= umin || uvalue >= umax) {
			ustep = -ustep;
		}
		uvalue = max(umin, min(uvalue, umax));
		return uvalue;
	}
};

template <int PIN_A,
          int PIN_B,
          int MIN_VAL,
          int MAX_VAL,
          int STEP_MIN = 1,
          int STEP_MAX = 16>
class RotaryEncoder {
public:
	void setup() {
		pinMode(PIN_A, INPUT_PULLUP);
		pinMode(PIN_B, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(PIN_A), read_state, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_B), read_state, CHANGE);
	}

	int value() { return valueref(); }

private:
	static int& valueref() {
		static int value = 0;
		return value;
	}

	static int step() {
		constexpr unsigned int TIME_RANGE_MS = 100;
		constexpr int STEP_RANGE = STEP_MAX - STEP_MIN;

		if (STEP_RANGE <= 0) {
			return STEP_MIN;
		}

		static unsigned int last_time_ms = 0;

		unsigned int now_ms = millis();
		const int time_diff = min(TIME_RANGE_MS, now_ms - last_time_ms);
		int cur_step = STEP_MAX - (time_diff * STEP_RANGE / TIME_RANGE_MS);

		last_time_ms = now_ms;
		return cur_step;
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
		if ((full_step & 0x03) == new_state)
			return;

		// Appending new_state to the history.
		full_step = (full_step << 2) | new_state;

		// If we get one of the two valid states, we rotate!
		// To work reliably, the rotary encoder must be hardware debounced,
		// else we will never match anything here.
		switch (full_step) {
			case 0b00011110: {
				const int v = valueref() + step();
				valueref() = min(v, MAX_VAL);
				break;
			}
			case 0b10110100: {
				const int v = valueref() - step();
				valueref() = max(v, MIN_VAL);
				break;
			}
		};
	}
};

void connect_wifi() {
	Serial.printf("Connecting wifi...\n");
	WiFi.mode(WIFI_STA);

	WiFi.begin("ssid", "pass");

	// Use the WiFi.status() function to check if the ESP8266
	// is connected to a WiFi network.
	while (WiFi.status() != WL_CONNECTED) {
		delay(100);
	}
	Serial.printf("WIFI CONNECTED (%s)\n", WiFi.localIP().toString().c_str());
}

//RotaryEncoder<ROTENC_A, ROTENC_B, 0, 1023, 1, 42> rot_enc;
RotaryEncoder<ROTENC_A, ROTENC_B, 0, 511, 1, 21> rot_enc;
//WiFiServer server(80);

void setup() {
	Serial.begin(9600);
	Serial.printf("SETUP START\n");

	pinMode(SW_LED_PWM, OUTPUT);
	pinMode(SW_LED_VBOOST, OUTPUT);
	pinMode(SW_STATE, INPUT_PULLUP);

	analogWrite(SW_LED_PWM, 0);
	digitalWrite(SW_LED_VBOOST, 0);
	rot_enc.setup();

	// connect_wifi();
	// server.begin();

	Serial.printf("SETUP DONE\n");
}

void loop() {
	static int blinks = 2;
	if (blinks-- > 0) {
		Serial.printf("BLINK BLINK %d\n", blinks);

		analogWrite(SW_LED_PWM, 255);
		delay(250);
		analogWrite(SW_LED_PWM, 0);
		delay(250);
		analogWrite(SW_LED_PWM, 255);
		delay(250);
		analogWrite(SW_LED_PWM, 0);
		delay(250);


		analogWrite(SW_LED_PWM, 255);
		digitalWrite(SW_LED_VBOOST, 1);
		delay(250);
		analogWrite(SW_LED_PWM, 0);
		delay(250);
		analogWrite(SW_LED_PWM, 255);
		delay(100);
		digitalWrite(SW_LED_VBOOST, 0);
		delay(150);
		analogWrite(SW_LED_PWM, 0);

		if (blinks > 0) {
			delay(1000);
		} else {
			Serial.printf("LET'S GO!\n");
		}
		return;
	}

	int sw_state = digitalRead(SW_STATE);

	static int led_pwm_val = 0;
	{
		static int last_val = 0;
		int val = rot_enc.value();
		if (val != last_val) {
			Serial.printf("-> %d\n", rot_enc.value());
			digitalWrite(SW_LED_VBOOST, (val >= 256));
			led_pwm_val = val % 256;
		}
		last_val = val;
	}

	static int blink = 0;
	if (sw_state) {
		blink = !blink;
	} else {
		blink = 0;
	}

	if (blink) {
		analogWrite(SW_LED_PWM, 0);
	} else {
		analogWrite(SW_LED_PWM, led_pwm_val);
	}

	delay(10);
	return;

#if 0
	WiFiClient client = server.available();
	if (!client) {
		return;
	}

	String req = client.readStringUntil('\r');
	Serial.println(req);
	client.flush();

	// Match the request
	int val = -1;  // We'll use 'val' to keep track of both the
	// request type (read/set) and value if set.
	if (req.indexOf("/led/0") != -1)
		val = 0;  // Will write LED low
	else if (req.indexOf("/led/1") != -1)
		val = 1;  // Will write LED high
	else if (req.indexOf("/knob") != -1)
		val = 2;  // Will write LED high
	// Otherwise request will be invalid. We'll say as much in HTML

	// Set GPIO5 according to the request
	if (val >= 0)
		digitalWrite(LED_PIN, !val);

	client.flush();

	// Prepare the response. Start with the common header:
	String s = "HTTP/1.1 200 OK\r\n";
	s += "Content-Type: text/html\r\n";
	// s += "Transfer-Encoding: chunked\r\n";
	s += "\r\n";
	s += "<!DOCTYPE HTML>\r\n<html>\r\n";
	// If we're setting the LED, print out a message saying we did
	if (val == 2) {
		client.print(s);
		client.setNoDelay(true);
		while (client.connected()) {
			static int last_val = 0;
			int val = re.value();
			if (val != last_val) {
				Serial.printf("-> %d\n", re.value());
				client.printf("-> %d<br>\r\n", re.value());
				last_val = val;
				client.flush();
			}
			delay(10);
		}
	}
	if (val >= 0 and val < 2) {
		s += "LED is now ";
		s += (val) ? "on" : "off";
	} else {
		s += "Invalid Request.<br> Try /led/1, /led/0, or /knob.";
	}
	s += "</html>\n";

	if (val != 2) {
		// Send the response to the client
		client.print(s);
	}
	delay(1);
	Serial.println("Client disconnected");

	delay(10);
#endif
}
