set(TARGET_NAME range-ai-lib)

set(SOURCES
    src/rai_claude_agent.cpp
)

set(HEADERS
    include/rai_claude_agent.h
)

add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        Qt6::Core
        Qt6::Network
        common_defines
        range-base-lib
)
