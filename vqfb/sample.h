#ifndef _SAMPLE_FRAMEWORK_H
#define _SAMPLE_FRAMEWORK_H

#include <stdio.h>

#include <kos.h>

#define DEFAULT_FRAMEBUFFER_WIDTH 256
#define DEFAULT_FRAMEBUFFER_HEIGHT 256

/****************************************************************************/
/*                               VQ Texture                                 */
/****************************************************************************/
//The codebook for a VQ texture is always 2048 bytes
#define CODEBOOK_SIZE 2048

typedef struct {
	uint8 codebook[CODEBOOK_SIZE];
	uint8 texture[];
} VQ_Texture;

/****************************************************************************/
/*                              Text Overlay                                */
/****************************************************************************/
#define TEXT_OVERLAY_WIDTH 512
#define TEXT_OVERLAY_HEIGHT 512
#define TEXT_OVERLAY_SIZE_IN_BYTES (TEXT_OVERLAY_WIDTH * TEXT_OVERLAY_HEIGHT * 2)

#define TOP_MARGIN 32
#define LEFT_MARGIN 32

/****************************************************************************/
/*                             Utility Macros                               */
/****************************************************************************/
#define PACK_ARGB8888(a,r,g,b) ( ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF) )

typedef enum {
    TEXTURE_MODE_RAW,
    TEXTURE_MODE_PALETTED,
    TEXTURE_MODE_VQ
} TextureMode;

class Sample {
    public: 
        virtual ~Sample() {};
        virtual void init() { printf("ERROR: INITIALIZING BASE CLASS\n"); };
        virtual void update() {};
        virtual void render() {};
        virtual void cleanup() {};
        virtual void handleInput(cont_state_t* state) {};

    protected:
        uint32 getFramebufferSize();
        
        void setupTexture(TextureMode mode, uint16 width, uint16 height, uint8 bpp);

        void allocateTextOverlayTexture();

        void updateTextOverlayStrings(const char** overlayString, uint16 numStrings);

        void submitTexturedRectangle(uint32 list, uint32 textureFormat, uint16 textureWidth, uint16 textureHeight, pvr_ptr_t texture, uint32 filtering, 
                                     float xPosition, float yPosition, float zPosition, float width, float height, float u1, float u2, float v1, float v2);

        TextureMode textureMode;
        uint16 framebufferWidth;
        uint16 framebufferHeight;
        uint8  framebufferBitsPerPixel;
        bool   isFramebufferVQ;

        uint8* textureBuffer;
        pvr_ptr_t texture;
        uint8* textOverlayBuffer;
        pvr_ptr_t textOverlayTexture;
};

#endif