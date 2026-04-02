# Create a static library
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(range-ai-lib
    STATIC
        src/rai_agent_settings_manager.cpp
        src/rai_agent_settings.cpp
        src/rai_agent.cpp
        include/rai_agent_settings_manager.h
        include/rai_agent_settings.h
        include/rai_agent.h

)

# Include directories
target_include_directories(range-ai-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(range-ai-lib
    PUBLIC
        range-base-lib
        range-cloud-lib
        common_defines
)

target_link_libraries(range-ai-lib PRIVATE Qt6::Core)
