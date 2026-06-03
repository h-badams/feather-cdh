####
# F Prime CMakeLists.txt:
#
# SOURCES: list of source files (to be compiled)
# AUTOCODER_INPUTS: list of files to be passed to the autocoders
# DEPENDS: list of libraries that this module depends on
#
# More information in the F´ CMake API documentation:
# https://fprime.jpl.nasa.gov/latest/docs/reference/api/cmake/API/
#
####

# Replace the framework's default TlmChan definition so instances.fpp can
# define CdhCore.tlmSend as TlmPacketizer without a duplicate-instance error.
register_fprime_config(
    FeatherCdhPartialDeployment_TlmOverride
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/CdhCoreTlmConfig.fpp"
    INTERFACE
)

register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/instances.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/topology.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/FeatherCdhPartialDeploymentTopology.cpp"
    DEPENDS
        Fw_Logger
        FeatherCdh_Types
        FeatherCdh_Components_SatStateMachine
        FeatherCdh_Components_EPSApplication
        FeatherCdh_Components_CommsApplication
        FeatherCdh_Components_SimplifiedMpptIcManager
        FeatherCdh_Components_HC12Manager
)
