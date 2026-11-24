# Optional ONNX Runtime dependency for the neural vocoder.
#
# ONNX Runtime ships as a prebuilt NuGet package. When LESSAMPLER_ENABLE_NEURAL
# is on, this module downloads and unpacks the CPU package into lib/onnxruntime
# (unless ONNXRUNTIME_ROOT points at an existing installation), so MSVC users do
# not need to configure headers and import libraries by hand.
#
# The package is pinned. Update the version and SHA-256 together.

set(LESSAMPLER_ONNXRUNTIME_VERSION "1.32.0" CACHE STRING
        "ONNX Runtime NuGet package version used by the neural backend")
set(LESSAMPLER_ONNXRUNTIME_SHA256
        "a9150bc2992cbbb6722070da9910e1ffa989a8bfc67bdd5bb8e745ca755641db"
        CACHE STRING "SHA-256 of the ONNX Runtime NuGet package (empty disables verification)")
set(LESSAMPLER_ONNXRUNTIME_ROOT "${PROJECT_SOURCE_DIR}/lib/onnxruntime" CACHE PATH
        "Managed ONNX Runtime installation directory")
set(LESSAMPLER_ONNXRUNTIME_INCLUDE_DIR "" CACHE INTERNAL "")
set(LESSAMPLER_ONNXRUNTIME_LIBRARY "" CACHE INTERNAL "")
set(LESSAMPLER_ONNXRUNTIME_DLL "" CACHE INTERNAL "")
set(LESSAMPLER_ONNXRUNTIME_DLL_DIR "" CACHE INTERNAL "")

function(_lessampler_onnxruntime_rid out_var)
    if (WIN32)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64")
            set(${out_var} "win-arm64" PARENT_SCOPE)
        else ()
            set(${out_var} "win-x64" PARENT_SCOPE)
        endif ()
    elseif (APPLE)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(${out_var} "osx-arm64" PARENT_SCOPE)
        else ()
            set(${out_var} "osx-x64" PARENT_SCOPE)
        endif ()
    else ()
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(${out_var} "linux-arm64" PARENT_SCOPE)
        else ()
            set(${out_var} "linux-x64" PARENT_SCOPE)
        endif ()
    endif ()
endfunction()

