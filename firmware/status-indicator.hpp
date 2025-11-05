#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#ifndef STATUS_INDICATOR_HEADER
#define STATUS_INDICATOR_HEADER

#define STATUS_SUCCESS 1
#define STATUS_ACTIVE 2
#define STATUS_WARNING 3
#define STATUS_ERROR 4
#define STATUS_IDLE 5

class StatusIndicator {
    public:
		StatusIndicator();

		void network(byte status);
		void connection(byte status);
		void permit(byte status);

	private:
		Adafruit_NeoPixel pixels;

		void setLight(int index, byte status);
};

extern StatusIndicator status;

#endif