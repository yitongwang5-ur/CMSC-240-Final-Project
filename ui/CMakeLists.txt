cmake_minimum_required(VERSION 3.16)
project(dining_ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(APPLE AND NOT DEFINED CMAKE_PREFIX_PATH)
    execute_process(
        COMMAND brew --prefix qt
        OUTPUT_VARIABLE BREW_QT_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE BREW_QT_RESULT
    )
    if(BREW_QT_RESULT EQUAL 0 AND BREW_QT_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${BREW_QT_PREFIX}")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(UI_SOURCES
    main.cpp
    MainWindow.cpp
    DataLoader.cpp
    HistoryModel.cpp
    StatusBanner.cpp
    ChartsTab.cpp
    OverviewTab.cpp
    RecommendationsTab.cpp
)

set(UI_HEADERS
    MainWindow.h
    DataLoader.h
    HistoryModel.h
    StatusBanner.h
    ChartsTab.h
    OverviewTab.h
    RecommendationsTab.h
    Palette.h
)

qt_add_resources(UI_RESOURCES resources.qrc)

add_executable(dining_ui ${UI_SOURCES} ${UI_HEADERS} ${UI_RESOURCES})
target_link_libraries(dining_ui PRIVATE
    Qt6::Widgets
    Qt6::Charts
    nlohmann_json::nlohmann_json
)

set_target_properties(dining_ui PROPERTIES
    # macOS notifications only fire from apps with a bundle identifier.
    # Build as a proper .app bundle so macOS registers it with the
    # Notification Center and shows the app in System Settings → Notifications.
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER edu.richmond.diningdollars
    MACOSX_BUNDLE_BUNDLE_NAME "UR Dining Dollars"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    WIN32_EXECUTABLE OFF
)
