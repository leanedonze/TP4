#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>


#include <main.h>
#include <motors.h>
#include <pi_regulator.h>
#include <process_image.h>

#define	KP		0
#define	KI		0
#define GOAL	10

static THD_WORKING_AREA(waPiRegulator, 256);
static THD_FUNCTION(PiRegulator, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    systime_t time;

    int16_t speed = 0;
    float error = 0;
    float sum_error = 0;

    while(1){
        time = chVTGetSystemTime();
        error = get_distance_cm() - GOAL;
        sum_error += error;
        speed = KP*error + KI*sum_error;

        
        //applies the speed from the PI regulator
		// right_motor_set_speed(speed);
		// left_motor_set_speed(speed);

        //100Hz
        chThdSleepUntilWindowed(time, time + MS2ST(10));
    }
}

void pi_regulator_start(void){
	chThdCreateStatic(waPiRegulator, sizeof(waPiRegulator), NORMALPRIO, PiRegulator, NULL);
}
