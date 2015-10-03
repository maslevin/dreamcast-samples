/*-----------------------------------------------------------
vqfb - main.cpp
Sept. 25th, 2015

Author: Matt Slevinsky
------------------------------------------------------------*/

#include <kos.h>
#include "simplexnoise.h"

pvr_init_params_t pvr_params = {
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
KOS_INIT_ROMDISK(romdisk);

#define FRAMEBUFFER_WIDTH 256
#define FRAMEBUFFER_HEIGHT 256

//Here we define the size of a framebuffer texture we want
#define FRAMEBUFFER_PIXELS (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT)

//The codebook for a VQ texture is always 2048 bytes
#define CODEBOOK_SIZE 2048

typedef struct {
	unsigned char codebook[CODEBOOK_SIZE];
	unsigned char texture[FRAMEBUFFER_PIXELS];
} VQ_Texture;

VQ_Texture* framebuffer;
pvr_ptr_t texture;

SimplexNoise simplexNoise(1.0f, 0.85f, 2.0f, 0.5f);

void initTexture() {
	framebuffer = (VQ_Texture*)memalign(64, sizeof(VQ_Texture));

	//This creates a greyscale codebook (4 pixels of the same color, for each level of grey)
	//to use as a palette for this texture
	unsigned char* codebook = (unsigned char*)&(framebuffer -> codebook);
	uint16* codebookEntry = (uint16*)codebook;
	uint32 codebookIdx;
	for (codebookIdx = 0; codebookIdx < 256; codebookIdx++) {
		uint16 codebookValue = (((codebookIdx >> 3) << 11) |
		                        ((codebookIdx >> 2) << 5) |
		                        (codebookIdx >> 3));
		*codebookEntry++ = codebookValue;
		*codebookEntry++ = codebookValue;
		*codebookEntry++ = codebookValue;
		*codebookEntry++ = codebookValue;
	}

	//Start with color bands
	unsigned char* textureData = (unsigned char*)&(framebuffer -> texture);
	int textureIdx;
	for (textureIdx = 0; textureIdx < FRAMEBUFFER_PIXELS; textureIdx++) {
		textureData[textureIdx] = textureIdx % FRAMEBUFFER_WIDTH;
	}

	texture = pvr_mem_malloc(sizeof(VQ_Texture));
	pvr_txr_load(framebuffer, texture, sizeof(VQ_Texture));
}

float xVal = 0.0f;

void updateTexture() {
	unsigned char* pTexture = (unsigned char*)&(framebuffer -> texture);

	if (xVal == 0.0f) {
		for (int y = 0; y < FRAMEBUFFER_HEIGHT; y++) {
			for (int x = 0; x < FRAMEBUFFER_WIDTH; x++) {
				float pVal = simplexNoise.fractal(5, xVal + ((float)x / (float)FRAMEBUFFER_WIDTH) + 0.5f, ((float)y / (float)FRAMEBUFFER_WIDTH) + 0.5f);
				if ((pVal > 0.0f) && (pVal < 1.0f)) {
					*pTexture = (unsigned char)(pVal * 255.0f);
				} else if (pVal >= 1.0f) {
					*pTexture = 255;
				} else {
					*pTexture = 0;
				}
				pTexture++;
			}
		}		
	} else {
		for (int y = 0; y < FRAMEBUFFER_HEIGHT; y++) {
			for (int x = 0; x < (FRAMEBUFFER_WIDTH - 1); x++) {
				*pTexture = pTexture[1];
				pTexture++;
			}

			float pVal = simplexNoise.fractal(5, xVal + ((float)(FRAMEBUFFER_WIDTH - 1) / (float)FRAMEBUFFER_WIDTH) + 0.5f, ((float)y / (float)FRAMEBUFFER_WIDTH) + 0.5f);
			if ((pVal > 0.0f) && (pVal < 1.0f)) {
				*pTexture = (unsigned char)(pVal * 255.0f);
			} else if (pVal >= 1.0f) {
				*pTexture = 255;
			} else {
				*pTexture = 0;
			}
			pTexture++;
		}				
	}

	pvr_txr_load(framebuffer, texture, sizeof(VQ_Texture));

	xVal += (1.0f / (float)FRAMEBUFFER_WIDTH);
}

void freeTexture() {
	free (framebuffer);
	pvr_mem_free(texture);
}

void drawScreen() {
	pvr_wait_ready();
	pvr_scene_begin();

	pvr_list_begin(PVR_LIST_OP_POLY);

	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
	//lxdream expects 2x width, real hardware expects 4x width
	pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED, FRAMEBUFFER_WIDTH * 4, FRAMEBUFFER_HEIGHT, texture, PVR_FILTER_NONE);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	pvr_vertex_t my_vertex;		

	my_vertex.flags = PVR_CMD_VERTEX;
	my_vertex.x = 0.0f;
	my_vertex.y = 480.0f;
	my_vertex.z = 1.0f;
	my_vertex.u = 0.0f;
	my_vertex.v = 1.0f;
	my_vertex.argb = 0xFFFFFFFF;
	my_vertex.oargb = 0;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.y = 0.0f;
	my_vertex.v = 0.0f;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.x = 640.0f;
	my_vertex.y = 480.0f;
	my_vertex.u = 1.0f;
	my_vertex.v = 1.0f;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.flags = PVR_CMD_VERTEX_EOL;
	my_vertex.y = 0.0f;
	my_vertex.v = 0.0f;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	pvr_list_finish();

	pvr_scene_finish();	
}

int main() {
	maple_device_t* cont;
	cont_state_t* state;
	bool exitMainLoop = false;

	printf("Initializing PVR\n");
	pvr_setup();

	printf("Initializing VQ Texture\n");
	initTexture();

    while(!exitMainLoop) {
    	int maxControllerIndex = maple_enum_count();
    	int controllerIndex = 0;
    	int controllerMask = 0x1;
    	for (;controllerIndex < maxControllerIndex; controllerIndex++) {
	        if ((cont = maple_enum_type(controllerIndex, MAPLE_FUNC_CONTROLLER)) != NULL) {
	            state = (cont_state_t *)maple_dev_status(cont);

	            if (state == NULL) {
	            	exitMainLoop = true;
	            }

	            if (state->buttons & CONT_START)
	                exitMainLoop = true;
	        }
		    controllerMask <<= 1;
    	}

        drawScreen();

        updateTexture();
    }

    printf("Unloading texture\n");
    freeTexture();

	return 0;
}
