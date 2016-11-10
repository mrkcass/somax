
#include "steppermotor.h"
#include "mcu_api.h"
#include "stdlib.h"
#include "string.h"

int do_command (StepperMotor * motor_x, StepperMotor * motor_y, char * cmd, int cmd_len);
void cycleWires ();
void run();
int nextRunDirectionX = StepperMotor_DIR_CW;
int nextRunDirectionY = StepperMotor_DIR_CW;

void mcu_main()
{
	cycleWires();
	run();
}

/*
arduino breakout board pen mapping
IO0  130
IO1  131
IO2  128 : R1
IO3  12  : R2
IO4  129 : R3
IO5  13  : R4
IO6  182 : L3
IO7  48  : L1/2
IO8  49  : L1/L2
IO9  183 : L4
IO10 41
IO11 43
IO12 42
IO13 40
IO14 44
IO15 45
IO16 46
IO17 47
IO18 14
IO19 165
*/
#define NUM_TO_CYCLE 8
static Wire wires[NUM_TO_CYCLE];
static Wire * wireptrStart = wires;
static Wire * wireptrEnd = wires+(NUM_TO_CYCLE-1);
static Wire * j;

void cycleWires ()
{
	int wire_pins[NUM_TO_CYCLE] = {41,43,42,40,128,12,129,13};

	int sleep_ticks=5;
	int sleep_ms=sleep_ticks*10;
	int i = 0;
	for (j = wireptrStart; j <= wireptrEnd; j++, i++)
	{
		Wire_connect (j, wire_pins[i], WIRE_DIRECTION_OUT);
		Wire_write(j, 0);
	}
	while (1)
	{
		for (j = wireptrStart; j <= wireptrEnd; j++)
		{
			Wire_write(j, 1);
			mcu_sleep(sleep_ms);
			Wire_write(j, 0);
		}
	}
}

#define NUM_MOTORS 2
#define X_AXIS 0
#define Y_AXIS 1
#define X_AXIS_MOTOR (&motors[X_AXIS])
#define Y_AXIS_MOTOR (&motors[Y_AXIS])

void run()
{
	int len;
	char buf[32];
	//StepperMotor * motor = StepperMotor_StepperMotor(2,4,7,8);
	//StepperMotor * motor = StepperMotor_StepperMotor(128,129,48,49);
	//x-axis motor
	StepperMotor motors[2];
	StepperMotor_StepperMotor(X_AXIS_MOTOR, 182, 48, 49, 183);
	//y-axis motor
	StepperMotor_StepperMotor(Y_AXIS_MOTOR, 128, 12, 129, 13);

	StepperMotor_powerOn(X_AXIS_MOTOR);
	StepperMotor_stop(X_AXIS_MOTOR, false);
	StepperMotor_powerOn(Y_AXIS_MOTOR);
	StepperMotor_stop(Y_AXIS_MOTOR, false);
	debug_print (DBG_INFO, "Motor is running\n");
	while (1)
	{
		len = host_receive((unsigned char *)buf, 64);
		while (len > 0)
		{
			if (buf[len] == '\n')
			{
				buf[len] = '\0';
				char pbuf[64];
				mcu_snprintf(pbuf, 64, "host receive:%d: %s\n", len, buf);
				debug_print (DBG_INFO, pbuf);
				do_command(X_AXIS_MOTOR, Y_AXIS_MOTOR, buf, len);
				break;
			}
			len--;
		}
		StepperMotor_loop(X_AXIS_MOTOR);
		StepperMotor_loop(Y_AXIS_MOTOR);
		mcu_delay(1);
	}

	StepperMotor_powerOff(X_AXIS_MOTOR);
	StepperMotor_powerOff(Y_AXIS_MOTOR);
}

