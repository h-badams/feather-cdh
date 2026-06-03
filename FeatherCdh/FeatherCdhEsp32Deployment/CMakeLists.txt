#####
# 'FeatherCdhEsp32Deployment' Deployment:
#
# This registers the 'FeatherCdhEsp32Deployment' deployment to the build system. 
# Custom components that have not been added at the project-level should be added to 
# the list below.
#
#####

# Deployment is only compatible with the Arduino platform
restrict_platforms(ArduinoFw)

###
# Topology and Components
###

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top/")

# Add custom components to this specific deployment here
# add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/MyComponent/")

register_fprime_deployment(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
    DEPENDS
        ${FPRIME_CURRENT_MODULE}_Top
)

# Attach Arduino libraries, the Arduino core, and post-build steps (also generates
# the arduino-cli-sketch directory containing build_opt.h required by the compile flags)
finalize_arduino_executable("${FPRIME_CURRENT_MODULE}")
