#include "fire_vqfb.h"

#include <math.h>

void FireVqfb::init() {
    printf("1\n");
	setupTexture(TEXTURE_MODE_VQ, DEFAULT_FRAMEBUFFER_WIDTH, DEFAULT_FRAMEBUFFER_HEIGHT, 8);

    printf("2\n");
    VQ_Texture* framebuffer = (VQ_Texture*)textureBuffer;
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

    printf("3\n");
	// Initialize texture to 0's
	uint8* textureData = framebuffer -> texture;
	memset(textureData, 0, framebufferWidth * framebufferHeight);
	int xIndex;
	int yIndex = framebufferWidth;
	// Randomize the 2nd row from the bottom
	for (xIndex = 0; xIndex < framebufferWidth; xIndex++) {
		textureData[xIndex] = rand() % 255;		
		textureData[yIndex + xIndex] = rand() % 255;
	}

    printf("4\n");
	update();

    printf("5\n");
    allocateTextOverlayTexture();
	const char* overlayStrings[] = {
		"Fire - 16 bit",
		"L / R - toggle modes"
	};
    printf("6\n");    
    updateTextOverlayStrings(overlayStrings, 2);
}

void FireVqfb::update() {
    VQ_Texture* framebuffer = (VQ_Texture*)textureBuffer;
	unsigned char* pTexture = framebuffer -> texture;

	int yIndex;
	int xIndex;
	uint32 framebufferSize = (framebufferWidth * framebufferHeight) + CODEBOOK_SIZE;
	for (yIndex = framebufferHeight; yIndex > 1; yIndex--) {
		for (xIndex = 0; xIndex < framebufferWidth; xIndex++) {
			int framebufferIndex = yIndex * framebufferWidth + xIndex;
			float accumulator = pTexture[(yIndex - 1) * framebufferWidth + (xIndex - 1 + framebufferWidth) % framebufferWidth] +
				pTexture[(yIndex - 1) * framebufferWidth + (xIndex + framebufferWidth) % framebufferWidth] +
				pTexture[(yIndex - 1) * framebufferWidth + (xIndex + 1 + framebufferWidth) % framebufferWidth] +
				pTexture[(yIndex - 2) * framebufferWidth + (xIndex + framebufferWidth) % framebufferWidth];
			pTexture[framebufferIndex] = (unsigned char)floor(accumulator / 4.0125f);
		}
	}

	pvr_txr_load(framebuffer, texture, framebufferSize);

	yIndex = framebufferWidth;
	for (xIndex = 0; xIndex < framebufferWidth; xIndex++) {
		pTexture[yIndex + xIndex] = rand() % 255;
	}
}

void FireVqfb::cleanup() {
    free (textureBuffer);
    free (textOverlayBuffer);
    pvr_mem_free(texture);
    pvr_mem_free(textOverlayTexture); 
}

void FireVqfb::render() {
	pvr_wait_ready();
	pvr_scene_begin();

	// Draw the VQ framebuffer into the Opaque Polygon list at z=1.0f
	pvr_list_begin(PVR_LIST_OP_POLY);

	// Draw the paletted framebuffer into the Opaque Polygon list at z=1.0f
	submitTexturedRectangle(PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED, framebufferWidth * 4, framebufferHeight, texture, PVR_FILTER_BILINEAR, 
                            0.0f, 0.0f, 1.0f, 640.0f, 480.0f, 0.0f, 1.0f, 1.0f, 0.02f);

	pvr_list_finish();
/*
	pvr_list_begin(PVR_LIST_TR_POLY);

	// Draw the text overlay texture into the Transparent Polygon list at z=2.0f
	submitTexturedRectangle(PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED, TEXT_OVERLAY_WIDTH, TEXT_OVERLAY_HEIGHT, textOverlayTexture, PVR_FILTER_NONE, 
                            0.0f, 0.0f, 2.0f, 512.0f, 512.0f, 0.0f, 1.0f, 0.0f, 1.0f);

	pvr_list_finish();
*/

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