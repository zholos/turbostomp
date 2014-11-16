set(SHADERS
    preamble.glsl
    lib.glsl
    common.glsl
    cube.vert cube.frag
    tile.vert tile.frag
    mesh.vert mesh.frag
    thruster.vert thruster.frag
    ball.vert ball.frag
    bolt.vert bolt.frag
    cube_effect.vert cube_effect.frag
    post.vert post.frag
)

add_custom_command(
    OUTPUT shaders.cc
    COMMAND pgamecc-gen-files -v shaders
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders.cc ${SHADERS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${SHADERS}
)

add_custom_target(shaders DEPENDS shaders.cc)
