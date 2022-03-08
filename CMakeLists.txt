project(pubviz)
cmake_minimum_required(VERSION 3.10)

find_package(GWEN_opengl REQUIRED HINTS ../GWEN/gwen)

find_package(pubsub REQUIRED HINTS ../pubsub_workspace/pubsub)

# Samples
add_executable(pubviz
  pubviz.cpp
  main.cpp
  OpenGLCanvas.cpp
  GraphCanvas.cpp)
target_include_directories(pubviz PUBLIC
  ${GWEN_INCLUDE_DIRECTORIES}
  ${pubsub_INCLUDE_DIRECTORIES})
target_link_libraries(pubviz GWEN_opengl GWEN_static pubsub)


set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
