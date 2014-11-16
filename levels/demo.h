#ifndef LEVELS_DEMO_H
#define LEVELS_DEMO_H

#include "level.h"

class Craft;


class DemoLevel : public Level {
    Craft* craft = {};

public:
    void generate();
    void before_step();
    void after_step();
};


#endif
