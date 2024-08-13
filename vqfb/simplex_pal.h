#ifndef _SIMPLEX_PAL_H
#define _SIMPLEX_PAL_H

#include "sample.h"

#include "simplexnoise.h"

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
        SimplexNoise simplexNoise;
        float xVal;        
};

#endif