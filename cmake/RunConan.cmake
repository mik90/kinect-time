# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake"
                EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_configure(REQUIRES ${CONAN_EXTRA_REQUIRES}
                      GENERATORS cmake_find_package)
conan_cmake_autodetect(settings)

set(CONAN_REMOTES conan-center)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE ${CONAN_REMOTES}
                    SETTINGS ${settings})