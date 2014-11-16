#ifndef CORE_EFFECT_H
#define CORE_EFFECT_H

#include "box.h"

#include <list>

using std::list;


class BoxEffect {
public: // TODO: private
    SBox b;
    int i;

public:
    BoxEffect(SBox);

    static void step_all();
    static list<BoxEffect*> all(); // TODO: better access mechanism
};


#endif
