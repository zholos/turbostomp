include_directories(../core)
link_libraries(core)

add_executable(
    box-test
    box-test.cc
)

add_executable(
    grid-test
    grid-test.cc
)
