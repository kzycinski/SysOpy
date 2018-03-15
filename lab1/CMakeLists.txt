cmake_minimum_required(VERSION 3.6)
project(lab12)


#run by typing 'make test' in console
#compile single element using eg. make shmain

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ")

add_library (sharedlib SHARED library.c)
add_library (staticlib STATIC library.c)
add_library (dynamiclib MODULE library.c)

add_executable(main main.c)
add_executable(shmain main.c)
add_executable(dmain dyn_main.c)

target_link_libraries(main staticlib)
target_link_libraries(shmain sharedlib)
target_link_libraries(dmain ${CMAKE_DL_LIBS})


foreach(loop_var -O -O1 -O2 -O3 -Os)

    set(shared_lib "sharedlib${loop_var}")
    set(static_lib "staticlib${loop_var}")
    set(dynamic_lib "dynamiclib${loop_var}")

    add_library (${shared_lib} SHARED library.c)
    add_library (${static_lib} STATIC library.c)
    add_library (${dynamic_lib} MODULE library.c)

    target_compile_options(${shared_lib} PUBLIC ${loop_var})
    target_compile_options(${static_lib} PUBLIC ${loop_var})
    target_compile_options(${dynamic_lib} PUBLIC ${loop_var})

    set(main "main${loop_var}")
    set(shmain "shmain${loop_var}")
    set(dmain "dmain${loop_var}")

    add_executable(${main} main.c)
    add_executable(${shmain} main.c)
    add_executable(${dmain} dyn_main.c)

    target_compile_options(${main} PUBLIC ${loop_var})
    target_compile_options(${shmain} PUBLIC ${loop_var})
    target_compile_options(${dmain} PUBLIC ${loop_var})

    target_link_libraries(${main} ${static_lib})
    target_link_libraries(${shmain} ${shared_lib})
    target_link_libraries(${dmain} ${CMAKE_DL_LIBS})
endforeach(loop_var)

enable_testing()

add_test(test_static main raport3a.txt 99999 999 0 find 200 ada 90000 dta 90000 test_static)
add_test(test_shared shmain raport3a.txt 99999 999 0 find 200 ada 90000 dta 90000 test_shared)
add_test(test_dynamic main raport3a.txt 99999 999 0 find 200 ada 90000 dta 90000 test_dynamic)
add_test(test_static_d main raport3a.txt 99999 999 1 find 200 ada 90000 dta 90000 test_static_d)
add_test(test_shared_d shmain raport3a.txt 99999 999 1 find 200 ada 90000 dta 90000 test_shared_d)
add_test(test_dynamic_d main raport3a.txt 99999 999 1 find 200 ada 90000 dta 90000 test_dynamic_d)

foreach(loop_var -O -O1 -O2 -O3 -Os)
    set(main "main${loop_var}")
    set(shmain "shmain${loop_var}")
    set(dmain "dmain${loop_var}")

    set(test_static "test_static${loop_var}")
    set(test_shared "test_shared${loop_var}")
    set(test_dynamic "test_dynamic${loop_var}")
    set(test_static_d "test_static_d${loop_var}")
    set(test_shared_d "test_shared_d${loop_var}")
    set(test_dynamic_d "test_dynamic_d${loop_var}")

    add_test(${test_static} ${main} raport3b.txt 99999 999 0 find 200 ada 90000 dta 90000 ${test_static})
    add_test(${test_shared} ${shmain} raport3b.txt 99999 999 0 find 200 ada 90000 dta 90000 ${test_shared})
    add_test(${test_dynamic} ${dmain} raport3b.txt 99999 999 0 find 200 ada 90000 dta 90000 ${test_dynamic})
    add_test(${test_static_d} ${main} raport3b.txt 99999 999 1 find 200 ada 90000 dta 90000 ${test_static_d})
    add_test(${test_shared_d} ${shmain} raport3b.txt 99999 999 1 find 200 ada 90000 dta 90000 ${test_shared_d})
    add_test(${test_dynamic_d} ${dmain} raport3b.txt 99999 999 1 find 200 ada 90000 dta 90000 ${test_dynamic_d})
endforeach(loop_var)

