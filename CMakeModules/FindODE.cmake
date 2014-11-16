find_path(ODE_INCLUDE_DIRS ode/ode.h)
find_library(ODE_LIBRARIES ode)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ODE DEFAULT_MSG
    ODE_LIBRARIES ODE_INCLUDE_DIRS)
