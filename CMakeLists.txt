# Create a static library
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(range-ai-lib
    STATIC
        src/rai_anthropic_agent.cpp
        src/rai_anthropic_agent_settings.cpp
        src/rai_open_ai_agent.cpp
        src/rai_open_ai_agent_settings.cpp
        include/rai_anthropic_agent.h
        include/rai_anthropic_agent_settings.h
        include/rai_open_ai_agent.h
        include/rai_open_ai_agent_settings.h
)

# Include directories
target_include_directories(range-ai-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(range-ai-lib
    PUBLIC
        range-base-lib
        Qt6::Network
        common_defines
)

target_link_libraries(range-ai-lib PRIVATE Qt6::Core)
