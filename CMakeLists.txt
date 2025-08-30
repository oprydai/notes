cmake_minimum_required(VERSION 3.16)

project(Notes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Prefer Release by default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Find Qt 6 or 5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
  src/main.cpp
  src/db/DatabaseManager.h
  src/db/DatabaseManager.cpp
  src/utils/Roles.h
  src/ui/MainWindow.h
  src/ui/MainWindow.cpp
  src/ui/NoteListDelegate.h
  src/ui/NoteListDelegate.cpp
  src/ui/MarkdownHighlighter.h
  src/ui/MarkdownHighlighter.cpp
      src/ui/TextEditor.h
    src/ui/TextEditor.cpp
  src/ui/SettingsDialog.h
  src/ui/SettingsDialog.cpp
  src/ui/NotesModel.h
  src/ui/NotesModel.cpp
  resources/resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
  )
else()
  add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
  )
endif()

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

# On macOS and Windows, enable high-DPI scaling by default
if (APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_finalize_executable(${PROJECT_NAME})
endif()


