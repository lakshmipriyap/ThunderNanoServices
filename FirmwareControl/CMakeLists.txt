set(PLUGIN_NAME FirmwareControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(MFRFWLibs REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    FirmwareControl.cpp
    FirmwareControlJsonRpc.cpp
    Module.cpp
)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        mfrfwlibs::mfrfwlibs
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)

target_include_directories(${MODULE_NAME} 
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
