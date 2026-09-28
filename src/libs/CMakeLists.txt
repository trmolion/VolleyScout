qt_add_library(volleyscout_core STATIC
    include/volleyscout/command.h
    include/volleyscout/textformat.h
    include/volleyscout/scoutentry.h
    include/volleyscout/entrymodel.h
    include/volleyscout/entryproxymodel.h
    include/volleyscout/commandlistmodel.h
    include/volleyscout/rostermodel.h
    include/volleyscout/statistics.h
    include/volleyscout/statssheet.h
    include/volleyscout/statstablemodel.h
    include/volleyscout/scoutfiles.h
    include/volleyscout/xlsxexport.h
    src/command.cpp
    src/textformat.cpp
    src/scoutentry.cpp
    src/entrymodel.cpp
    src/entryproxymodel.cpp
    src/commandlistmodel.cpp
    src/rostermodel.cpp
    src/statistics.cpp
    src/statssheet.cpp
    src/statstablemodel.cpp
    src/scoutfiles.cpp
    src/xlsxexport.cpp
)

target_include_directories(volleyscout_core PUBLIC include)

# QZipWriter — приватный API QtGui; тот же, на котором построен сам QXlsx.
target_link_libraries(volleyscout_core
    PUBLIC  Qt6::Core Qt6::Gui
    PRIVATE Qt6::GuiPrivate QXlsx::QXlsx
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(volleyscout_core PRIVATE -Wall -Wextra)
endif()
