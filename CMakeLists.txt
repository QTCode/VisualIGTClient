cmake_minimum_required(VERSION 3.1.0)
project(VisualIGTClient)

#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)

set(OpenIGTLink_SRC ${CMAKE_SOURCE_DIR}/../OpenIGTLink)
set(OpenIGTLink_DIR ${CMAKE_SOURCE_DIR}/../OpenIGTLink-build)

set(KIT_include_directories )
set(KIT_link_directories )
set(KIT_target_libraries )

set(CMAKE_MODULE_PATH
"${CMAKE_MODULE_PATH}"
${OpenIGTLink_DIR}
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

include(OpenIGTLinkConfig)

include_directories(
 ${CMAKE_CURRENT_SOURCE_DIR}
${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OpenIGTLink_INCLUDE_DIRS}
)

link_directories(
  ${KIT_link_directories}
)

set(SRCS
  main.cpp  
  IGTLinkClientWidget.h
  IGTLinkClientWidget.cpp
VisualOpenIGTLinkClient.h
VisualOpenIGTLinkClient.cpp
)

set(MOC_SRCS
  IGTLinkClientWidget.h
VisualOpenIGTLinkClient.h
)

set(UI_SRCS
  IGTLinkClientWidget.ui
)

set(RCC_SRCS
  IGTLinkClientWidget.qrc
)

qt5_wrap_cpp(KIT_MOC_CXX ${MOC_SRCS})
qt5_wrap_ui(KIT_UI_CXX ${UI_SRCS})
qt5_add_resources(KIT_RCC_CXX ${RCC_SRCS})

# Group files
source_group("Resources" FILES
  ${UI_SRCS}
  ${RCC_SRCS}
)

source_group("Generated" FILES
   ${KIT_MOC_CXX}
   ${KIT_UI_CXX}
   ${KIT_RCC_CXX}
  )

# Source files
set(Source_SRCS
  ${SRCS}
  ${KIT_MOC_CXX}
  ${KIT_UI_CXX}
  ${KIT_RCC_CXX}
  )

#set(CONSOLEFLAG)
#if(NOT BUILD_CONSOLE)
 # set(CONSOLEFLAG WIN32)
#endif(NOT BUILD_CONSOLE)

add_executable( ${PROJECT_NAME} ${CONSOLEFLAG}
  ${Source_SRCS}
)
  
target_link_libraries(${PROJECT_NAME}
  ${KIT_target_libraries}
   ${OpenIGTLink_LIBRARIES}

)

qt5_use_modules(${PROJECT_NAME} Widgets Xml)
