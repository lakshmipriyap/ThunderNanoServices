set(PLUGIN_NAME JoinTest)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})


# ------------------- Example Library ------------------------
add_library(exampleTest SHARED
    lib/exampleTest.cpp
)

set_target_properties(exampleTest PROPERTIES PUBLIC_HEADER lib/exampleTest.h)

target_link_libraries(exampleTest
PRIVATE
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS exampleTest
    DESTINATION lib)

# -------------------- JoinTest plugin ----------------------
 message("Setting up ${PLUGIN_NAME}")

 find_package(${NAMESPACE}Plugins REQUIRED)
 find_package(${NAMESPACE}Definitions REQUIRED)
 find_package(CompileSettingsDebug CONFIG REQUIRED)

 add_library(${MODULE_NAME} SHARED
        Module.cpp
        JoinTest.cpp
        JoinTestImplementation.cpp
)

 set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

 target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

 install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

 write_config(${PLUGIN_NAME})
