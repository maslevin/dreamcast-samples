#ifndef _SIMPLEX_VQFB_H
#define _SIMPLEX_VQFB_H

#include "sample.h"

#include "simplexnoise.h"

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

class SimplexVqfb : public Sample {
    public:
        SimplexVqfb() {
            simplexNoise = SimplexNoise(1.0f, 0.85f, 2.0f, 0.5f);
        }
        void init();
        void update();
        void cleanup();
        void render();

    private:
        VQ_Texture* framebuffer;
        pvr_ptr_t texture;
        SimplexNoise simplexNoise;
        uint16* textOverlayBuffer;
        pvr_ptr_t textOverlayTexture;
        float xVal = 0.0f;        
};

#endif