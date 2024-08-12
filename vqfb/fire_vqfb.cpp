#include "fire_vqfb.h"

#include <math.h>

/****************************************************************************/
/*                           Text Overlay Variables                         */
/****************************************************************************/
#define TEXT_OVERLAY_WIDTH 512
#define TEXT_OVERLAY_HEIGHT 512
#define TEXT_OVERLAY_SIZE_IN_BYTES (TEXT_OVERLAY_WIDTH * TEXT_OVERLAY_HEIGHT * 2)

#define TOP_MARGIN 32
#define LEFT_MARGIN 32

void FireVqfb::init() {
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
	memset(textureData, 0, FRAMEBUFFER_PIXELS);
	int xIndex;
	int yIndex = FRAMEBUFFER_WIDTH;
	// Randomize the 2nd row from the bottom
	for (xIndex = 0; xIndex < FRAMEBUFFER_WIDTH; xIndex++) {
		textureData[xIndex] = rand() % 255;		
		textureData[yIndex + xIndex] = rand() % 255;
	}

	texture = pvr_mem_malloc(sizeof(VQ_Texture));

	update();

	// Initialize the text overlay texture
	textOverlayBuffer = (uint16*)memalign(64, TEXT_OVERLAY_SIZE_IN_BYTES);
	memset(textOverlayBuffer, 0, TEXT_OVERLAY_SIZE_IN_BYTES);

	// Draw the overlay text into this texture using the kos bios font
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
	bfont_draw_str_ex(textOverlayBuffer + (LEFT_MARGIN + (TOP_MARGIN * TEXT_OVERLAY_WIDTH)), TEXT_OVERLAY_WIDTH, 0x0000FFFF, 0x0000FFFF, 16, 0, "Fire - 16 bit");
	bfont_draw_str_ex(textOverlayBuffer + (LEFT_MARGIN + ((TOP_MARGIN + 24) * TEXT_OVERLAY_WIDTH)), TEXT_OVERLAY_WIDTH, 0x0000FFFF, 0x0000FFFF, 16, 0, "L / R - toggle modes");

	// Allocate PVR VRAM for the text overlay texture
	textOverlayTexture = pvr_mem_malloc(TEXT_OVERLAY_SIZE_IN_BYTES);

	// Upload the text overlay texture to VRAM
	pvr_txr_load(textOverlayBuffer, textOverlayTexture, TEXT_OVERLAY_SIZE_IN_BYTES);
}

void FireVqfb::update() {
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

void FireVqfb::cleanup() {
    free (framebuffer);
    free (textOverlayBuffer);
    pvr_mem_free(texture);
    pvr_mem_free(textOverlayTexture); 
}

void FireVqfb::render() {
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	pvr_wait_ready();
	pvr_scene_begin();

	// Draw the VQ framebuffer into the Opaque Polygon list at z=1.0f
	pvr_list_begin(PVR_LIST_OP_POLY);

	pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
	// multiply width by 4x, as the codebook entries are 4 pixels of the same color
	pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED, FRAMEBUFFER_WIDTH * 4, FRAMEBUFFER_HEIGHT, texture, PVR_FILTER_BILINEAR);
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

float FireVqfb::hue2rgb(float p, float q, float t) {
	if(t < 0.0f) t += 1.0f;
	if(t > 1.0f) t -= 1.0f;
	if(t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
	if(t < 1.0f/2.0f) return q;
	if(t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
	return p;
}

uint16 FireVqfb::hslToRgb(float hue, float saturation, float luminance) {
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