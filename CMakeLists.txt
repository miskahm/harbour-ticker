cmake_minimum_required(VERSION 3.1)
project(harbour-ticker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 14)

find_package(Qt5 COMPONENTS Core Qml Quick Network REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SAILFISHAPP REQUIRED sailfishapp)

add_executable(harbour-ticker
    src/main.cpp
    src/tickercontroller.cpp
)

target_link_libraries(harbour-ticker
    Qt5::Core Qt5::Qml Qt5::Quick Qt5::Network
    ${SAILFISHAPP_LIBRARIES}
)
target_include_directories(harbour-ticker PRIVATE ${SAILFISHAPP_INCLUDE_DIRS})

install(TARGETS harbour-ticker DESTINATION bin)
install(DIRECTORY qml/ DESTINATION share/harbour-ticker/qml)
install(FILES harbour-ticker.desktop DESTINATION share/applications)
install(FILES icons/86x86/harbour-ticker.png DESTINATION share/icons/hicolor/86x86/apps)
install(FILES icons/108x108/harbour-ticker.png DESTINATION share/icons/hicolor/108x108/apps)
install(FILES icons/128x128/harbour-ticker.png DESTINATION share/icons/hicolor/128x128/apps)
install(FILES icons/172x172/harbour-ticker.png DESTINATION share/icons/hicolor/172x172/apps)
