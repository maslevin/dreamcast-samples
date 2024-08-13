#include "simplex_pal.h"

#include "simplexnoise.h"

/****************************************************************************/
/*                           Text Overlay Variables                         */
/****************************************************************************/
#define TEXT_OVERLAY_WIDTH 512
#define TEXT_OVERLAY_HEIGHT 512
#define TEXT_OVERLAY_SIZE_IN_BYTES (TEXT_OVERLAY_WIDTH * TEXT_OVERLAY_HEIGHT * 2)

#define TOP_MARGIN 32
#define LEFT_MARGIN 32

#define PACK_ARGB8888(a,r,g,b) ( ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF) )

void SimplexPal::init() {
	// Initialize Global Palette
	pvr_set_pal_format(PVR_PAL_ARGB8888);
	for (int i = 0; i <= 255; i++) {
		pvr_set_pal_entry(i, PACK_ARGB8888(255, i,  i, i));
	}

	// Initialize Texture First - align on a 64 byte boundary so it's storage queue movable
	palettedTextureBuffer = (uint8*)memalign(64, FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT);

	// Start with vertical color bands as a test
	uint8* textureData = palettedTextureBuffer;
	int textureIdx;
	for (textureIdx = 0; textureIdx < FRAMEBUFFER_PIXELS; textureIdx++) {
		textureData[textureIdx] = textureIdx % FRAMEBUFFER_WIDTH;
	}

	// Allocate PVR VRAM for the VQ texture
	texture = pvr_mem_malloc(FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT);

	// Move the paletted texture to VRAM
	pvr_txr_load_ex(palettedTextureBuffer, texture, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, PVR_TXRLOAD_8BPP);

	// Initialize the text overlay texture
	textOverlayBuffer = (uint16*)memalign(64, TEXT_OVERLAY_SIZE_IN_BYTES);
	memset(textOverlayBuffer, 0, TEXT_OVERLAY_SIZE_IN_BYTES);

	// Draw the overlay text into this texture using the kos bios font
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
	bfont_draw_str_ex(textOverlayBuffer + (LEFT_MARGIN + (TOP_MARGIN * TEXT_OVERLAY_WIDTH)), TEXT_OVERLAY_WIDTH, 0x0000FFFF, 0x0000FFFF, 16, 0, "Simplex Noise - 32-bit");
	bfont_draw_str_ex(textOverlayBuffer + (LEFT_MARGIN + ((TOP_MARGIN + 24) * TEXT_OVERLAY_WIDTH)), TEXT_OVERLAY_WIDTH, 0x0000FFFF, 0x0000FFFF, 16, 0, "L / R - toggle modes");

	// Allocate PVR VRAM for the text overlay texture
	textOverlayTexture = pvr_mem_malloc(TEXT_OVERLAY_SIZE_IN_BYTES);

	// Upload the text overlay texture to VRAM
	pvr_txr_load(textOverlayBuffer, textOverlayTexture, TEXT_OVERLAY_SIZE_IN_BYTES);

	xVal = 0.0f;
}

void SimplexPal::update() {
	uint8* pTexture = palettedTextureBuffer;

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

	pvr_txr_load_ex(palettedTextureBuffer, texture, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, PVR_TXRLOAD_8BPP);

	xVal += (1.0f / (float)FRAMEBUFFER_WIDTH);
}

void SimplexPal::render() {
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	pvr_wait_ready();
	pvr_scene_begin();

	// Draw the VQ framebuffer into the Opaque Polygon list at z=1.0f
	pvr_list_begin(PVR_LIST_OP_POLY);

	pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
	pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_PAL8BPP, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, texture, PVR_FILTER_BILINEAR);
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

void SimplexPal::cleanup() {
    free (palettedTextureBuffer);
    free (textOverlayBuffer);
    pvr_mem_free(texture);
    pvr_mem_free(textOverlayTexture);    
}