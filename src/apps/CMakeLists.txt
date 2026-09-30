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
    include/selftest.h
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
    src/selftest.cpp
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
    Qt6::GuiPrivate # QZipReader в самопроверке
    Qt6::Svg
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

target_compile_definitions(VolleyScout PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(VolleyScout PRIVATE -Wall -Wextra)
endif()

if(WIN32)
    # Иконка exe (IDI_ICON1 — её же Qt берёт для окон) и VERSIONINFO.
    set(VS_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/windows/app.ico")
    set(VS_COMPANY "${VOLLEYSCOUT_VENDOR}")
    set(VS_DESCRIPTION "${PROJECT_DESCRIPTION}")
    configure_file(resources/windows/app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
    target_sources(VolleyScout PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    # .rc в UTF-8; без явной кодировки windres читает его как cp1252 и портит кириллицу.
    # (target_compile_options до windres-правила CMake не доносит — только CMAKE_RC_FLAGS.)
    string(APPEND CMAKE_RC_FLAGS " --codepage=65001")

    # Без консольного окна (qt_add_executable(WIN32) уже ставит это свойство — фиксируем явно).
    set_target_properties(VolleyScout PROPERTIES WIN32_EXECUTABLE ON)
    # Релизный бинарник без символов.
    target_link_options(VolleyScout PRIVATE "$<$<CONFIG:Release,MinSizeRel>:-s>")
endif()
