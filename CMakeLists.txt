cmake_minimum_required(VERSION 3.0)

project(GirderFileBrowser)

# Automoc, AutoUIC, and AutoRCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Network Widgets REQUIRED)

set(SRCS
  girderfilebrowser.cxx
  girderrequest.cxx
  girderauthenticator.cxx
  girderfilebrowserfetcher.cxx
  ui/girderlogindialog.cxx
  ui/girderfilebrowserdialog.cxx
  ui/girderfilebrowserlistview.cxx
  utils.cxx
)

#resource files
set(resources
 resources.qrc
)
qt5_add_resources(res_srcs ${resources})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ui")

add_executable(girderfilebrowser MACOSX_BUNDLE WIN32 ${SRCS} ${res_srcs})

qt5_use_modules(girderfilebrowser Core Network Widgets)
