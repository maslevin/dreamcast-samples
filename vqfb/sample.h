#ifndef _SAMPLE_FRAMEWORK_H
#define _SAMPLE_FRAMEWORK_H

#include <stdio.h>

class Sample {
    public: 
        virtual ~Sample() {};
        virtual void init() { printf("ERROR: INITIALIZING BASE CLASS\n"); };
        virtual void update() {};
        virtual void render() {};
        virtual void cleanup() {};
};

#endif