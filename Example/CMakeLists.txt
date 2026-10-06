# Template version: 0.0.1
# Sets the version of CMakeLists.txt file.
# Never, under any circumstances, change this line. It is used by
# the module generation script to identify the template version.

#======================== Library configuration ===============================#
# Publicly included directories to provide headers include
set( Crsf_PublicIncludeDirs
    
)


# Privately included directories to provide headers include
set( Crsf_PrivateIncludeDirs
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# Source files list for current library generation
set( Crsf_SourceFiles
    Crsf_Example.cpp
    Crsf_Phy_Usart.cpp
)


# List of public header files provided by the current library
set( Crsf_PublicHeaders 
    Crsf_Example.h
    Crsf_Hal.h
)


# List of dependent libraries that are publicly used by the current library
set( Crsf_PublicDependentLibs
    CrsfCore_Lib
)


# List of dependent libraries that are privately used by the current library
set( Crsf_PrivateDependentLibs
    
)


#========================== Library generation ================================#
# Create library
add_library(Crsf_Lib STATIC ${Crsf_SourceFiles})


# Include necessary libraries to the target
target_link_libraries( Crsf_Lib
    PUBLIC 
        ${Crsf_PublicDependentLibs} 
    PRIVATE 
        ${Crsf_PrivateDependentLibs}
)


# Set public headers directory path in build folder
set(PUBLIC_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/PublicHeaders)

# Generates the public headers directory in build folder
file(MAKE_DIRECTORY ${PUBLIC_HEADERS_DIR})

# Process all public headers
foreach(PublicHeader IN LISTS Crsf_PublicHeaders)
    
    # Copy (or configure) only the public header there
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${PublicHeader} 
        ${PUBLIC_HEADERS_DIR}/${PublicHeader}
        COPYONLY
    )
    
endforeach()


# Set include directories for the created target
target_include_directories( Crsf_Lib
    PUBLIC
        $<BUILD_INTERFACE:${PUBLIC_HEADERS_DIR}>
        $<INSTALL_INTERFACE:include>
        ${Crsf_PublicIncludeDirs}
    PRIVATE
        ${Crsf_PrivateIncludeDirs}
)


# Print status message
message(STATUS "Crsf_Lib library created")

#========================= Documentation configuration ========================#
if(DOXYGEN_AVAILABLE STREQUAL "ON")
    Doxygen_AddPath("${CMAKE_CURRENT_SOURCE_DIR}/")
endif()
