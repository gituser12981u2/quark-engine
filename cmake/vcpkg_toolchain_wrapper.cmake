get_filename_component(QUARK_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(QUARK_VCPKG_ROOT "${QUARK_SOURCE_DIR}/vcpkg")

if(EXISTS "${QUARK_SOURCE_DIR}/.gitmodules")
  find_program(QUARK_GIT_EXECUTABLE git)
  if(QUARK_GIT_EXECUTABLE)
    execute_process(
      COMMAND "${QUARK_GIT_EXECUTABLE}" submodule update --init --recursive --force
      WORKING_DIRECTORY "${QUARK_SOURCE_DIR}"
      RESULT_VARIABLE quark_submodule_result
      OUTPUT_QUIET
      ERROR_QUIET
    )

    if(NOT quark_submodule_result EQUAL 0)
      message(WARNING "Unable to auto-update git submodules. Run 'git submodule update --init --recursive'.")
    endif()

    if(EXISTS "${QUARK_VCPKG_ROOT}/.git")
      foreach(quark_bootstrap_file bootstrap-vcpkg.sh bootstrap-vcpkg.bat)
        if(NOT EXISTS "${QUARK_VCPKG_ROOT}/${quark_bootstrap_file}")
          execute_process(
            COMMAND "${QUARK_GIT_EXECUTABLE}" -C "${QUARK_VCPKG_ROOT}" restore -- "${quark_bootstrap_file}"
            RESULT_VARIABLE quark_restore_result
            OUTPUT_QUIET
            ERROR_QUIET
          )

          if(NOT quark_restore_result EQUAL 0)
            message(WARNING "Unable to restore ${quark_bootstrap_file} in vcpkg submodule.")
          endif()
        endif()
      endforeach()
    endif()
  endif()
endif()

if(UNIX AND EXISTS "${QUARK_VCPKG_ROOT}/bootstrap-vcpkg.sh")
  execute_process(
    COMMAND chmod +x "${QUARK_VCPKG_ROOT}/bootstrap-vcpkg.sh"
    OUTPUT_QUIET
    ERROR_QUIET
  )
endif()

if(NOT EXISTS "${QUARK_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  message(FATAL_ERROR "Missing vcpkg toolchain file at ${QUARK_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

include("${QUARK_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
