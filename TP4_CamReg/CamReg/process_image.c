#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>

#define THRESHOLD	15


static float distance_cm = 0;

//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE);

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 10 + 11 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 10, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();

    while(1){
    	//systime_t time;
    	//time = chVTGetSystemTime();
    	//chThdSleepMilliseconds(12);
        //starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		//chprintf((BaseSequentialStream *)&SDU1, "time = %d \n", chVTGetSystemTime()-time);
		//signals an image has been captured
		chBSemSignal(&image_ready_sem);

    }
}


static THD_WORKING_AREA(waProcessImage, 1024);
static THD_FUNCTION(ProcessImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = {0};
	uint16_t pixel_counter = 0;
	uint8_t envoi=0;

    while(1){
    	//waits until an image has been captured
        chBSemWait(&image_ready_sem);
		//gets the pointer to the array filled with the last image in RGB565    
		img_buff_ptr = dcmi_get_last_image_ptr();
		for (unsigned int i = 0; i < 2*IMAGE_BUFFER_SIZE; i+=2)
		{
			image[i/2] = img_buff_ptr[i] & (0b11111000);
			if (image[i/2] < THRESHOLD)
				++pixel_counter;

			/*if (image[i/2-1] - image[i/2] > 50){
				pixel_counter = i/2;
			}
			if (image[i/2]-image[i/2-1] > 50){
				pixel_counter = i/2-pixel_counter;
			}*/

		}
		/*for (unsigned int i = 0; i < IMAGE_BUFFER_SIZE; ++i){
			if (image[i] - image[i+10] > 50){
				pixel_counter = i;
			}

			if (image[i+10] - image[i] > 50){
				pixel_counter = i - pixel_counter;
			}
		}*/
		if (envoi%4==0)
			SendUint8ToComputer(image, IMAGE_BUFFER_SIZE);
		++envoi;
		chprintf((BaseSequentialStream *)&SDU1, "largeur en pixels = %d \n", pixel_counter);
		pixel_counter = 0;

	}
   }


float get_distance_cm(void){
	return distance_cm;
}

void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}


