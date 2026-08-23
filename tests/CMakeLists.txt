cmake_minimum_required(VERSION 3.16)
project(ESPressioWiFiTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

FetchContent_Declare(ESPressioObservable
    GIT_REPOSITORY https://github.com/ESPressio-Development-Platform/ESPressio-Observable.git
    GIT_TAG 3.0.2 GIT_SHALLOW TRUE)
FetchContent_GetProperties(ESPressioObservable)
if(NOT espressioobservable_POPULATED)
    FetchContent_Populate(ESPressioObservable)
endif()

FetchContent_Declare(ESPressioSerializable
    GIT_REPOSITORY https://github.com/ESPressio-Development-Platform/ESPressio-Serializable.git
    GIT_TAG 0.11.2 GIT_SHALLOW TRUE)
FetchContent_GetProperties(ESPressioSerializable)
if(NOT espressioserializable_POPULATED)
    FetchContent_Populate(ESPressioSerializable)
endif()

set(WIFI_TEST_INCLUDES
    ../src
    ${espressioobservable_SOURCE_DIR}/src
    ${espressioserializable_SOURCE_DIR}/src)

add_executable(espressio_wifi_tests test_wifi.cpp)
target_include_directories(espressio_wifi_tests PRIVATE ${WIFI_TEST_INCLUDES})

add_executable(espressio_wifi_configuration_tests test_configuration.cpp)
target_include_directories(espressio_wifi_configuration_tests PRIVATE ${WIFI_TEST_INCLUDES})

enable_testing()
add_test(NAME ESPressioWiFiTests COMMAND espressio_wifi_tests)
add_test(NAME ESPressioWiFiConfigurationTests COMMAND espressio_wifi_configuration_tests)