function(lessampler_setup_onnxruntime)
    _lessampler_onnxruntime_rid(rid)

    set(root "${LESSAMPLER_ONNXRUNTIME_ROOT}")
    set(managed TRUE)
    if (ONNXRUNTIME_ROOT)
        set(root "${ONNXRUNTIME_ROOT}")
        set(managed FALSE)
    endif ()

    find_path(onnxruntime_include
            NAMES onnxruntime_cxx_api.h
            HINTS "${root}"
            PATH_SUFFIXES include build/native/include
    )
    find_library(onnxruntime_library
            NAMES onnxruntime
            HINTS "${root}"
            PATH_SUFFIXES lib lib64 "runtimes/${rid}/native"
    )

    if (NOT onnxruntime_include OR NOT onnxruntime_library)
        if (NOT managed)
            message(FATAL_ERROR
                    "ONNXRUNTIME_ROOT=${root} does not contain onnxruntime_cxx_api.h and the onnxruntime library. "
                    "Unset it to let lessampler manage lib/onnxruntime automatically.")
        endif ()

        set(nupkg "${root}/onnxruntime-${LESSAMPLER_ONNXRUNTIME_VERSION}.nupkg")
        if (NOT EXISTS "${nupkg}")
            set(download_url
                    "https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/${LESSAMPLER_ONNXRUNTIME_VERSION}/microsoft.ml.onnxruntime.${LESSAMPLER_ONNXRUNTIME_VERSION}.nupkg")
            set(hash_args)
            if (LESSAMPLER_ONNXRUNTIME_SHA256)
                set(hash_args EXPECTED_HASH SHA256=${LESSAMPLER_ONNXRUNTIME_SHA256})
            endif ()
            message(STATUS "Downloading ONNX Runtime ${LESSAMPLER_ONNXRUNTIME_VERSION} for ${rid}")
            file(DOWNLOAD "${download_url}" "${nupkg}"
                    ${hash_args}
                    TLS_VERIFY ON
                    STATUS download_status
                    SHOW_PROGRESS)
            list(GET download_status 0 download_code)
            list(GET download_status 1 download_message)
            if (NOT download_code EQUAL 0)
                file(REMOVE "${nupkg}")
                message(FATAL_ERROR "Cannot download ONNX Runtime: ${download_message}")
            endif ()
        endif ()

        set(extract_dir "${root}/_package")
        file(REMOVE_RECURSE "${extract_dir}")
        file(ARCHIVE_EXTRACT
                INPUT "${nupkg}"
                DESTINATION "${extract_dir}"
                PATTERNS "build/native/include/*" "runtimes/${rid}/native/*")
        if (NOT EXISTS "${extract_dir}/build/native/include/onnxruntime_cxx_api.h")
            message(FATAL_ERROR
                    "ONNX Runtime ${LESSAMPLER_ONNXRUNTIME_VERSION} does not ship a ${rid} build. "
                    "Use a version with this runtime identifier or set ONNXRUNTIME_ROOT.")
        endif ()

        file(MAKE_DIRECTORY "${root}/include" "${root}/lib" "${root}/bin")
        file(GLOB onnxruntime_headers "${extract_dir}/build/native/include/*.h")
        file(COPY ${onnxruntime_headers} DESTINATION "${root}/include")

        file(GLOB onnxruntime_native "${extract_dir}/runtimes/${rid}/native/*")
        foreach(native_file IN LISTS onnxruntime_native)
            get_filename_component(native_name "${native_file}" NAME)
            if (WIN32 AND native_name MATCHES "\\.dll$")
                file(COPY "${native_file}" DESTINATION "${root}/bin")
            elseif (native_name MATCHES "^(lib)?onnxruntime\\.(so|dylib|lib)$")
                file(COPY "${native_file}" DESTINATION "${root}/lib")
            endif ()
        endforeach()

        if (UNIX AND NOT APPLE)
            # NuGet's Linux payload has no SONAME symlink. The linker records
            # libonnxruntime.so.1, so provide it next to the real library.
            if (EXISTS "${root}/lib/libonnxruntime.so")
                file(REMOVE "${root}/lib/libonnxruntime.so.1")
                file(CREATE_LINK "libonnxruntime.so" "${root}/lib/libonnxruntime.so.1" SYMBOLIC)
            endif ()
        endif ()

        file(REMOVE_RECURSE "${extract_dir}")

        find_path(onnxruntime_include
                NAMES onnxruntime_cxx_api.h
                HINTS "${root}"
                PATH_SUFFIXES include)
        find_library(onnxruntime_library
                NAMES onnxruntime
                HINTS "${root}"
                PATH_SUFFIXES lib)
    endif ()

    if (NOT onnxruntime_include OR NOT onnxruntime_library)
        message(FATAL_ERROR "Cannot locate ONNX Runtime headers or library under ${root}")
    endif ()

    set(LESSAMPLER_ONNXRUNTIME_INCLUDE_DIR "${onnxruntime_include}" CACHE INTERNAL "" FORCE)
    set(LESSAMPLER_ONNXRUNTIME_LIBRARY "${onnxruntime_library}" CACHE INTERNAL "" FORCE)

    if (WIN32)
        find_file(onnxruntime_dll
                NAMES onnxruntime.dll
                HINTS "${root}"
                PATH_SUFFIXES bin "runtimes/${rid}/native"
        )
        if (onnxruntime_dll)
            get_filename_component(onnxruntime_dll_dir "${onnxruntime_dll}" DIRECTORY)
            set(LESSAMPLER_ONNXRUNTIME_DLL "${onnxruntime_dll}" CACHE INTERNAL "" FORCE)
            set(LESSAMPLER_ONNXRUNTIME_DLL_DIR "${onnxruntime_dll_dir}" CACHE INTERNAL "" FORCE)
        else ()
            message(WARNING "onnxruntime.dll was not found under ${root}; add it to PATH when running lessampler")
        endif ()
    endif ()

    message(STATUS "ONNX Runtime: ${onnxruntime_library}")
endfunction()

function(lessampler_link_onnxruntime target)
    target_include_directories(${target} SYSTEM PRIVATE "${LESSAMPLER_ONNXRUNTIME_INCLUDE_DIR}")
    target_link_libraries(${target} PRIVATE "${LESSAMPLER_ONNXRUNTIME_LIBRARY}")
endfunction()

function(lessampler_copy_onnxruntime_dll target)
    if (WIN32 AND LESSAMPLER_ONNXRUNTIME_DLL)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LESSAMPLER_ONNXRUNTIME_DLL}" "$<TARGET_FILE_DIR:${target}>"
                VERBATIM)
    endif ()
endfunction()
