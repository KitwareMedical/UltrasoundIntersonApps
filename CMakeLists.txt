cmake_minimum_required( VERSION 3.5 )

project( UltrasoundOpticNerveUI CXX )


FIND_PACKAGE( IntersonArraySDKCxx REQUIRED )


find_package( ITK REQUIRED )
include( ${ITK_USE_FILE} )


find_package(Qt5 COMPONENTS Core Widgets REQUIRED)


set( source_files
  OpticNerveUIDriver.cxx
  OpticNerveUI.cxx
  OpticNerveEstimator.cxx
)


include_directories( ${CMAKE_SOURCE_DIR} )
include_directories( ${UltrasoundOpticNerveUI_SOURCE_DIR} )
include_directories( ${IntersonArraySDKCxx_INCLUDE_DIRS} )
#TODO: Hardcode hack above is wrong needs to be fixed in 
#      IntersonArraySDKCxx
include_directories( "C:/Code/IntersonArraySDKCxx/include" )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions( -DNOMINMAX )

qt5_wrap_ui( OpticNerveUI_H OpticNerve.ui )
qt5_wrap_cpp( source_files_moc OpticNerveUI.h )

add_executable( UltrasoundOpticNerveUI
  ${source_files}
  ${OpticNerveUI_H}
  ${source_files_moc}
)

target_link_libraries( UltrasoundOpticNerveUI
  ${ITK_LIBRARIES}
  #TODO Hardcode hack take correct library from INtersonArraySDKCxx
  IntersonArrayCxx.lib
  Qt5::Widgets
)
