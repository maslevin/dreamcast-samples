/*-----------------------------------------------------------
VQ Framebuffer Example

vqfb - main.cpp
Sept. 25th, 2015

Author: Matt Slevinsky
------------------------------------------------------------*/

#include <kos.h>
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

/****************************************************************************/
/*                           VQ Texture Variables                           */
/****************************************************************************/
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

// Initialize the Simplex Noise Generator with these hardcoded parameters
SimplexNoise simplexNoise(1.0f, 0.85f, 2.0f, 0.5f);

/****************************************************************************/
/*                           Text Overlay Variables                         */
/****************************************************************************/
#define TEXT_OVERLAY_WIDTH 512
#define TEXT_OVERLAY_HEIGHT 512
#define TEXT_OVERLAY_SIZE_IN_BYTES (TEXT_OVERLAY_WIDTH * TEXT_OVERLAY_HEIGHT * 2)

#define TOP_MARGIN 32
#define LEFT_MARGIN 32

uint16* textOverlayBuffer;
pvr_ptr_t textOverlayTexture;

void initTextures() {
	// Initialize VQ Texture First - align on a 64 byte boundary so it's storage queue movable
	framebuffer = (VQ_Texture*)memalign(64, sizeof(VQ_Texture));

	// This creates a greyscale codebook (4 pixels of the same color, for each level of grey)
	// to use as a palette for this texture
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

	// Start with vertical color bands as a test
	unsigned char* textureData = (unsigned char*)&(framebuffer -> texture);
	int textureIdx;
	for (textureIdx = 0; textureIdx < FRAMEBUFFER_PIXELS; textureIdx++) {
		textureData[textureIdx] = textureIdx % FRAMEBUFFER_WIDTH;
	}

	// Allocate PVR VRAM for the VQ texture
	texture = pvr_mem_malloc(sizeof(VQ_Texture));

	// Move the VQ texture to VRAM
	pvr_txr_load(framebuffer, texture, sizeof(VQ_Texture));

	// Initialize the text overlay texture
	textOverlayBuffer = (uint16*)memalign(64, TEXT_OVERLAY_SIZE_IN_BYTES);
	memset(textOverlayBuffer, 0, TEXT_OVERLAY_SIZE_IN_BYTES);

	// Draw the overlay text into this texture using the kos bios font
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
	bfont_draw_str_ex(textOverlayBuffer + (LEFT_MARGIN + (TOP_MARGIN * TEXT_OVERLAY_WIDTH)), TEXT_OVERLAY_WIDTH, 0x0000FFFF, 0x0000FFFF, 16, 0, "Simplex Noise - 16-bit");

	// Allocate PVR VRAM for the text overlay texture
	textOverlayTexture = pvr_mem_malloc(TEXT_OVERLAY_SIZE_IN_BYTES);

	// Upload the text overlay texture to VRAM
	pvr_txr_load(textOverlayBuffer, textOverlayTexture, TEXT_OVERLAY_SIZE_IN_BYTES);
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

void freeTextures() {
	free (framebuffer);
	free (textOverlayBuffer);
	pvr_mem_free(texture);
	pvr_mem_free(textOverlayTexture);
}

void drawScreen() {
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	pvr_wait_ready();
	pvr_scene_begin();

	// Draw the VQ framebuffer into the Opaque Polygon list at z=1.0f
	pvr_list_begin(PVR_LIST_OP_POLY);

	pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
	// multiply width by 4x, as the codebook entries are 4 pixels of the same color
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

	pvr_list_begin(PVR_LIST_TR_POLY);

	// Draw the text overlay texture into the Transparent Polygon list at z=2.0f
	pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED, TEXT_OVERLAY_WIDTH, TEXT_OVERLAY_HEIGHT, textOverlayTexture, PVR_FILTER_NONE);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	//pvr_vertex_t my_vertex;
	my_vertex.flags = PVR_CMD_VERTEX;
	my_vertex.x = 0.0f;
	my_vertex.y = TEXT_OVERLAY_HEIGHT;
	my_vertex.z = 2.0f;
	my_vertex.u = 0.0f;
	my_vertex.v = 1.0f;
	my_vertex.argb = 0xFFFFFFFF;
	my_vertex.oargb = 0;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.y = 0.0f;
	my_vertex.v = 0.0f;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.x = TEXT_OVERLAY_WIDTH;
	my_vertex.y = TEXT_OVERLAY_HEIGHT;
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
#ifdef DEBUG
	gdb_init();
#endif

	maple_device_t* cont;
	cont_state_t* state;
	bool exitMainLoop = false;

	printf("Initializing PVR\n");
	pvr_setup();

	printf("Initializing Textures\n");
	initTextures();

    while(!exitMainLoop) {
    	int maxControllerIndex = maple_enum_count();
    	int controllerIndex = 0;
    	for (;controllerIndex < maxControllerIndex; controllerIndex++) {
	        if ((cont = maple_enum_type(controllerIndex, MAPLE_FUNC_CONTROLLER)) != NULL) {
	            state = (cont_state_t *)maple_dev_status(cont);

	            if (state == NULL) {
	            	exitMainLoop = true;
	            }

	            if (state->buttons & CONT_START)
	                exitMainLoop = true;
	        }
    	}

        drawScreen();

        updateTexture();
    }

    printf("Unloading Textures\n");
    freeTextures();

	return 0;
}
