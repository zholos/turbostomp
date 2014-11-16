# headers should be found automatically
add_library(
    core
    box.cc
    tile.cc
    grid.cc
    level.cc
    paint.cc
    control.cc
    camera.cc
    ode.cc
    mesh.cc
    sea.cc
    tools.cc
    effect.cc
    render.cc
    window.cc
    main.cc
    debug.cc
    ../shaders/shaders.cc
    ../meshes/meshes.cc
)

add_dependencies(core shaders meshes)
set_source_files_properties(
    ../shaders/shaders.cc
    ../meshes/meshes.cc
    PROPERTIES GENERATED TRUE)
