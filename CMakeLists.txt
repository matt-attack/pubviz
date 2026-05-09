project(pubviz)
cmake_minimum_required(VERSION 3.10)

find_package(GWEN_opengl REQUIRED)
find_package(pubsub REQUIRED)
find_package(pubsub_cpp REQUIRED)
find_package(librucksack REQUIRED)

# Pub
add_library(libpubviz
  controls/pubviz.cpp
  controls/OpenGLCanvas.cpp
  controls/GraphCanvas.cpp
  controls/GraphBase.cpp
  controls/Parameters.cpp
  LocalXY.h 
  plugins/Marker.h
)
target_include_directories(libpubviz PUBLIC
  ${GWEN_INCLUDE_DIRECTORIES}
  ${pubsub_INCLUDE_DIRS})
target_link_libraries(libpubviz PUBLIC GWEN_opengl GWEN_static pubsub_cpp GLU pubsub_msgs)
set_target_properties(libpubviz PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET libpubviz PROPERTY CXX_STANDARD 11)

add_executable(pubviz WIN32
  tools/pubviz.cpp
)
target_include_directories(pubviz PUBLIC
  ${GWEN_INCLUDE_DIRECTORIES}
  ${librucksack_DEBUG_LIBRARIES}
  ${librucksack_LIBRARIES})
target_link_libraries(pubviz libpubviz)

# Sackviz
add_executable(sackviz WIN32
  controls/sackviz.cpp
  tools/sackviz.cpp
  controls/GraphBase.cpp
  controls/SackGraph.cpp
  controls/SackViewer.cpp)
target_include_directories(sackviz PUBLIC
  ${GWEN_INCLUDE_DIRS}
  ${pubsub_INCLUDE_DIRS}
  ${librucksack_INCLUDE_DIRS})
target_link_libraries(sackviz GWEN_opengl GWEN_static ${pubsub_LIBRARIES} pubsub_cpp
  ${librucksack_LIBRARIES}
  ${librucksack_LIBRARIES})
set_property(TARGET sackviz PROPERTY CXX_STANDARD 11)

# Canviz
#add_executable(canviz
#  controls/canviz.cpp
#  tools/canviz.cpp
#  controls/GraphBase.cpp
#  controls/SackGraph.cpp
#  controls/SackViewer.cpp
#)
#target_include_directories(canviz PUBLIC
#  ${GWEN_INCLUDE_DIRS}
#  ${pubsub_INCLUDE_DIRS}
#  ${librucksack_INCLUDE_DIRS})
#target_link_libraries(canviz GWEN_opengl GWEN_static ${pubsub_LIBRARIES} ${librucksack_LIBRARIES})
#set_property(TARGET canviz PROPERTY CXX_STANDARD 11)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

install(TARGETS pubviz sackviz
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

set(CPACK_GENERATOR "TBZ2")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_PACKAGE_DIRECTORY ${PROJECT_BINARY_DIR}/package)

set(CPACK_SOURCE_IGNORE_FILES .git/ .github/ .vscode/ .mypy_cache/ _CPack_Packages/
${CMAKE_BINARY_DIR}/ ${PROJECT_BINARY_DIR}/
)
include(CPack)

function(my_export_target _target _include_dir)
    file(
        WRITE "${CMAKE_CURRENT_BINARY_DIR}/${_target}Config.cmake"
        "
            include(\"\$\{CMAKE_CURRENT_LIST_DIR\}/${_target}Targets.cmake\")
            set_property(
                TARGET ${_target}
                APPEND PROPERTY
                    INTERFACE_INCLUDE_DIRECTORIES \"${_include_dir}\"
            )
            set(${_target}_INCLUDE_DIRS \"${_include_dir}\")
            set(${_target}_LIBRARIES \"${_target}\")
        "
    )

    export(TARGETS ${_target} FILE "${CMAKE_CURRENT_BINARY_DIR}/${_target}Targets.cmake")

    # NOTE: The following call can pollute your PC's CMake package registry
    #       See comments/alternatives below
    export(PACKAGE ${_target})
endfunction(my_export_target)

my_export_target(libpubviz "${CMAKE_CURRENT_SOURCE_DIR}/include")
