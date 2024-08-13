#ifndef _SIMPLEX_VQFB_H
#define _SIMPLEX_VQFB_H

#include "sample.h"

#include "simplexnoise.h"

#include "vqfb.h"

class SimplexVqfb : public Sample {
    public:
        SimplexVqfb() {
            simplexNoise = SimplexNoise(1.0f, 0.85f, 2.0f, 0.5f);
        }
        void init();
        void update();
        void cleanup();
        void render();
        void handleInput(cont_state_t* state);

    private:
        VQ_Texture* framebuffer;
        pvr_ptr_t texture;
        SimplexNoise simplexNoise;
        uint16* textOverlayBuffer;
        pvr_ptr_t textOverlayTexture;
        float xVal = 0.0f;
        float yVal = 0.0f;
        uint16 xSplit = 0;
        uint16 xIncr = 1;
};

#endif