#if ARDUINO >= 100
 #include "Arduino.h"
 #include "Print.h"
#else
 #include "WProgram.h"
#endif

#define JT             (9.80665F)              /**< Earth's gravity in m/s^2 */

class BPI {
public:
	static const int *clrscr[];
};
