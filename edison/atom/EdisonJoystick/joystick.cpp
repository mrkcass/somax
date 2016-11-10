#include <pthread.h>
#include <stdio.h>
#include "unistd.h"


#include "joystick.h"

enum
{
	RUNSTATE_RUN,
	RUNSTATE_RUNNING,
	RUNSTATE_STOP,
	RUNSTATE_STOPPING,
	RUNSTATE_STOPPPED,
};

static void * loop(void * data);

Joystick::Joystick()
{
	x_val = y_val = -1;
	button_val = 0;
	x_wire = y_wire = NULL;
	runState = RUNSTATE_STOPPPED;
}

Joystick::~Joystick()
{
	if (x_wire)
		delete x_wire;
	if (y_wire)
		delete y_wire;
}

int Joystick::connect(int xAxisPin, int yAxisPin, int buttonPin)
{
	x_wire = new Wire();
	x_wire->connectAnalog(xAxisPin);
	y_wire = new Wire();
	y_wire->connectAnalog(yAxisPin);
	button_wire = new Wire();
	button_wire->connectDigital(buttonPin, WIRE_DIRECTION_IN, WIRE_MODE_PULLUP);
	return 0;
}

int Joystick::run(int _pollInterval)
{
	pollInterval = _pollInterval;
	//start the motor thread. plays motor scripts.
	runState = RUNSTATE_RUN;
	int iret1 = pthread_create(&loopThread, NULL, loop, (void*)this);
	if (iret1)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", iret1);
	}
	return 0;
}

int Joystick::stop()
{
	return 0;
}

int Joystick::read(int source)
{
	if (source == JOYSTICK_SRC_AXIS_X)
		return x_val;
	else if (source == JOYSTICK_SRC_AXIS_Y)
		return y_val;
	else if (source == JOYSTICK_SRC_BUTTON)
		return button_val;

	return -1;
}

static void * loop(void * data)
{
	Joystick * jstick = (Joystick*)data;

	jstick->runState = RUNSTATE_RUNNING;
	while (jstick->runState < RUNSTATE_STOP)
	{
		usleep(jstick->pollInterval);
		jstick->x_val = jstick->x_wire->read();
		jstick->y_val = jstick->y_wire->read();
		jstick->button_val = jstick->button_wire->read();
	}
	
	jstick->runState = RUNSTATE_STOPPPED;

	return NULL;
}

