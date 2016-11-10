#ifndef __joystick_h__
#define __joystick_h__

#include "wire.h"

#define JOYSTICK_SRC_AXIS_X 1
#define JOYSTICK_SRC_AXIS_Y 2
#define JOYSTICK_SRC_BUTTON 3

#define JOYSTICK_RANGE 1000

struct Joystick
{

	Joystick();
	~Joystick();
	int connect(int xAxisPin, int yAxisPin, int buttonPin);
	int run(int poll_interval);
	int stop();
	int read(int axis);

	int x_val;
	int y_val;
	int button_val;

	//private:
		Wire * x_wire;
		Wire * y_wire;
		Wire * button_wire;
		int runState;
		pthread_t loopThread;
		int pollInterval;
};

#endif
