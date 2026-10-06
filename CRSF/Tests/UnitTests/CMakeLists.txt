#======================== Unit tests configuration ============================#
# Unit tests of CRSF middleware. Registered in module CMakeLists.txt by
# UnitTesting_AddPath() and created by UnitTesting_Generate() in "UnitTest"
# build (see EmBi_Platform/CMake/UnitTesting/README.md).
#
# Test files are C++ (module is C++), Unity test runner is compiled as C++
# too (see UnitTesting_Add_Test).

get_filename_component(CRSF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

set(CRSF_UT_INCLUDE_DIRS
    ${CRSF_DIR}/AppLayer
    ${CRSF_DIR}/TransportLayer
    ${CRSF_DIR}/PhysicalLayer
)

set(CRSF_UT_TL_SOURCES
    ${CRSF_DIR}/TransportLayer/Crsf_Crc.cpp
    ${CRSF_DIR}/TransportLayer/Crsf_TL.cpp
)

set(CRSF_UT_APP_SOURCES
    ${CRSF_UT_TL_SOURCES}
    ${CRSF_DIR}/AppLayer/Crsf_App.cpp
    ${CRSF_DIR}/AppLayer/Crsf_App_Codec.cpp
)

set(CRSF_UT_HELPER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/UtCrsf_FakePhy.cpp
)

UnitTesting_Add_Test(
    NAME            Crsf_Crc
    TEST_SOURCE     Test_Crsf_Crc.cpp
    UUT_SOURCES     ${CRSF_DIR}/TransportLayer/Crsf_Crc.cpp
    SOURCES         ${CRSF_UT_HELPER_SOURCES}
    INCLUDE_DIRS    ${CRSF_UT_INCLUDE_DIRS}
)

UnitTesting_Add_Test(
    NAME            Crsf_TL
    TEST_SOURCE     Test_Crsf_TL.cpp
    UUT_SOURCES     ${CRSF_UT_TL_SOURCES}
    SOURCES         ${CRSF_UT_HELPER_SOURCES}
    INCLUDE_DIRS    ${CRSF_UT_INCLUDE_DIRS}
)

UnitTesting_Add_Test(
    NAME            Crsf_Codec
    TEST_SOURCE     Test_Crsf_Codec.cpp
    UUT_SOURCES     ${CRSF_DIR}/AppLayer/Crsf_App_Codec.cpp
    INCLUDE_DIRS    ${CRSF_UT_INCLUDE_DIRS}
)

UnitTesting_Add_Test(
    NAME            Crsf_AppCore
    TEST_SOURCE     Test_Crsf_AppCore.cpp
    UUT_SOURCES     ${CRSF_UT_APP_SOURCES}
                    ${CRSF_DIR}/AppLayer/Crsf_App_RcChannels.cpp
                    ${CRSF_DIR}/AppLayer/Crsf_App_Telemetry.cpp
    SOURCES         ${CRSF_UT_HELPER_SOURCES}
    INCLUDE_DIRS    ${CRSF_UT_INCLUDE_DIRS}
)

UnitTesting_Add_Test(
    NAME            Crsf_Params
    TEST_SOURCE     Test_Crsf_Params.cpp
    UUT_SOURCES     ${CRSF_UT_APP_SOURCES}
                    ${CRSF_DIR}/AppLayer/Crsf_App_Params.cpp
    SOURCES         ${CRSF_UT_HELPER_SOURCES}
    INCLUDE_DIRS    ${CRSF_UT_INCLUDE_DIRS}
)

UnitTesting_Add_Test(
    NAME            Crsf_Commands
    TEST_SOURCE     Test_Crsf_Commands.cpp
    UUT_SOURCES     ${CRSF_UT_APP_SOURCES}
                    ${CRSF_DIR}/AppLayer/Crsf_App_Commands.cpp
    SOURCES         ${CRSF_UT_HELPER_SOURCES}
    INCLUDE_DIRS    ${CRSF_UT_INCLUDE_DIRS}
)

UnitTesting_Add_Test(
    NAME            Crsf_Bridge
    TEST_SOURCE     Test_Crsf_Bridge.cpp
    UUT_SOURCES     ${CRSF_UT_APP_SOURCES}
                    ${CRSF_DIR}/AppLayer/Crsf_App_Bridge.cpp
    SOURCES         ${CRSF_UT_HELPER_SOURCES}
    INCLUDE_DIRS    ${CRSF_UT_INCLUDE_DIRS}
)
