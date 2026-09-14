cmake_minimum_required(VERSION 3.16)
project(ESPressioWiFiContracts LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ESPRESSIO_DEPENDENCIES "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH "Dependency working-branch checkouts")
find_package(Threads REQUIRED)
enable_testing()

set(WIFI_TEST_INCLUDES ../src)
foreach(dependency System Observable Serializable Units Timing Task Threads Primitive Persistence Security Event Command)
    list(APPEND WIFI_TEST_INCLUDES "${ESPRESSIO_DEPENDENCIES}/ESPressio-${dependency}/src")
endforeach()
list(APPEND WIFI_TEST_INCLUDES
    "${ESPRESSIO_DEPENDENCIES}/ESPressio-Timing/tests/stubs"
    "${ESPRESSIO_DEPENDENCIES}/ESPressio-Task/tests")

add_executable(espressio_wifi_tests test_wifi.cpp)
target_include_directories(espressio_wifi_tests PRIVATE ${WIFI_TEST_INCLUDES})
target_compile_options(espressio_wifi_tests PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_link_libraries(espressio_wifi_tests PRIVATE Threads::Threads)

add_executable(espressio_wifi_configuration_tests test_configuration.cpp)
target_include_directories(espressio_wifi_configuration_tests PRIVATE ${WIFI_TEST_INCLUDES})
target_compile_options(espressio_wifi_configuration_tests PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_link_libraries(espressio_wifi_configuration_tests PRIVATE Threads::Threads)

add_executable(espressio_wifi_primitive_family_migration test_primitive_family_migration.cpp)
target_include_directories(espressio_wifi_primitive_family_migration PRIVATE ${WIFI_TEST_INCLUDES})
target_compile_options(espressio_wifi_primitive_family_migration PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_link_libraries(espressio_wifi_primitive_family_migration PRIVATE Threads::Threads)

add_test(NAME ESPressioWiFiTests COMMAND espressio_wifi_tests)
add_test(NAME ESPressioWiFiConfigurationTests COMMAND espressio_wifi_configuration_tests)
add_test(NAME ESPressioWiFiPrimitiveFamilyMigration COMMAND espressio_wifi_primitive_family_migration)
