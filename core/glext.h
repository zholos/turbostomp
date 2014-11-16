#ifndef CORE_GLEXT_H
#define CORE_GLEXT_H

#include <pgamecc.h>

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using std::function;
using std::tuple;
using std::vector;
using std::get;
using std::index_sequence;
using std::make_index_sequence;
using std::conditional_t;
using std::forward;

namespace gl = pgamecc::gl;


// OpenGL buffer management for instanced rendering.

template<typename... Args>
class VertexStream {
    typedef make_index_sequence<sizeof...(Args)> Indexes;

    typedef function<void()> Render;
    Render render;

    tuple<gl::StreamArray<Args>...> buffers;
    tuple<vector<Args>...> data;
    const size_t block;

    template<size_t... I>
    void push_(index_sequence<I...>, Args&&... args) {
        [](...){}((get<I>(data).push_back(forward<Args>(args)), 0)...);
    }

    template<size_t... I>
    void load_(index_sequence<I...>) {
        [](...){}((get<I>(buffers).load(get<I>(data)), 0)...);
    }

    template<size_t... I>
    void clear_(index_sequence<I...>) {
        [](...){}((get<I>(data).clear(), 0)...);
    }

    template<size_t... I>
    void attribs_(index_sequence<I...>,
                  gl::Program& program,
                  conditional_t<0, Args, int>... attribs) {
        [](...){}((program.attrib(attribs).array(get<I>(buffers)), 0)...);
    }

    template<size_t... I>
    void attribs_instanced_(index_sequence<I...>,
                            gl::Program& program,
                            conditional_t<0, Args, int>... attribs) {
        [](...){}((program.attrib(attribs).instanced().array(get<I>(buffers)),
                   0)...);
    }

public:
    VertexStream(size_t block = 1024) : block(block) {}

    void set_render(Render&& r) { render = r; }

    size_t size() const { return get<0>(data).size(); }

    void push(Args&&... args) {
        push_(Indexes(), forward<Args>(args)...);
        if (size() == block)
            flush();
    }

    void attribs(gl::Program& program,
                 conditional_t<0, Args, int>... attribs) {
        attribs_(Indexes(), program, attribs...);
    }

    void attribs_instanced(gl::Program& program,
                           conditional_t<0, Args, int>... attribs) {
        attribs_instanced_(Indexes(), program, attribs...);
    }

    void flush() {
        if (size()) {
            load_(Indexes());
            render();
            clear_(Indexes());
        }
    }
};


template<int... Arrays>
class WithProgram {
    gl::Program& program;

public:
    WithProgram(gl::Program& program) : program(program) { program.use(); }
    ~WithProgram() {
        [](...){}((program.attrib(Arrays).unarray(), 0)...);
        program.unuse();
    }
};


#endif
