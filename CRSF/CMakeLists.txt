# Template version: 1.0.0
# Sets the version of CMakeLists.txt file.
# Never, under any circumstances, change this line. It is used by
# the module generation script to identify the template version.

#======================== Library configuration ===============================#
# Publicly included directories to provide headers include
set( CrsfCore_PublicIncludeDirs
    ${CMAKE_CURRENT_SOURCE_DIR}/AppLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/TransportLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/PhysicalLayer
)


# Privately included directories to provide headers include
set( CrsfCore_PrivateIncludeDirs
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# Source files list for current library generation
set( CrsfCore_SourceFiles
    TransportLayer/Crsf_Crc.cpp
    TransportLayer/Crsf_TL.cpp
    AppLayer/Crsf_App.cpp
    AppLayer/Crsf_App_Codec.cpp
    AppLayer/Crsf_App_RcChannels.cpp
    AppLayer/Crsf_App_Telemetry.cpp
    AppLayer/Crsf_App_Params.cpp
    AppLayer/Crsf_App_Commands.cpp
    AppLayer/Crsf_App_Bridge.cpp
)


# List of public header files provided by the current library
set( CrsfCore_PublicHeaders 
    AppLayer/Crsf_Types.h
    AppLayer/Crsf_App_Types.hpp
    AppLayer/Crsf_App.hpp
    AppLayer/Crsf_App_Codec.hpp
    AppLayer/Crsf_App_RcChannels.hpp
    AppLayer/Crsf_App_Telemetry.hpp
    AppLayer/Crsf_App_Params.hpp
    AppLayer/Crsf_App_Commands.hpp
    AppLayer/Crsf_App_Bridge.hpp
    PhysicalLayer/Crsf_Phy.hpp
    TransportLayer/Crsf_Crc.hpp
    TransportLayer/Crsf_TL.hpp
)


# List of dependent libraries that are publicly used by the current library
set( CrsfCore_PublicDependentLibs
    
)


# List of dependent libraries that are privately used by the current library
set( CrsfCore_PrivateDependentLibs
    
)


#========================== Library generation ================================#
# Create library
add_library(CrsfCore_Lib STATIC ${CrsfCore_SourceFiles})


# Include necessary libraries to the target
target_link_libraries( CrsfCore_Lib
    PUBLIC 
        ${CrsfCore_PublicDependentLibs} 
    PRIVATE 
        ${CrsfCore_PrivateDependentLibs}
)


# Set public headers directory path in build folder
set(PUBLIC_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/PublicHeaders)

# Generates the public headers directory in build folder
file(MAKE_DIRECTORY ${PUBLIC_HEADERS_DIR})

# Process all public headers
foreach(PublicHeader IN LISTS CrsfCore_PublicHeaders)
    
    # Copy (or configure) only the public header there
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${PublicHeader} 
        ${PUBLIC_HEADERS_DIR}/${PublicHeader}
        COPYONLY
    )
    
endforeach()


# Set include directories for the created target
target_include_directories( CrsfCore_Lib
    PUBLIC
        $<BUILD_INTERFACE:${PUBLIC_HEADERS_DIR}>
        $<INSTALL_INTERFACE:include>
        ${CrsfCore_PublicIncludeDirs}
    PRIVATE
        ${CrsfCore_PrivateIncludeDirs}
)


# Print status message
message(STATUS "CrsfCore_Lib library created")

#========================= Documentation configuration ========================#
if(DOXYGEN_AVAILABLE STREQUAL "ON")
    Doxygen_AddPath("${CMAKE_CURRENT_SOURCE_DIR}/")
endif()

#========================= Unit tests configuration ===========================#
if(UNIT_TESTING_AVAILABLE STREQUAL "ON")
    UnitTesting_AddPath("${CMAKE_CURRENT_SOURCE_DIR}/Tests/UnitTests")
endif()

#===================== Integration tests configuration ========================#
if(INTEGRATION_TESTING_AVAILABLE STREQUAL "ON")
    IntegrationTesting_AddPath("${CMAKE_CURRENT_SOURCE_DIR}/Tests/IntegrationTests")
endif()
