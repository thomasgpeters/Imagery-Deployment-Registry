# Deployment Registry — source build configuration

# Only build the Wt executable when Wt is available
if(Wt_FOUND)
    add_executable(deployment_registry
        main.cpp
        app/RegistryApplication.cpp
        api/AlsClient.cpp
        docker/DockerStatusProvider.cpp
        ui/MainLayout.cpp
        ui/DeploymentListView.cpp
        ui/DeploymentDetailView.cpp
    )

    # Place binary in the build root (not build/src/)
    set_target_properties(deployment_registry PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
    )

    target_include_directories(deployment_registry PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Suppress deprecation warnings from Wt headers
    target_compile_options(deployment_registry PRIVATE -Wno-deprecated-declarations)
    get_target_property(_wt_inc Wt::Wt INTERFACE_INCLUDE_DIRECTORIES)
    if(_wt_inc)
        target_include_directories(deployment_registry SYSTEM PRIVATE ${_wt_inc})
    endif()

    target_link_libraries(deployment_registry PRIVATE
        Wt::Wt
        Wt::HTTP
        Boost::filesystem
        Boost::system
        Boost::program_options
        Threads::Threads
    )

    # Link nlohmann-json
    if(nlohmann_json_FOUND)
        target_link_libraries(deployment_registry PRIVATE nlohmann_json::nlohmann_json)
    elseif(NLOHMANN_JSON_FOUND)
        target_include_directories(deployment_registry PRIVATE ${NLOHMANN_JSON_INCLUDE_DIRS})
    endif()

    # --- Copy Wt standard resources into build directory ---
    get_target_property(_wt_include_dirs Wt::Wt INTERFACE_INCLUDE_DIRECTORIES)
    if(_wt_include_dirs)
        list(GET _wt_include_dirs 0 _wt_include_dir)
        get_filename_component(_wt_prefix "${_wt_include_dir}" DIRECTORY)
        set(_wt_resources_src "${_wt_prefix}/share/Wt/resources")
    endif()

    if(EXISTS "${_wt_resources_src}")
        message(STATUS "Copying Wt resources from ${_wt_resources_src}")
        file(COPY "${_wt_resources_src}" DESTINATION "${CMAKE_BINARY_DIR}")
    else()
        message(WARNING "Wt resources not found at ${_wt_resources_src} — "
            "you may need to pass --resources-dir at runtime")
    endif()

    install(TARGETS deployment_registry RUNTIME DESTINATION bin)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/ DESTINATION share/deployment-registry/resources)
else()
    message(STATUS "Wt not found — skipping Deployment Registry UI executable")
endif()
