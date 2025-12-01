cmake_minimum_required(VERSION 3.15)
project(DerivX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки
option(USE_CPPRESTSDK "Use cpprestsdk for REST API" ON)
option(USE_CROW "Use Crow for REST API" OFF)

# JSON библиотека
# Используем установленную версию (brew install nlohmann-json)
find_package(nlohmann_json REQUIRED)
message(STATUS "Found nlohmann/json")

# Выбор REST фреймворка
if(USE_CPPRESTSDK)
    find_package(cpprestsdk QUIET)
    if(cpprestsdk_FOUND)
        message(STATUS "Found cpprestsdk")
        # Проверяем какое имя target используется
        if(TARGET cpprestsdk::cpprest)
            set(REST_LIB cpprestsdk::cpprest)
        elseif(TARGET cpprestsdk::cpprestsdk)
            set(REST_LIB cpprestsdk::cpprestsdk)
        else()
            message(FATAL_ERROR "cpprestsdk target not found. Available targets may differ.")
        endif()
    else()
        message(FATAL_ERROR "cpprestsdk not found! Please install it:\n  brew install cpprestsdk\nOr set USE_CPPRESTSDK=OFF to use Crow instead")
    endif()
elseif(USE_CROW)
    # Crow будет включен как header-only
    add_subdirectory(backend/crow)
    set(REST_LIB crow)
else()
    message(FATAL_ERROR "No REST framework selected! Set USE_CPPRESTSDK=ON or USE_CROW=ON")
endif()

# Исходные файлы
set(BACKEND_SOURCES
    backend/src/main.cpp
    backend/src/option_pricing.cpp
    backend/src/volatility.cpp
    backend/src/api_handler.cpp
)

set(BACKEND_HEADERS
    backend/include/option_pricing.hpp
    backend/include/volatility.hpp
    backend/include/api_handler.hpp
)

# Исполняемый файл
add_executable(derivx_api
    ${BACKEND_SOURCES}
    ${BACKEND_HEADERS}
)

# Включаемые директории
target_include_directories(derivx_api PRIVATE
    backend/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Связывание библиотек
target_link_libraries(derivx_api PRIVATE
    ${REST_LIB}
    nlohmann_json::nlohmann_json
)

# Компиляционные флаги
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(derivx_api PRIVATE -Wall -Wextra -O2)
endif()

# Установка
install(TARGETS derivx_api DESTINATION bin)

