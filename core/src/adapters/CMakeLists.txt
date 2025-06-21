# Ensure adapter sources are included in both the main library and test targets

# Add adapter sources to the main parser_core library
target_sources(parser_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/adapter_registry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/language_adapter.c
)

# Make sure the include directories are set
target_include_directories(parser_core PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/include/scopemux
    ${CMAKE_SOURCE_DIR}/core/include/tree_sitter
)
