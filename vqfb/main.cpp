/*-----------------------------------------------------------
VQ Framebuffer Example

vqfb - main.cpp
Sept. 25th, 2015

Author: Matt Slevinsky
------------------------------------------------------------*/

#include <kos.h>

#include "sample.h"
#include "simplex_vqfb.h"
#include "simplex_pal.h"
#include "fire_vqfb.h"

/****************************************************************************/
/*                           kos Initialization                             */
/****************************************************************************/
pvr_init_params_t pvr_params =  {
    /* Enable opaque and translucent polygons with size 16 */
    { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },

    /* Vertex buffer size 512K */
    512 * 1024
};

void pvr_setup() {
    pvr_init(&pvr_params);
    pvr_set_bg_color(0, 0, 0);
}

extern uint8 romdisk[];

KOS_INIT_FLAGS(INIT_DEFAULT);

/****************************************************************************/
/*                               Demo States                                */
/****************************************************************************/
typedef enum {
	MODE_SIMPLEX_NOISE_16_BPP = 0,
	MODE_SIMPLEX_NOISE_32_BPP = 1,
	MODE_FIRE_VQFB = 2,
	MODE_NONE = 3,
} SampleMode;

typedef struct SampleTable {
	SampleMode mode;
	Sample* sample;
} SampleTable;

#define NUM_SAMPLES 4

SampleTable samples[] = {
	{
		MODE_SIMPLEX_NOISE_16_BPP, new SimplexVqfb()
	},
	{
		MODE_SIMPLEX_NOISE_32_BPP, new SimplexPal()
	},
	{
		MODE_FIRE_VQFB, new FireVqfb()
	},
	{
		MODE_NONE, new Sample()
	}
};

SampleMode currentMode = MODE_SIMPLEX_NOISE_16_BPP;
Sample* currentSample;

Sample* getSample(SampleMode mode) {
	for (int i = 0;;i++) {
		if ((samples[i].mode == mode) || (samples[i].mode == MODE_NONE)) {
			return samples[i].sample;
		}
	}
}

void initialize() {
	printf("Initializing Mode\n");
	currentSample = getSample(currentMode);
	currentSample -> init();
	printf("Completed Initialization\n");
}

void update() {
	currentSample -> update();
}

void cleanup() {
	currentSample -> cleanup();
	for (int i = 0; i < NUM_SAMPLES; i++) {
		delete samples[i].sample;
	}
	currentSample = NULL;
}

void switchNextMode() {
	currentSample -> cleanup();
	for (int i = 0; i <= NUM_SAMPLES - 1; i++) {
		if (samples[i].mode == currentMode) {
			if (i != NUM_SAMPLES - 2) {
				currentMode = samples[i + 1].mode;
			} else {
				currentMode = samples[0].mode;
			}
			break;
		}
	}
	currentSample = getSample(currentMode);
	currentSample -> init();
}

void switchPrevMode() {
	currentSample -> cleanup();
	for (int i = 0; i <= NUM_SAMPLES - 1; i++) {
		if (samples[i].mode == currentMode) {
			if (i != 0) {
				currentMode = samples[i - 1].mode;
			} else {
				currentMode = samples[NUM_SAMPLES - 2].mode;
			}
			break;
		}
	}
	currentSample = getSample(currentMode);
	currentSample -> init();
}

void drawScreen() {
	currentSample -> render();
}

int main() {
#ifdef DEBUG
	gdb_init();
#endif

	maple_device_t* cont;
	cont_state_t* state;
	bool exitMainLoop = false;

	printf("Initializing PVR\n");
	pvr_setup();

	printf("Initializing\n");
	initialize();

	bool lockControl = false;

    while(!exitMainLoop) {
    	int maxControllerIndex = maple_enum_count();
    	int controllerIndex = 0;
    	for (;controllerIndex < maxControllerIndex; controllerIndex++) {
	        if ((cont = maple_enum_type(controllerIndex, MAPLE_FUNC_CONTROLLER)) != NULL) {
	            state = (cont_state_t *)maple_dev_status(cont);

	            if (state == NULL) {
	            	exitMainLoop = true;
	            }

	            if (state -> buttons & CONT_START) {
	                exitMainLoop = true;
				}

				if (!lockControl) {
					if (state -> buttons & CONT_DPAD_LEFT) {
						switchPrevMode();
						lockControl = true;
					} else if (state -> buttons & CONT_DPAD_RIGHT) {
						switchNextMode();
						lockControl = true;
					}
				} else if (!(state -> buttons & (CONT_DPAD_RIGHT | CONT_DPAD_LEFT))) {
					lockControl = false;
				}
	        }
    	}

        drawScreen();

        update();
    }

    printf("Cleaning Up\n");
    cleanup();

	return 0;
}
