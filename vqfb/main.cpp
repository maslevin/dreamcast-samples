/*-----------------------------------------------------------
vqfb - main.cpp
Sept. 25th, 2015

Author: Matt Slevinsky
------------------------------------------------------------*/

#include <kos.h>
#include <math.h>
#include "simplexnoise.h"

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

void updateTexture() {
	unsigned char* pTexture = (unsigned char*)&(framebuffer -> texture);

	int yIndex;
	int xIndex;
	
	for (yIndex = FRAMEBUFFER_HEIGHT; yIndex > 1; yIndex--) {
		for (xIndex = 0; xIndex < FRAMEBUFFER_WIDTH; xIndex++) {
			int framebufferIndex = yIndex * FRAMEBUFFER_WIDTH + xIndex;
			float accumulator = pTexture[(yIndex - 1) * FRAMEBUFFER_WIDTH + (xIndex - 1 + FRAMEBUFFER_WIDTH) % FRAMEBUFFER_WIDTH] +
				pTexture[(yIndex - 1) * FRAMEBUFFER_WIDTH + (xIndex + FRAMEBUFFER_WIDTH) % FRAMEBUFFER_WIDTH] +
				pTexture[(yIndex - 1) * FRAMEBUFFER_WIDTH + (xIndex + 1 + FRAMEBUFFER_WIDTH) % FRAMEBUFFER_WIDTH] +
				pTexture[(yIndex - 2) * FRAMEBUFFER_WIDTH + (xIndex + FRAMEBUFFER_WIDTH) % FRAMEBUFFER_WIDTH];
			pTexture[framebufferIndex] = (unsigned char)floor(accumulator / 4.0125f);
		}
	}

	pvr_txr_load(framebuffer, texture, sizeof(VQ_Texture));

	yIndex = FRAMEBUFFER_WIDTH;
	for (xIndex = 0; xIndex < FRAMEBUFFER_WIDTH; xIndex++) {
		pTexture[yIndex + xIndex] = rand() % 255;
	}
}

float hue2rgb(float p, float q, float t) {
	if(t < 0.0f) t += 1.0f;
	if(t > 1.0f) t -= 1.0f;
	if(t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
	if(t < 1.0f/2.0f) return q;
	if(t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
	return p;
}

uint16 hslToRgb(float hue, float saturation, float luminance) {
	float r,g,b;

    if (saturation == 0.0f){
        r = g = b = luminance; // achromatic
    } else {

        float q = luminance < 0.5f ? luminance * (1.0f + saturation) : luminance + saturation - luminance * saturation;
        float p = 2.0f * luminance - q;
        r = hue2rgb(p, q, hue + 1.0f/3.0f);
        g = hue2rgb(p, q, hue);
        b = hue2rgb(p, q, hue - 1.0f/3.0f);
    }

	uint16 red = round(r * 255);
	uint16 green = round(g * 255);
	uint16 blue = round(b * 255);

	return (red >> 3) << 11 | 
	       (green >> 2) << 5 | 
		   (blue >> 3);
} 

void initTexture() {
	framebuffer = (VQ_Texture*)memalign(64, sizeof(VQ_Texture));

	//This creates a greyscale codebook (4 pixels of the same color, for each level of red)
	//to use as a palette for this texture
	unsigned char* codebook = (unsigned char*)&(framebuffer -> codebook);
	uint16* codebookEntry = (uint16*)codebook;
	uint32 codebookIdx;
	for (codebookIdx = 0; codebookIdx < 256; codebookIdx++) {
		float hue = codebookIdx * 0.54f / 255.0f;
		float saturation = 1.0f;
		float luminance = (codebookIdx < 128) ? (codebookIdx * 2.0 / 255.0f) : 1.0f;
		uint16 codebookValue = hslToRgb(hue, saturation, luminance);
		*codebookEntry++ = codebookValue;
		*codebookEntry++ = codebookValue;
		*codebookEntry++ = codebookValue;
		*codebookEntry++ = codebookValue;
	}

	// Initialize texture to 0's
	unsigned char* textureData = (unsigned char*)&(framebuffer -> texture);
	memset(textureData, FRAMEBUFFER_PIXELS, 1);
	int xIndex;
	int yIndex = FRAMEBUFFER_WIDTH;
	// Randomize the 2nd row from the bottom
	for (xIndex = 0; xIndex < FRAMEBUFFER_WIDTH; xIndex++) {
		textureData[xIndex] = rand() % 255;		
		textureData[yIndex + xIndex] = rand() % 255;
	}

	texture = pvr_mem_malloc(sizeof(VQ_Texture));

	updateTexture();
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
	my_vertex.v = 0.02f;
	my_vertex.argb = 0xFFFFFFFF;
	my_vertex.oargb = 0;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.y = 0.0f;
	my_vertex.v = 1.0f;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.x = 640.0f;
	my_vertex.y = 480.0f;
	my_vertex.u = 1.0f;
	my_vertex.v = 0.02f;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.flags = PVR_CMD_VERTEX_EOL;
	my_vertex.y = 0.0f;
	my_vertex.v = 1.0f;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	pvr_list_finish();

	pvr_scene_finish();	
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
