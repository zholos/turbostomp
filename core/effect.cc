#include "effect.h"

#include <memory>

using std::unique_ptr;


list<unique_ptr<BoxEffect>> effects;


BoxEffect::BoxEffect(SBox b) :
    b(b),
    i(0)
{
    effects.emplace_back(this);
}


void
BoxEffect::step_all()
{
    for (auto it = effects.begin(); it != effects.end();) {
        if (++it->get()->i >= 20)
            it = effects.erase(it);
        else
            ++it;
    }
}


list<BoxEffect*>
BoxEffect::all()
{
    list<BoxEffect*> result;
    for (auto& e: effects)
        result.push_back(e.get());
    return result;
}
