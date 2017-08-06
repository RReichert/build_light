set(SOURCE_FILES gpio.cpp job_monitor.cpp)
set(HEADER_FILES gpio.h job_monitor.h)

set(CMAKE_AUTOMOC true)
set(CMAKE_INCLUDE_CURRENT_DIR true)

find_package(Qt5 COMPONENTS Core Network REQUIRED)

add_executable(${CMAKE_PROJECT_NAME} main.cpp ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Core Qt5::Network)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC Qt5::Core Qt5::Network)