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
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network)

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
  src/sync/GoogleDriveManager.h
  src/sync/GoogleDriveManager.cpp
  src/sync/SyncManager.h
  src/sync/SyncManager.cpp
  src/sync/GoogleDriveConfig.h
  src/sync/GoogleDriveConfig.cpp
  src/sync/ConfigLoader.h
  src/sync/ConfigLoader.cpp
  src/ui/GoogleAuthDialog.h
  src/ui/GoogleAuthDialog.cpp
  src/utils/Logger.h
  src/utils/Logger.cpp
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
    Qt${QT_VERSION_MAJOR}::Network
)

# On macOS and Windows, enable high-DPI scaling by default
if (APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_finalize_executable(${PROJECT_NAME})
endif()

# Install the executable
install(TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install config file to a location where the app can find it
# The app looks for config files in multiple locations, including relative to the executable
install(FILES config/google_drive_config.ini.example
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
  RENAME google_drive_config.ini
)

# Install documentation
install(FILES README.md
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/${PROJECT_NAME}
)

# Install desktop file and icons
install(FILES debian/notes.desktop
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

# Install icons
install(DIRECTORY debian/icons/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
  FILES_MATCHING PATTERN "*.png" PATTERN "*.svg"
)


