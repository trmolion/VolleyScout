# Установка и пакеты для Windows: NSIS-инсталлятор и ZIP (portable).
#
# В пакет идёт ВСЁ содержимое проверенной папки build-win/deploy/ (её готовит
# scripts/deploy-windows.sh, проверяет scripts/check-deploy.sh) — отдельного списка
# файлов нет, поэтому инсталлятор не может разойтись с тем, что проверено.

set(VOLLEYSCOUT_DEPLOY_DIR "${CMAKE_BINARY_DIR}/deploy" CACHE PATH "Папка с развёрнутым приложением")

install(DIRECTORY "${VOLLEYSCOUT_DEPLOY_DIR}/" DESTINATION . USE_SOURCE_PERMISSIONS)

set(VOLLEYSCOUT_ICO "${PROJECT_SOURCE_DIR}/src/apps/resources/windows/app.ico")

set(CPACK_GENERATOR "NSIS;ZIP")
# Только наш компонент (install(DIRECTORY deploy/)). QXlsx подключён EXCLUDE_FROM_ALL и
# ничего не устанавливает; это страховка от чужих install()-правил в других компонентах.
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};VolleyScout;Unspecified;/")
set(CPACK_PACKAGE_NAME "VolleyScout")
set(CPACK_PACKAGE_VENDOR "${VOLLEYSCOUT_VENDOR}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_FILE_NAME "VolleyScout-${PROJECT_VERSION}-win64")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "VolleyScout")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "VolleyScout") # одна запись в «Установка и удаление программ» на все версии
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "VolleyScout" "VolleyScout") # ярлык в «Пуске»
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".") # exe лежит в корне установки, а не в bin\ (по умолчанию CPack)
set(CPACK_STRIP_FILES OFF) # бинарник уже без символов, DLL из MSYS2 не трогаем

# ZIP: portable — всё в одной папке VolleyScout-<версия>-win64/.
set(CPACK_ARCHIVE_ZIP_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_ARCHIVE_INCLUDE_TOPLEVEL_DIRECTORY ON) # только для ZIP; NSIS эту опцию не поддерживает

# NSIS.
set(CPACK_NSIS_PACKAGE_NAME "VolleyScout") # заголовок инсталлятора и папка в «Пуске»
set(CPACK_NSIS_DISPLAY_NAME "VolleyScout")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_MUI_ICON "${VOLLEYSCOUT_ICO}")
set(CPACK_NSIS_MUI_UNIICON "${VOLLEYSCOUT_ICO}")
set(CPACK_NSIS_INSTALLED_ICON_NAME "VolleyScout.exe") # иконка в «Установка и удаление программ»
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
# Заглушки NSIS 32-битные, поэтому запись об установке — в 32-битном представлении реестра
# (Wow6432Node). «Установка и удаление программ» показывает оба представления.
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "VolleyScout.exe")
set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
# CPACK_NSIS_BRANDING_TEXT не используем: CPack добавляет «/TRIM…», а makensis под Linux его не поддерживает.
set(CPACK_NSIS_DEFINES "BrandingText \\\"VolleyScout ${PROJECT_VERSION}\\\"")
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
# Ярлык на рабочем столе. Стандартный механизм CPack создаёт его только со страницы
# настройки PATH, которая нам не нужна, — поэтому явно; удаляется деинсталлятором.
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
    "CreateShortCut \\\"$DESKTOP\\\\VolleyScout.lnk\\\" \\\"$INSTDIR\\\\VolleyScout.exe\\\"")
# Sleep: деинсталлятор NSIS перезапускается из временной копии и удаляет оригинальный
# Uninstall.exe; пауза даёт оригиналу завершиться, иначе файл занят и остаётся в папке.
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
    "Sleep 1000\n  Delete \\\"$DESKTOP\\\\VolleyScout.lnk\\\"")

# Язык инсталлятора — только русский: берём шаблон NSIS из установленного CMake и оставляем
# в нём один MUI_LANGUAGE. Копию шаблона в репозиторий не кладём — она устаревала бы с CMake.
set(_nsis_template "${CMAKE_ROOT}/Modules/Internal/CPack/NSIS.template.in")
if(EXISTS "${_nsis_template}")
    file(READ "${_nsis_template}" _nsis)
    string(REGEX REPLACE "[ \t]*!insertmacro MUI_LANGUAGE \"[A-Za-z]+\"[^\n]*\n" "" _nsis "${_nsis}")
    # Язык объявляется после всех страниц MUI, включая страницы деинсталлятора.
    string(REPLACE "!insertmacro MUI_UNPAGE_INSTFILES"
                   "!insertmacro MUI_UNPAGE_INSTFILES\n\n  !insertmacro MUI_LANGUAGE \"Russian\"" _nsis "${_nsis}")
    set(_nsis_module_dir "${CMAKE_BINARY_DIR}/cpack-templates")
    file(WRITE "${_nsis_module_dir}/NSIS.template.in" "${_nsis}")
    set(CPACK_MODULE_PATH "${_nsis_module_dir}")
else()
    message(WARNING "Не найден шаблон NSIS ${_nsis_template}: инсталлятор будет на английском")
endif()

include(CPack)
