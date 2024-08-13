#ifndef _FIRE_VQFB_H
#define _FIRE_VQFB_H

#include <kos.h>

#include "sample.h"

class FireVqfb : public Sample {
    public:
        void init();
        void update();
        void cleanup();
        void render();

    private:
        float hue2rgb(float p, float q, float t);
        uint16 hslToRgb(float hue, float saturation, float luminance);
};

#endif