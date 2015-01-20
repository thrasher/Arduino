#include "Thrasher_BMP183.h"

float Adafruit_BMP183_Unified::getTemperature(void)
{
	float tempF = getTemperature() * 9 / 5 + 32;

	return tempF;
}
