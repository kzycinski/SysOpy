cmake_minimum_required(VERSION 3.6)
project(lab1)


#run by typing 'make test' in console
#compile single element using eg. make shmain

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ")

#building libs
add_library (staticlib STATIC library.c)
add_library (sharedlib SHARED library.c)
add_library (dynamiclib MODULE library.c)

#building execs
add_executable(main main.c)
add_executable(shmain main.c)
add_executable(dmain main.c)

#dynamic main flag
target_compile_options(dmain PUBLIC -DDLL)

#libraries link
target_link_libraries(main staticlib)
target_link_libraries(shmain sharedlib)
target_link_libraries(dmain ${CMAKE_DL_LIBS})


foreach(option -O -O1 -O2 -O3 -Os)

    #each lib will be compiled with option added to its name
    set(shared_lib "sharedlib${option}")
    set(static_lib "staticlib${option}")
    set(dynamic_lib "dynamiclib${option}")

    add_library (${shared_lib} SHARED library.c)
    add_library (${static_lib} STATIC library.c)
    add_library (${dynamic_lib} MODULE library.c)

    target_compile_options(${shared_lib} PUBLIC ${option})
    target_compile_options(${static_lib} PUBLIC ${option})
    target_compile_options(${dynamic_lib} PUBLIC ${option})

    #each main will also be compiled with option added to name
    set(main "main${option}")
    set(shmain "shmain${option}")
    set(dmain "dmain${option}")

    add_executable(${main} main.c)
    add_executable(${shmain} main.c)
    add_executable(${dmain} main.c)

    target_compile_options(${main} PUBLIC ${option})
    target_compile_options(${shmain} PUBLIC ${option})
    target_compile_options(${dmain} PUBLIC ${option})
    target_compile_options(${dmain} PUBLIC -DDLL)

    target_link_libraries(${main} ${static_lib})
    target_link_libraries(${shmain} ${shared_lib})
    target_link_libraries(${dmain} ${CMAKE_DL_LIBS})
endforeach(option)


#---------------------------------------------------------------------------------------------
#tests
enable_testing()

#each test builds array [99999][999] and uses 3 options - find element that has neares ASCII sum to element [200],
#alternately deleting and adding first 90000 blocks and deleting then adding 90000 blocks
add_test(test_static main raport3a.txt 99999 999 1 find 200 ada 90000 dta 90000 test_static)
add_test(test_shared shmain raport3a.txt 99999 999 1 find 200 ada 90000 dta 90000 test_shared)
add_test(test_dynamic main raport3a.txt 99999 999 1 find 200 ada 90000 dta 90000 test_dynamic)
add_test(test_static_d main raport3a.txt 99999 999 0 find 200 ada 90000 dta 90000 test_static_d)
add_test(test_shared_d shmain raport3a.txt 99999 999 0 find 200 ada 90000 dta 90000 test_shared_d)
add_test(test_dynamic_d main raport3a.txt 99999 999 0 find 200 ada 90000 dta 90000 test_dynamic_d)

foreach(option -O -O1 -O2 -O3 -Os)
    set(main "main${option}")
    set(shmain "shmain${option}")
    set(dmain "dmain${option}")

    #test names changed for better visibility in console
    set(test_static "test_static${option}")
    set(test_shared "test_shared${option}")
    set(test_dynamic "test_dynamic${option}")
    set(test_static_d "test_static_d${option}")
    set(test_shared_d "test_shared_d${option}")
    set(test_dynamic_d "test_dynamic_d${option}")

    add_test(${test_static} ${main} raport3b.txt 99999 999 1 find 200 ada 90000 dta 90000 ${test_static})
    add_test(${test_shared} ${shmain} raport3b.txt 99999 999 1 find 200 ada 90000 dta 90000 ${test_shared})
    add_test(${test_dynamic} ${dmain} raport3b.txt 99999 999 1 find 200 ada 90000 dta 90000 ${test_dynamic})
    add_test(${test_static_d} ${main} raport3b.txt 99999 999 0 find 200 ada 90000 dta 90000 ${test_static_d})
    add_test(${test_shared_d} ${shmain} raport3b.txt 99999 999 0 find 200 ada 90000 dta 90000 ${test_shared_d})
    add_test(${test_dynamic_d} ${dmain} raport3b.txt 99999 999 0 find 200 ada 90000 dta 90000 ${test_dynamic_d})
endforeach(option)

