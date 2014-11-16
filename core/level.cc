#include "level.h"

#include <map>
#include <tuple>

using std::map;
using std::get;
using std::tuple;


void
Level::step()
{
    before_step();
    sea.step(grid);
    after_step();
}



typedef map<tuple<int, string>, function<Level*()>> Entries;

static Entries&
entries() {
    // safe static initialization order
    static Entries entries;
    return entries;
}

Level::catalogue::catalogue(int priority, string name,
                            function<Level*()> factory)
{
    auto r = entries().emplace(Entries::key_type{ priority, name }, factory);
    assert(r.second); // TODO: figure out what to do with duplicate names
}

bool
Level::catalogue::empty()
{
    return entries().empty();
}

list<Level::catalogue::Entry>
Level::catalogue::all()
{
    list<Level::catalogue::Entry> l;
    for (auto& e: entries())
        l.push_back(Entry{ get<1>(e.first), e.second });
    return l;
}

Level*
Level::catalogue::first()
{
    return entries().begin()->second();
}
