qt_add_executable(VolleyScout WIN32 MACOSX_BUNDLE
    include/mainwindow.h
    include/formationpage.h
    include/tablepage.h
    include/historypanel.h
    include/videopanel.h
    include/timelinewidget.h
    include/setdelegate.h
    include/sheetdelegate.h
    include/stylegallery.h
    include/screenshots.h
    include/theme/themetokens.h
    include/theme/thememanager.h
    include/theme/beachstyle.h
    include/theme/svgiconengine.h
    include/theme/icons.h
    include/theme/fonts.h
    src/main.cpp
    src/mainwindow.cpp
    src/formationpage.cpp
    src/tablepage.cpp
    src/historypanel.cpp
    src/videopanel.cpp
    src/timelinewidget.cpp
    src/setdelegate.cpp
    src/sheetdelegate.cpp
    src/stylegallery.cpp
    src/screenshots.cpp
    src/theme/themetokens.cpp
    src/theme/thememanager.cpp
    src/theme/beachstyle.cpp
    src/theme/svgiconengine.cpp
    src/theme/icons.cpp
    src/theme/fonts.cpp
    resources/resources.qrc
)

set_target_properties(VolleyScout PROPERTIES AUTORCC ON)

target_include_directories(VolleyScout PRIVATE include)

target_link_libraries(VolleyScout PRIVATE
    volleyscout_core
    Qt6::Widgets
    Qt6::Svg
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

target_compile_definitions(VolleyScout PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(VolleyScout PRIVATE -Wall -Wextra)
endif()
