#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "joystick.h"

//#define KEYPRESS_ENABLED 1

struct termios orig_termios;

static char XPOS_STR[] = "x_positive\n";
static char XNEG_STR[] = "x_negative\n";
static char YPOS_STR[] = "y_positive\n";
static char YNEG_STR[] = "y_negative\n";
static char RUN_STR[] = "run\n";
static char STOP_STR[] = "stop\n";
static char MODE_STR[] = "toggle_mode\n";
static char STEPF_STR[] = "step_forward\n";
static char STEPB_STR[] = "step_backward\n";

#ifdef KEYPRESS_ENABLED
void reset_terminal_mode();
void set_conio_terminal_mode();
int kbhit();
int getch();
#endif
int send_to_mcu(char * msg, int msg_len);

int main(int argc, char * argv[])
{
	Joystick jstick;
	int tolerance = JOYSTICK_RANGE / 10;
	bool motor_running = false;
	bool button_pressed = false;

	mraa_init();

	jstick.connect(0, 1, 10);
	jstick.run(1000 * 10);

#ifdef KEYPRESS_ENABLED
	//system("/bin/stty raw");
#endif
	int x_val = JOYSTICK_RANGE / 2;
	int y_val = JOYSTICK_RANGE / 2;

#ifdef KEYPRESS_ENABLED
	printf("\n\rJoystick is running (q to exit)\n\r");
	fflush(stdin);
#endif

#ifdef KEYPRESS_ENABLED
	set_conio_terminal_mode();
#endif
	send_to_mcu(STOP_STR, strlen(STOP_STR));
	usleep(100 * 1000);
	int center = JOYSTICK_RANGE / 2;
	int next_x_up = center + tolerance;
	int next_x_down = center - tolerance;
	int next_y_up = center + tolerance;
	int next_y_down = center - tolerance;

	while (1)
	{
#ifdef KEYPRESS_ENABLED
		if (kbhit())
		{
			int c = getch();
			//printf("\r \r");
			if (c == 'q')
			{
				send_to_mcu(STOP_STR, STOP_STR_LEN);
				break;
			}
		}
		else
		{
#endif
			x_val = JOYSTICK_RANGE - jstick.read(JOYSTICK_SRC_AXIS_X);
			y_val = JOYSTICK_RANGE - jstick.read(JOYSTICK_SRC_AXIS_Y);
			int new_button = jstick.read(JOYSTICK_SRC_BUTTON);

			if (!button_pressed && new_button == 0)
			{
				button_pressed = true;
				if (motor_running)
				{
					send_to_mcu(STOP_STR, strlen(STOP_STR));
					motor_running = false;
				}
				else
				{
					send_to_mcu(RUN_STR, strlen(RUN_STR));
					motor_running = true;
				}

			}
			else if (button_pressed && new_button == 1)
			{
				button_pressed = false;
			}
			else
			{
				if (x_val > next_x_up)
				{
					printf("\n\rXUP: J_X=%4d J_Y=%4d, nxd=%4d nxu=%4d", x_val, y_val, next_x_down, next_x_up);
					send_to_mcu(XPOS_STR, strlen(XPOS_STR));
					next_x_up += tolerance;
					next_x_down = center - tolerance;
				}
				else if (x_val < next_x_down)
				{
					printf("\n\rXDN: J_X=%4d J_Y=%4d, nxd=%4d nxu=%4d", x_val, y_val, next_x_down, next_x_up);
					send_to_mcu(XNEG_STR, strlen(XNEG_STR));
					next_x_down -= tolerance;
					next_x_up = center + tolerance;
				}

				if (y_val > next_y_up)
				{
					printf("\n\rYUP: J_X=%4d J_Y=%4d, nyd=%4d nyu=%4d", x_val, y_val, next_y_down, next_y_up);
					send_to_mcu(YPOS_STR, strlen(YPOS_STR));
					next_y_up += tolerance;
					next_y_down = center - tolerance;
				}
				else if (y_val < next_y_down)
				{
					printf("\n\rYDN: J_X=%4d J_Y=%4d, nyd=%4d nyu=%4d", x_val, y_val, next_y_down, next_y_up);
					send_to_mcu(YNEG_STR, strlen(YNEG_STR));
					next_y_down -= tolerance;
					next_y_up = center + tolerance;
				}
			}
			usleep(jstick.pollInterval);
#ifdef KEYPRESS_ENABLED
		}
#endif
	}
#ifdef KEYPRESS_ENABLED
	//reset terminal to wait for enter key
	//system("/bin/stty cooked");
#endif
	mraa_deinit();
	return 0;
}

int send_to_mcu(char * msg, int msg_len)
{
	int mcu_fd = open("/dev/ttymcu0", O_RDWR | O_NOCTTY);
	if (mcu_fd == -1)
	{
		printf("open ttymcu1 failed!\n");
		return 1;
	}
	else
	{
		write(mcu_fd, msg, msg_len);
		close(mcu_fd);
	}
	return 0;
}

#ifdef KEYPRESS_ENABLED
void reset_terminal_mode()
{
	tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
	struct termios new_termios;

	/* take two copies - one for now, one for later */
	tcgetattr(0, &orig_termios);
	memcpy(&new_termios, &orig_termios, sizeof(new_termios));

	/* register cleanup handler, and set the new terminal mode */
	atexit(reset_terminal_mode);
	cfmakeraw(&new_termios);
	tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
	int r;
	unsigned char c;
	if ((r = read(0, &c, sizeof(c))) < 0) {
		return r;
	}
	else {
		return c;
	}
}
#endif