project(examples)

file(GLOB EXAMPLE_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(EXAMPLE ${EXAMPLE_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE})
        if (NOT ${EXAMPLE} STREQUAL "python")
            add_subdirectory(${EXAMPLE})
        endif ()
    endif()
endforeach()
