cmake_minimum_required(VERSION 3.22)

project(matlog LANGUAGES CXX)

file(GLOB_RECURSE source_files ${CMAKE_HOME_DIRECTORY}/src/*.cpp)

add_executable(${PROJECT_NAME} ${source_files})

add_subdirectory(${CMAKE_HOME_DIRECTORY}/external/BuDDy)
target_link_libraries(${PROJECT_NAME} PUBLIC bdd)

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_HOME_DIRECTORY}/external/BuDDy/src
                           PUBLIC ${CMAKE_HOME_DIRECTORY}/external/cli11
                         )

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(
    copy_compile_commands
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_BINARY_DIR}/compile_commands.json
      ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Copying compile_commands.json to ${CMAKE_SOURCE_DIR}")

  add_dependencies(${PROJECT_NAME} copy_compile_commands)
endif()
