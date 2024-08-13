#include "sample.h"

#include <malloc.h>

uint32 Sample::getFramebufferSize() {
    uint32 size = (textureMode == TEXTURE_MODE_VQ) ? CODEBOOK_SIZE : 0;
    size += (framebufferBitsPerPixel * framebufferWidth * framebufferHeight / 8);
    return size;
}

void Sample::setupTexture(TextureMode mode, uint16 width, uint16 height, uint8 bpp) {
    textureMode = mode;
    framebufferWidth = width;
    framebufferHeight = height;
    framebufferBitsPerPixel = bpp;
    uint32 framebufferSize = getFramebufferSize();
//    textureBuffer = (uint8*)malloc(framebufferSize);
    textureBuffer = (uint8*)memalign(64, framebufferSize);
    memset(textureBuffer, 0, framebufferSize);
    texture = pvr_mem_malloc(framebufferSize);
}

void Sample::allocateTextOverlayTexture() {
	textOverlayBuffer = (uint8*)malloc(TEXT_OVERLAY_SIZE_IN_BYTES);
	memset(textOverlayBuffer, 0, TEXT_OVERLAY_SIZE_IN_BYTES);

	// Allocate PVR VRAM for the text overlay texture
	textOverlayTexture = pvr_mem_malloc(TEXT_OVERLAY_SIZE_IN_BYTES);

	// Upload the text overlay texture to VRAM
	pvr_txr_load(textOverlayBuffer, textOverlayTexture, TEXT_OVERLAY_SIZE_IN_BYTES);    
}

void Sample::updateTextOverlayStrings(const char** overlayStrings, uint16 numStrings) {
	memset(textOverlayBuffer, 0, TEXT_OVERLAY_SIZE_IN_BYTES);

	// Draw the overlay text into this texture using the kos bios font
    uint32 leftOffset = LEFT_MARGIN;
    uint32 topOffset = TOP_MARGIN;
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
    for (uint32 i = 0; i < numStrings; i++) {
	    bfont_draw_str_ex(textOverlayBuffer + ((leftOffset + (topOffset * TEXT_OVERLAY_WIDTH)) * 2), TEXT_OVERLAY_WIDTH, 0x0000FFFF, 0x0000FFFF, 16, 0, overlayStrings[i]);
        topOffset += TOP_MARGIN;
    }

    pvr_txr_load(textOverlayBuffer, textOverlayTexture, TEXT_OVERLAY_SIZE_IN_BYTES);
}

void Sample::submitTexturedRectangle(uint32 list, uint32 textureFormat, uint16 textureWidth, uint16 textureHeight, pvr_ptr_t texture, uint32 filtering, 
                                     float xPosition, float yPosition, float zPosition, float width, float height, float u1, float u2, float v1, float v2) {
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	pvr_poly_cxt_col(&cxt, list);
	pvr_poly_cxt_txr(&cxt, list, textureFormat, textureWidth, textureHeight, texture, filtering);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	pvr_vertex_t my_vertex;

	my_vertex.flags = PVR_CMD_VERTEX;
	my_vertex.x = xPosition;
	my_vertex.y = yPosition + height;
	my_vertex.z = zPosition;
	my_vertex.u = u1;
	my_vertex.v = v2;
	my_vertex.argb = 0xFFFFFFFF;
	my_vertex.oargb = 0;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.y = yPosition;
	my_vertex.v = v1;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.x = xPosition + width;
	my_vertex.y = yPosition + height;
	my_vertex.u = u2;
	my_vertex.v = v2;
	pvr_prim(&my_vertex, sizeof(my_vertex));

	my_vertex.flags = PVR_CMD_VERTEX_EOL;
	my_vertex.y = yPosition;
	my_vertex.v = v1;
	pvr_prim(&my_vertex, sizeof(my_vertex));
}
