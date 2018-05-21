cmake_minimum_required(VERSION 3.9)
project(lab8 C)

set(CMAKE_C_STANDARD 99)

add_executable(filter main.c)

target_link_libraries(filter pthread)
target_link_libraries(filter m)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lm")


#---------------------------------------------------------------------------------------------

enable_testing()

foreach(threads 1 2 4 8)
    set(test_filter "test_filter_${threads}")
    set(result "result_${threads}.pgm")
    add_test(
            NAME ${test_filter}
            COMMAND filter ${threads} saturn.pgm filter.txt ${result}
    )
endforeach(threads)

