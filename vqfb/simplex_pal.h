#ifndef _SIMPLEX_PAL_H
#define _SIMPLEX_PAL_H

#include "sample.h"

#include "simplexnoise.h"

#include "vqfb.h"

class SimplexPal : public Sample {
    public:
        SimplexPal() {
            simplexNoise = SimplexNoise(1.0f, 0.85f, 2.0f, 0.5f);
        }
        void init();
        void update();
        void cleanup();
        void render();

    private:
        uint8* palettedTextureBuffer;
        pvr_ptr_t texture;

        SimplexNoise simplexNoise;

        uint16* textOverlayBuffer;
        pvr_ptr_t textOverlayTexture;
        float xVal;        
};

#endif