#ifndef _SAMPLE_FRAMEWORK_H
#define _SAMPLE_FRAMEWORK_H

#include <stdio.h>

#include <kos.h>

class Sample {
    public: 
        virtual ~Sample() {};
        virtual void init() { printf("ERROR: INITIALIZING BASE CLASS\n"); };
        virtual void update() {};
        virtual void render() {};
        virtual void cleanup() {};
        virtual void handleInput(cont_state_t* state) {};
};

#endif