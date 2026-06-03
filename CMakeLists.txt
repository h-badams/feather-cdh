####
# This sets up the build system for the 'FeatherCdh' project, including
# components and deployments from project.cmake. In addition, it imports the core F Prime components.
####

cmake_minimum_required(VERSION 3.24.2)
project(FeatherCdh C CXX)

###
# F' Core Setup
# This includes all of the F prime core components, and imports the make-system.
###
include("${CMAKE_CURRENT_LIST_DIR}/lib/fprime/cmake/FPrime.cmake")
# NOTE: register custom targets between these two lines
fprime_setup_included_code()

# Override ActiveRateGroupOutputPorts (default 10 → 12) to accommodate the
# 12-slot 1 Hz rate group in both deployments.
register_fprime_config(
    FeatherCdh_AcConstants
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/config/AcConstants.fpp"
    INTERFACE
)

# Override FW_FILE_HANDLE_MAX_SIZE and FW_DIRECTORY_HANDLE_MAX_SIZE to 64
# (Arduino SD::File is ~40 bytes; fprime default is 16).
# Also replaces fprime's default/config/PlatformCfg.fpp so those constants
# are not defined twice when the fprime-arduino ARM platform is active.
register_fprime_config(
    FeatherCdh_PlatformCfg
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/config/PlatformCfg.fpp"
    INTERFACE
)

# This includes project-wide objects
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FeatherCdh")
