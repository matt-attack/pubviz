project(pubviz)
cmake_minimum_required(VERSION 3.10)

find_package(GWEN_opengl REQUIRED HINTS ../GWEN/gwen)

find_package(pubsub REQUIRED HINTS ../pubsub_workspace/pubsub)

find_package(librucksack REQUIRED)
find_package(PCL 1.12 REQUIRED)

# Pub
add_executable(pubviz WIN32
  controls/pubviz.cpp
  tools/pubviz.cpp
  controls/OpenGLCanvas.cpp
  controls/GraphCanvas.cpp
  controls/GraphBase.cpp
  controls/Parameters.cpp
  LocalXY.h 
  plugins/Marker.h
  plugins/LidarCalibration.cpp
)
target_include_directories(pubviz PUBLIC
  ${PCL_INCLUDE_DIRS}
  "C:/Program Files/PCL 1.12.1/3rdParty/VTK/include/vtk-9.1"
  ${GWEN_INCLUDE_DIRECTORIES}
  ${pubsub_INCLUDE_DIRECTORIES})
link_directories(${PCL_LIBRARY_DIRS})
target_link_libraries(pubviz GWEN_opengl GWEN_static pubsub ${PCL_LIBRARIES})

# Sackviz
#add_executable(sackviz
#  controls/sackviz.cpp
#  tools/sackviz.cpp
#  controls/GraphBase.cpp
#  controls/SackGraph.cpp
#  controls/SackViewer.cpp)
#target_include_directories(sackviz PUBLIC
#  ${GWEN_INCLUDE_DIRS}
#  ${pubsub_INCLUDE_DIRS}
#  ${librucksack_INCLUDE_DIRS})
#target_link_libraries(sackviz GWEN_opengl GWEN_static ${pubsub_LIBRARIES} ${librucksack_LIBRARIES})
#set_property(TARGET sackviz PROPERTY CXX_STANDARD 11)

# Canviz
add_executable(canviz
  controls/canviz.cpp
  tools/canviz.cpp
)
target_include_directories(canviz PUBLIC
  ${GWEN_INCLUDE_DIRS}
  ${pubsub_INCLUDE_DIRS}
  ${librucksack_INCLUDE_DIRS})
target_link_libraries(canviz GWEN_opengl GWEN_static ${pubsub_LIBRARIES} ${librucksack_LIBRARIES})
set_property(TARGET canviz PROPERTY CXX_STANDARD 11)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