int do_command (StepperMotor * motor_x, StepperMotor * motor_y, char * cmd, int cmd_len)
{
	int processed = 0;
	static int direction;

	#define XPOS_STR "x_positive"
	#define XPOS_STR_LEN (sizeof(XPOS_STR)-1)
	#define XNEG_STR "x_negative"
	#define XNEG_STR_LEN (sizeof(XNEG_STR)-1)

	#define YPOS_STR "y_positive"
	#define YPOS_STR_LEN (sizeof(YPOS_STR)-1)
	#define YNEG_STR "y_negative"
	#define YNEG_STR_LEN (sizeof(YNEG_STR)-1)

	#define RUN_STR "run"
	#define RUN_STR_LEN (sizeof(RUN_STR)-1)

	#define STOP_STR "stop"
	#define STOP_STR_LEN (sizeof(STOP_STR)-1)

	#define MODE_STR "toggle_mode"
	#define MODE_STR_LEN (sizeof(MODE_STR)-1)

	#define STEPF_STR "step_forward"
	#define STEPF_STR_LEN (sizeof(STEPF_STR)-1)

	#define STEPB_STR "step_backward"
	#define STEPB_STR_LEN (sizeof(STEPB_STR)-1)

	if (!processed && cmd_len == XPOS_STR_LEN && !strncmp(cmd, XPOS_STR, XPOS_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: xpos command\n");
		if (StepperMotor_running(motor_x))
		{
			StepperMotor_faster(motor_x);
		}
		else
		{
			StepperMotor_step(motor_x, StepperMotor_DIR_CW);
			nextRunDirectionX = StepperMotor_DIR_CW;
		}
		processed = 1;
	}
	else if (!processed && cmd_len == XNEG_STR_LEN && !strncmp(cmd, XNEG_STR, XNEG_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: xneg command\n");
		if (StepperMotor_running(motor_x))
		{
			StepperMotor_slower(motor_x);
		}
		else
		{
			StepperMotor_step(motor_x, StepperMotor_DIR_CCW);
			nextRunDirectionX = StepperMotor_DIR_CCW;
		}
		processed = 1;
	}
	else if (!processed && cmd_len == YPOS_STR_LEN && !strncmp(cmd, YPOS_STR, YPOS_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: ypos command\n");
		if (StepperMotor_running(motor_y))
		{
			StepperMotor_faster(motor_y);
		}
		else
		{
			StepperMotor_step(motor_y, StepperMotor_DIR_CW);
			nextRunDirectionY = StepperMotor_DIR_CW;
		}
		processed = 1;
	}
	else if (!processed && cmd_len == YNEG_STR_LEN && !strncmp(cmd, YNEG_STR, YNEG_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: yneg command\n");
		if (StepperMotor_running(motor_y))
		{
			StepperMotor_slower(motor_y);
		}
		else
		{
			StepperMotor_step(motor_y, StepperMotor_DIR_CCW);
			nextRunDirectionY = StepperMotor_DIR_CCW;
		}
		processed = 1;
	}
	else if (!processed && cmd_len == STOP_STR_LEN && !strncmp(cmd, STOP_STR, STOP_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: stop command\n");
		StepperMotor_stop(motor_x, false);
		StepperMotor_stop(motor_y, false);
		processed = 1;
	}
	else if (!processed && cmd_len == RUN_STR_LEN && !strncmp(cmd, RUN_STR, RUN_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: run command\n");
		if (!direction)
		{
			StepperMotor_run(motor_x, StepperMotor_DIR_CW);
			StepperMotor_run(motor_y, StepperMotor_DIR_CW);
			direction = 1;
		}
		else
		{
			StepperMotor_run(motor_x, StepperMotor_DIR_CCW);
			StepperMotor_run(motor_y, StepperMotor_DIR_CCW);
			direction = 0;
		}
		processed = 1;
	}
	else if (!processed && cmd_len == MODE_STR_LEN && !strncmp(cmd, MODE_STR, MODE_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: mode command\n");
		StepperMotor_toggleStepMode(motor_x);
		StepperMotor_toggleStepMode(motor_y);
		processed = 1;
	}

	else if (!processed && cmd_len == STEPF_STR_LEN && !strncmp(cmd, STEPF_STR, STEPF_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: step forward command\n");
		StepperMotor_step (motor_y, StepperMotor_DIR_CW);
		processed = 1;
	}

	else if (!processed && cmd_len == STEPB_STR_LEN && !strncmp(cmd, STEPB_STR, STEPB_STR_LEN))
	{
		debug_print (DBG_INFO, "DO: step backward command\n");
		StepperMotor_step (motor_y, StepperMotor_DIR_CCW);
		processed = 1;
	}


	return processed;
}