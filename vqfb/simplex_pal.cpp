#include "simplex_pal.h"

#include "simplexnoise.h"

void SimplexPal::init() {
	// Initialize Global Palette
	pvr_set_pal_format(PVR_PAL_ARGB8888);
	for (int i = 0; i <= 255; i++) {
		pvr_set_pal_entry(i, PACK_ARGB8888(255, i,  i, i));
	}

	setupTexture(TEXTURE_MODE_PALETTED, DEFAULT_FRAMEBUFFER_WIDTH, DEFAULT_FRAMEBUFFER_HEIGHT, 8);

	// Start with vertical color bands as a test
	uint8* textureData = textureBuffer;
	uint32 framebufferSize = getFramebufferSize();
	for (uint32 textureIdx = 0; textureIdx < framebufferSize; textureIdx++) {
		textureData[textureIdx] = textureIdx % framebufferWidth;
	}

	// Move the paletted texture to VRAM
	pvr_txr_load_ex(textureBuffer, texture, framebufferWidth, framebufferHeight, PVR_TXRLOAD_8BPP);

	allocateTextOverlayTexture();
	const char* overlayStrings[] = {
		"Simplex Noise - 32-bit",
		"L / R - toggle modes",
	};
	updateTextOverlayStrings(overlayStrings, 2);

	xVal = 0.0f;
}

void SimplexPal::update() {
	uint8* pTexture = textureBuffer;

	if (xVal == 0.0f) {
		for (int y = 0; y < framebufferHeight; y++) {
			for (int x = 0; x < framebufferWidth; x++) {
				float pVal = simplexNoise.fractal(5, xVal + ((float)x / (float)framebufferWidth) + 0.5f, ((float)y / (float)framebufferWidth) + 0.5f);
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
		for (int y = 0; y < framebufferHeight; y++) {
			for (int x = 0; x < (framebufferWidth - 1); x++) {
				*pTexture = pTexture[1];
				pTexture++;
			}

			float pVal = simplexNoise.fractal(5, xVal + ((float)(framebufferWidth - 1) / (float)framebufferWidth) + 0.5f, ((float)y / (float)framebufferWidth) + 0.5f);
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

	pvr_txr_load_ex(textureBuffer, texture, framebufferWidth, framebufferHeight, PVR_TXRLOAD_8BPP);

	xVal += (1.0f / (float)framebufferWidth);
}

void SimplexPal::render() {
	pvr_wait_ready();
	pvr_scene_begin();

	pvr_list_begin(PVR_LIST_OP_POLY);
	// Draw the paletted framebuffer into the Opaque Polygon list at z=1.0f
	submitTexturedRectangle(PVR_LIST_OP_POLY, PVR_TXRFMT_PAL8BPP, framebufferWidth, framebufferHeight, texture, PVR_FILTER_BILINEAR, 
                            0.0f, 0.0f, 1.0f, 640.0f, 480.0f, 0.0f, 1.0f, 0.0f, 1.0f);

	pvr_list_finish();

	pvr_list_begin(PVR_LIST_TR_POLY);
	// Draw the text overlay texture into the Transparent Polygon list at z=2.0f
	submitTexturedRectangle(PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED, TEXT_OVERLAY_WIDTH, TEXT_OVERLAY_HEIGHT, textOverlayTexture, PVR_FILTER_NONE, 
                            0.0f, 0.0f, 2.0f, 512.0f, 512.0f, 0.0f, 1.0f, 0.0f, 1.0f);

	pvr_list_finish();

	pvr_scene_finish();	    
}

void SimplexPal::cleanup() {
    free (textureBuffer);
    free (textOverlayBuffer);
    pvr_mem_free(texture);
    pvr_mem_free(textOverlayTexture);    
}