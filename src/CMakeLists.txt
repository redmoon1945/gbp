cmake_minimum_required(VERSION 3.5)

project(gbp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Charts Widgets LinguistTools PrintSupport )

set(TS_FILES gbp_en.ts gbp_fr.ts)

set(PROJECT_SOURCES

    aboutdialog.cpp
    aboutdialog.h
    aboutdialog.ui
    analysisdialog.cpp
    analysisdialog.h
    analysisdialog.ui
    combinedfestreams.cpp
    combinedfestreams.cpp
    combinedfestreams.h
    combinedfestreams.h
    currencyhelper.cpp
    currencyhelper.h
    datehelper.cpp
    datehelper.h
    dateintervaldialog.cpp
    dateintervaldialog.h
    dateintervaldialog.ui
    daterange.cpp
    daterange.h
    editgrowthelementdialog.cpp
    editgrowthelementdialog.h
    editgrowthelementdialog.ui
    editirregulardialog.cpp
    editirregulardialog.h
    editirregulardialog.ui
    editirregularelementdialog.cpp
    editirregularelementdialog.h
    editirregularelementdialog.ui
    editirregularmodel.cpp
    editirregularmodel.h
    editperiodicdialog.cpp
    editperiodicdialog.h
    editperiodicdialog.ui
    editscenariodialog.cpp
    editscenariodialog.h
    editscenariodialog.ui
    editvariablegrowthdialog.cpp
    editvariablegrowthdialog.h
    editvariablegrowthdialog.ui
    editvariablegrowthmodel.cpp
    editvariablegrowthmodel.h
    fe.cpp
    fe.h
    festreamdef.cpp
    festreamdef.h
    gbpcontroller.cpp
    gbpcontroller.h
    growth.cpp
    growth.h
    irregularfestreamdef.cpp
    irregularfestreamdef.h
    loadirregulartextfiledialog.cpp
    loadirregulartextfiledialog.h
    loadirregulartextfiledialog.ui
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    optionsdialog.cpp
    optionsdialog.h
    optionsdialog.ui
    periodicfestreamdef.cpp
    periodicfestreamdef.h
    plaintexteditiondialog.cpp
    plaintexteditiondialog.h
    plaintexteditiondialog.ui
    qcustomplot.cpp
    qcustomplot.h
    scenario.cpp
    scenario.h
    scenariofetablemodel.cpp
    scenariofetablemodel.h
    scenariopropertiesdialog.h
    scenariopropertiesdialog.cpp
    scenariopropertiesdialog.ui
    selectcountrydialog.cpp
    selectcountrydialog.h
    selectcountrydialog.ui
    util.cpp
    util.h

    ${TS_FILES}
)




# Only call lrelease on each translation files to create qm files.
# This command assumes that the ts files already exists (manually created).
#qt6_add_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

# Call lupdate to generate (or update already existing) ts files from source
# code (.cpp and .ui) code AND generate qm files. We can call it with a source
# directory OR individual files to search for strings to translate. The ts files
# are generated in the source directory and the qm files are created in the
# build directory, as it should, because we don't want to pollute our source
# directory with generated binary files.
# Recall that the ts files are edited by translators, while qm files are just
# binary representation of the ts file for perfomance and size optimization.
qt6_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

#qt6_add_lupdate(gbp TS_FILES ${TS_FILES})


qt6_add_executable(gbp MANUAL_FINALIZATION ${PROJECT_SOURCES} ${QM_FILES}
    gbp_resources.qrc
)


target_link_libraries(gbp PRIVATE Qt6::Widgets Qt6::Charts Qt6::PrintSupport)

set_target_properties(gbp PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

#Note that qt_standard_project_setup() pulls in CMake's GNUInstallDirs.cmake. This is what defines the CMAKE_INSTALL_BINDIR variable.
#include(GNUInstallDirs)

# install(TARGETS gbp
#     BUNDLE DESTINATION .
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )


# for deployment (CDU)
# qt_generate_deploy_app_script(
#     TARGET gbp
#     OUTPUT_SCRIPT deploy_script
#     NO_UNSUPPORTED_PLATFORM_ERROR
# )
# install(SCRIPT ${deploy_script})

include(GNUInstallDirs)
install(TARGETS gbp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(gbp)

