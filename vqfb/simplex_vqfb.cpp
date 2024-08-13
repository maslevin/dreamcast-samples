#include "simplex_vqfb.h"

#include <math.h>

void SimplexVqfb::init() {
	setupTexture(TEXTURE_MODE_VQ, DEFAULT_FRAMEBUFFER_WIDTH, DEFAULT_FRAMEBUFFER_HEIGHT, 8);

	VQ_Texture* framebuffer = (VQ_Texture*)textureBuffer;
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
	uint32 framebufferSize = getFramebufferSize();
	for (uint32 textureIdx = 0; textureIdx < framebufferSize; textureIdx++) {
		textureData[textureIdx] = textureIdx % framebufferWidth;
	}

	// Move the VQ texture to VRAM
	pvr_txr_load(framebuffer, texture, framebufferSize);

	allocateTextOverlayTexture();
	const char* overlayStrings[] = {
		"Simplex Noise - 16-bit",
		"L / R - toggle modes"
	};
	updateTextOverlayStrings(overlayStrings, 2);

	xVal = 0.0f;
	yVal = 0.0f;
	xSplit = 0;
	xIncr = 1;
}

void SimplexVqfb::update() {
	uint8* pTexture = ((VQ_Texture*)textureBuffer) -> texture;

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
		pTexture += xSplit;
		for (int y = 0; y < framebufferHeight; y++) {
			float pVal = simplexNoise.fractal(5, xVal + ((float)(framebufferWidth - 1) / (float)framebufferWidth) + 0.5f, ((float)y / (float)framebufferWidth) + 0.5f);
			if ((pVal > 0.0f) && (pVal < 1.0f)) {
				*pTexture = (unsigned char)(pVal * 255.0f);
			} else if (pVal >= 1.0f) {
				*pTexture = 255;
			} else {
				*pTexture = 0;
			}

			pTexture+=framebufferWidth;
		}
		xSplit++;
		xSplit%=framebufferWidth;			
	}

	pvr_txr_load(textureBuffer, texture, getFramebufferSize());

	xVal += (1.0f / (float)framebufferWidth);
}

void SimplexVqfb::cleanup() {
    free (textureBuffer);
    free (textOverlayBuffer);
    pvr_mem_free(texture);
    pvr_mem_free(textOverlayTexture);    
}

void SimplexVqfb::render() {
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	pvr_wait_ready();
	pvr_scene_begin();

	// Draw the VQ framebuffer into the Opaque Polygon list at z=1.0f
	pvr_list_begin(PVR_LIST_OP_POLY);

	pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
	// multiply width by 4x, as the codebook entries are 4 pixels of the same color
	pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED, framebufferWidth * 4, framebufferHeight, texture, PVR_FILTER_BILINEAR);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	pvr_vertex_t my_vertex;

	if (xSplit == 0) {
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
	} else {
		float uLeft = (float)xSplit / (float)framebufferWidth;
		float uRight = ((float)xSplit + 1) / (float)framebufferWidth;
		float xRight = floor(640.0f - (640.0f * (float)xSplit / (float)framebufferWidth));

		my_vertex.flags = PVR_CMD_VERTEX;
		my_vertex.x = 0.0f;
		my_vertex.y = 480.0f;
		my_vertex.z = 1.0f;
		my_vertex.u = uRight;
		my_vertex.v = 1.0f;
		my_vertex.argb = 0xFFFFFFFF;
		my_vertex.oargb = 0;
		pvr_prim(&my_vertex, sizeof(my_vertex));

		my_vertex.y = 0.0f;
		my_vertex.v = 0.0f;
		pvr_prim(&my_vertex, sizeof(my_vertex));

		my_vertex.x = xRight;
		my_vertex.y = 480.0f;
		my_vertex.u = 1.0f;
		my_vertex.v = 1.0f;
		pvr_prim(&my_vertex, sizeof(my_vertex));

		my_vertex.flags = PVR_CMD_VERTEX_EOL;
		my_vertex.y = 0.0f;
		my_vertex.v = 0.0f;
		pvr_prim(&my_vertex, sizeof(my_vertex));

		my_vertex.flags = PVR_CMD_VERTEX;
		my_vertex.x = xRight;
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
		my_vertex.u = uLeft;
		my_vertex.v = 1.0f;
		pvr_prim(&my_vertex, sizeof(my_vertex));

		my_vertex.flags = PVR_CMD_VERTEX_EOL;
		my_vertex.y = 0.0f;
		my_vertex.v = 0.0f;
		pvr_prim(&my_vertex, sizeof(my_vertex));	
	}

	pvr_list_finish();

	pvr_list_begin(PVR_LIST_TR_POLY);

	// Draw the text overlay texture into the Transparent Polygon list at z=2.0f
	submitTexturedRectangle(PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED, TEXT_OVERLAY_WIDTH, TEXT_OVERLAY_HEIGHT, textOverlayTexture, PVR_FILTER_NONE, 
                            0.0f, 0.0f, 2.0f, 512.0f, 512.0f, 0.0f, 1.0f, 0.0f, 1.0f);	

	pvr_list_finish();

	pvr_scene_finish();	    
}

void SimplexVqfb::handleInput(cont_state_t* state) {
	if (state -> buttons & CONT_DPAD_UP) {
		xIncr++;
	}
	if (state -> buttons & CONT_DPAD_DOWN) {
		if (xIncr > 0) {
			xIncr--;
		}
	}
};