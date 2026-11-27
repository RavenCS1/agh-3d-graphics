cmake_minimum_required(VERSION 3.16)
project(Grafika3D LANGUAGES CXX)

# Kod jest zgodny z C++11 (jak wymagaja materialy). Kompilujemy w C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- SFML ---------------------------------------------------------------------
# 1) Najpierw szukamy SFML 2.x zainstalowanego w systemie (np. Ubuntu: libsfml-dev).
# 2) Jesli go nie ma, pobieramy SFML 2.6.2 z GitHuba (API zgodne z 2.5.1 z labow).
#    Na Windows nie trzeba wtedy NuGeta - wystarczy "Open Folder" w Visual Studio.
option(G3D_FETCH_SFML "Pobierz SFML 2.6.2, jesli nie ma go w systemie" ON)

find_package(SFML 2.5 COMPONENTS graphics window system QUIET)
if(NOT SFML_FOUND)
    if(NOT G3D_FETCH_SFML)
        message(FATAL_ERROR "Nie znaleziono SFML 2.x. Zainstaluj go albo wlacz G3D_FETCH_SFML.")
    endif()
    message(STATUS "SFML 2.x nie znaleziony w systemie - pobieram SFML 2.6.2")
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.2
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(SFML)
endif()

find_package(OpenGL REQUIRED)

# --- wspolna konfiguracja programow ------------------------------------------
# g3d_add_app(<cel> <katalog> <pliki...>)
#  - linkuje SFML + OpenGL + GLU,
#  - kopiuje assets/ i shaders/ obok pliku wykonywalnego (build/bin/<katalog>/),
#  - w Visual Studio ustawia katalog roboczy debuggera na folder z kodem.
function(g3d_add_app target dir)
    add_executable(${target} ${ARGN})
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE
        sfml-graphics sfml-window sfml-system OpenGL::GL OpenGL::GLU)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
        target_compile_definitions(${target} PRIVATE NOMINMAX _USE_MATH_DEFINES)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    # Kazdy program we wlasnym katalogu (bin/lab05/, bin/miniprojekt1/ ...), razem ze swoimi zasobami.
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${dir}"
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/${dir}")
    foreach(res assets shaders)
        if(EXISTS "${PROJECT_SOURCE_DIR}/${dir}/${res}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${PROJECT_SOURCE_DIR}/${dir}/${res}"
                        "$<TARGET_FILE_DIR:${target}>/${res}")
        endif()
    endforeach()
endfunction()

g3d_add_app(lab01 lab01 lab01/main.cpp)
g3d_add_app(lab02 lab02 lab02/main.cpp)
g3d_add_app(lab03 lab03 lab03/main.cpp)
g3d_add_app(lab04 lab04 lab04/main.cpp)
g3d_add_app(lab05 lab05 lab05/main.cpp)
g3d_add_app(miniprojekt1 miniprojekt1
    miniprojekt1/main.cpp
    miniprojekt1/mission.cpp
    miniprojekt1/hangar.cpp
    miniprojekt1/hud_view.cpp)

# Punkt startowy na kolejne laboratoria (wszystkie mechanizmy g3d w jednym miejscu).
g3d_add_app(szablon szablon szablon/main.cpp)
