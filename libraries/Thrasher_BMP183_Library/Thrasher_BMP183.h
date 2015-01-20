
#include <Adafruit_BMP183_U.h>

class Thrasher_BMP183 : public Adafruit_BMP183_Unified
{
  public:
    Adafruit_BMP183_Unified(int8_t SPICLK, int8_t SPIMISO, int8_t SPIMOSI, int8_t SPICS, int32_t sensorID = -1);
    Adafruit_BMP183_Unified(int8_t SPICS, int32_t sensorID = -1);
  
    float  getTemperatureF();
}