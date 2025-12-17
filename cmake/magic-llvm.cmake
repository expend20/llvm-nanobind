find_package(LLVM CONFIG QUIET)
if(NOT LLVM_FOUND)
    if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
        execute_process(
            COMMAND brew --prefix llvm
            RESULT_VARIABLE BREW_LLVM
            OUTPUT_VARIABLE BREW_LLVM_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(BREW_LLVM EQUAL 0 AND EXISTS "${BREW_LLVM_PREFIX}")
            list(APPEND CMAKE_PREFIX_PATH "${BREW_LLVM_PREFIX}")
            message(STATUS "Found LLVM keg installed by Homebrew at ${BREW_LLVM_PREFIX}")
        else()
            message(WARNING "LLVM not found, to install: brew install llvm")
        endif()
    endif()
endif()
