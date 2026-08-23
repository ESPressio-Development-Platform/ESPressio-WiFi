cmake_minimum_required(VERSION 3.16)

if(ESP_PLATFORM)
    idf_component_register(INCLUDE_DIRS "src")
else()
    project(ESPressioWiFi LANGUAGES CXX)
    enable_testing()
    add_subdirectory(tests)
endif()
