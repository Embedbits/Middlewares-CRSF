#===================== Integration tests configuration ========================#
# Integration tests of CRSF stack on target (Nucleo board) - RC side and flight
# controller on two cross connected USARTs. Registered in CRSF/CMakeLists.txt by
# IntegrationTesting_AddPath() and created by IntegrationTesting_Generate() in
# "IntegrationTest" build (see EmBi_Platform/CMake/IntegrationTesting/README.md).
#
# Required wiring is described in the header of ItTest_Crsf.cpp.
#
# The test set is independent - target interface (entry point, initialization,
# fault handlers, system reset) is implemented by ItTarget_CRSF.c and
# all libraries of the firmware are listed here.

# StartUp calls BspMain() - the entry point of the test firmware is provided by
# the test set (BspMain.h, ItTarget_CRSF.c)
if(NOT TARGET BspMain_Lib)
    add_library(BspMain_Lib INTERFACE)
    target_include_directories(BspMain_Lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

IntegrationTesting_Add_Test(
    NAME            Crsf
    TEST_SOURCE     ItTest_Crsf.cpp
    SOURCES         ItTarget_CRSF.c ItCrsf_UsartPhy.cpp
    LINK_LIBS       StartUp_Lib         # Startup code (calls BspMain)
                    CrsfCore_Lib        # module under test
                    Usart_Lib           # physical layer (USART MCAL)
                    Gpio_Lib            # RX pin pull-up
                    Tim_Lib             # 1 ms time base of CRSF tasks (TIM2), debug halt detection (TIM6)
                    Nvic_Lib            # reset of test case repeated after debug halt
    TIMEOUT         90
    CASE_TIMEOUT    15
)

# Test code is optimized for size - two complete stacks and the test do not fit
# 128 KB flash of STM32H503 at -O0. Module under test is built unchanged (-O0).
set_source_files_properties(ItTest_Crsf.cpp ItCrsf_UsartPhy.cpp
    PROPERTIES
        COMPILE_OPTIONS "-Os"
)
