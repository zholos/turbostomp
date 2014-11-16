set(MESHES
    craft.obj
    ramp.obj
    corner1.obj
    corner2.obj
)

add_custom_command(
    OUTPUT meshes.cc
    COMMAND pgamecc-gen-files -v meshes
            -o ${CMAKE_CURRENT_BINARY_DIR}/meshes.cc ${MESHES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${MESHES}
)

add_custom_target(meshes DEPENDS meshes.cc)
