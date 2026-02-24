# Deployment Registry — test build configuration

# Model tests (no Wt dependency — pure C++ + nlohmann-json)
add_executable(dr_test_model
    test_model.cpp
)

target_include_directories(dr_test_model PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(dr_test_model PRIVATE
    Threads::Threads
)

if(nlohmann_json_FOUND)
    target_link_libraries(dr_test_model PRIVATE nlohmann_json::nlohmann_json)
elseif(NLOHMANN_JSON_FOUND)
    target_include_directories(dr_test_model PRIVATE ${NLOHMANN_JSON_INCLUDE_DIRS})
endif()

add_test(NAME model_tests COMMAND dr_test_model)
