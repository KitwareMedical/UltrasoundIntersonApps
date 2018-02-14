############################################################################
#
# Library:   UltrasoundOpticNerveUI
#
# Copyright 2010 Kitware Inc. 28 Corporate Drive,
# Clifton Park, NY, 12065, USA.
#
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################
cmake_minimum_required( VERSION 3.5 )

project( UltrasoundIntersonApps CXX )

option( Build_Spectroscopy ON )

#Required packages and libraries
find_package( IntersonArraySDKCxx REQUIRED )

find_package( PlusLib REQUIRED )
include( ${PlusLib_USE_FILE} )

find_package( ITK REQUIRED )

if( ${Build_Spectroscopy} )
find_package( ITK COMPONENTS
  Ultrasound
  ITKBinaryMathematicalMorphology
  ITKCommon
  ITKDistanceMap
  ITKImageFilterBase
  ITKImageFunction
  ITKImageFusion
  ITKImageGrid
  ITKImageIntensity
  ITKLabelMap
  ITKMathematicalMorphology
  ITKMetricsv4
  ITKOptimizersv4
  ITKRegistrationMethodsv4
  ITKSmoothing
  ITKSpatialObjects
  ITKThresholding
  ITKTransform
  ITKIOImageBase
  ITKIONRRD
)
else()
find_package( ITK COMPONENTS
  ITKBinaryMathematicalMorphology
  ITKCommon
  ITKDistanceMap
  ITKImageFilterBase
  ITKImageFunction
  ITKImageFusion
  ITKImageGrid
  ITKImageIntensity
  ITKLabelMap
  ITKMathematicalMorphology
  ITKMetricsv4
  ITKOptimizersv4
  ITKRegistrationMethodsv4
  ITKSmoothing
  ITKSpatialObjects
  ITKThresholding
  ITKTransform
  ITKIOImageBase
  ITKIONRRD
)
endif()

include( ${ITK_USE_FILE} )

find_package( Qt5 COMPONENTS Core Widgets REQUIRED )

#Common setup
include_directories( ${CMAKE_SOURCE_DIR} )
include_directories( ${UltrasoundOpticNerveUI_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions( -DNOMINMAX )


#Optic nerve ui executable

qt5_wrap_ui( OpticNerveUI_H OpticNerve.ui )
qt5_wrap_cpp( optic_source_files_moc OpticNerveUI.h )

set( optic_source_files
  OpticNerveUIDriver.cxx
  OpticNerveUI.cxx
  OpticNerveEstimator.cxx
)

add_executable( OpticNerveUI
  ${optic_source_files}
  ${OpticNerveUI_H}
  ${optic_source_files_moc}
)

target_link_libraries( OpticNerveUI PUBLIC
  ${ITK_LIBRARIES}
  ${IntersonArraySDKCxx_LIBRARIES}
  Qt5::Widgets
)

install( TARGETS OpticNerveUI
  RUNTIME DESTINATION bin COMPONENT Runtime
  LIBRARY DESTINATION bin COMPONENT Runtime
  ARCHIVE DESTINATION lib COMPONENT Development
)

#Spectroscopy ui executable
if( ${Build_Spectroscopy} )

qt5_wrap_ui( SpectroscopyUI_H Spectroscopy.ui )
qt5_wrap_cpp( spectroscopy_source_files_moc SpectroscopyUI.h )

set( spectroscopy_source_files
  SpectroscopyUIDriver.cxx
  SpectroscopyUI.cxx
)

add_executable( SpectroscopyUI
  ${spectroscopy_source_files}
  ${SpectroscopyUI_H}
  ${spectroscopy_source_files_moc}
)

target_link_libraries( SpectroscopyUI PUBLIC
  ${ITK_LIBRARIES}
  ${IntersonArraySDKCxx_LIBRARIES}
  Qt5::Widgets
)

install( TARGETS SpectroscopyUI
  RUNTIME DESTINATION bin COMPONENT Runtime
  LIBRARY DESTINATION bin COMPONENT Runtime
  ARCHIVE DESTINATION lib COMPONENT Development
)
endif()


#CPack setup

include( InstallRequiredSystemLibraries )

set( CPACK_MONOLITHIC_INSTALL ON )
set( CPACK_PACKAGE_VENDOR "Kitware, Inc." )
set( CPACK_GENERATOR "ZIP" )
set( CPACK_PACKAGE_VERSION "0.1.4" )
set( CPACK_PACKAGE_VERSION_MAJOR "0" )
set( CPACK_PACKAGE_VERSION_MINOR "1" )
set( CPACK_PACKAGE_VERSION_PATCH "4" )

set( CPACK_INSTALL_CMAKE_PROJECTS "${IntersonArraySDKCxx_DIR};IntersonArraySDKCxx;Runtime;/" )
set( CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${PROJECT_NAME};ALL;/" )

# Get location of windeployqt.exe based on uic.exe location
get_target_property( uic_location Qt5::uic IMPORTED_LOCATION )
get_filename_component( _dir ${uic_location} DIRECTORY )
set( windeployqt "${_dir}/windeployqt.exe" )
if( NOT EXISTS ${windeployqt} )
  message( FATAL_ERROR "Failed to locate windeployqt executable: [${windeployqt}]" )
endif()
install( CODE "execute_process(COMMAND \"${windeployqt}\" \"\${CMAKE_INSTALL_PREFIX}/bin/OpticNerveUI.exe\")" )

include( CPack )
