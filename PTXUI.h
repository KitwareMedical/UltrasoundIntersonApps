/*=========================================================================

Library:   UltrasoundPTXRecorder

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/

#pragma once

#ifndef PTXUI_U 
#define PTXUI_U 

// Print debug statements
// Using print because I am to inept to setup visual studio
//#define DEBUG_PRINT

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QCheckBox>
#include <QLabel>
#include <qgroupbox.h>
#include <QCloseEvent>

//#include "ui_PTX.h"
#include "PTXUILayout.h"
#include "IntersonArrayDeviceRF.hxx"

#include "itkBModeImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkButterworthBandpass1DFilterFunction.h"

#include "itkForward1DFFTImageFilter.h"
#include "itkInverse1DFFTImageFilter.h"
#include "itkAbsImageFilter.h"
#include "itkAddImageFilter.h"
#include "itkMultiplyImageFIlter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkLog10ImageFilter.h"

#include <atomic>

#include "PTXPatientData.h"

#include "PTXDetector.hxx"

//Forward declaration of Ui::MainWindow;
//namespace Ui
//{
//  class MainWindow;
//}

  //Declaration of OpticNerveUI
class PTXUI : public QMainWindow
{
  Q_OBJECT

public:
  PTXUI( int bufferSize, QWidget *parent = nullptr, bool runBMode = false );
  ~PTXUI();

protected:
  void  closeEvent( QCloseEvent * event );

  protected slots:

  void ToggleProbe();
  void Save();

  void SetFrequency();
  void SetDepth();
  void SetPower();

  /** Update the images displayed from the probe */
  void UpdateImage();

  void UpdateFrameRate();


private:
  /** Layout for the Window */
  //Ui::MainWindow *ui;
  PTXUILayout *ui;
  QTimer *timer;
  QTimer *processing;
  std::vector< QCheckBox * > freqCheckBoxes;
 
  PTXPatientData patientData;

  IntersonArrayDeviceRF intersonDevice;
  int lastRenderedIndex;
  int lastMModeIndex;
  bool runInBMode;

  int mMode1Location; 
  int mMode2Location; 
  int mMode3Location; 

  std::atomic<int> nFramesRendered;
  std::atomic<int> nSkippedFrames;

  typedef IntersonArrayDeviceRF::RFImageType  RFImageType;
  typedef IntersonArrayDeviceRF::RFImageType3d  RFImageType3d;
  typedef IntersonArrayDeviceRF::ImageType  BModeImageType;

  typedef itk::Image<double, 2> ImageType;
  ImageType::Pointer mMode1;
  ImageType::Pointer mMode2;
  ImageType::Pointer mMode3;
  
  typedef PTXDetector<double> PTXDetectorType; 
  typedef PTXDetectorType::RGBImageType RGBImageType;

  //BMode filtering
  typedef itk::CastImageFilter<RFImageType, ImageType> CastFilter;
  CastFilter::Pointer m_CastFilter;
  
  typedef itk::CastImageFilter<BModeImageType, ImageType> BModeCastFilter;
  BModeCastFilter::Pointer m_BModeCastFilter;
  
  typedef itk::BModeImageFilter< ImageType >  BModeImageFilter;
  BModeImageFilter::Pointer m_BModeFilter;

  typedef itk::ButterworthBandpass1DFilterFunction ButterworthBandpassFilter;
  ButterworthBandpassFilter::Pointer m_BandpassFilter;

  typedef itk::ButterworthBandpass1DFilterFunction ButterworthBandpassFilterRF;
  ButterworthBandpassFilterRF::Pointer m_BandpassFilterRF;

  //RF filtering
  typedef itk::CastImageFilter<RFImageType, ImageType> CastFilterRF;
  CastFilterRF::Pointer m_CastFilterRF;

  typedef itk::Forward1DFFTImageFilter<ImageType> Forward1DFFTFilter;
  Forward1DFFTFilter::Pointer m_ForwardFFT;

  typedef itk::FrequencyDomain1DImageFilter<Forward1DFFTFilter::OutputImageType >
    FrequencyFilter;
  FrequencyFilter::Pointer m_FrequencyFilter;

  typedef itk::Inverse1DFFTImageFilter<Forward1DFFTFilter::OutputImageType, ImageType>
    Inverse1DFFTFilter;
  Inverse1DFFTFilter::Pointer m_InverseFFT;

  typedef itk::BinaryThresholdImageFilter< ImageType, ImageType >  ThresholdFilter;
  ThresholdFilter::Pointer m_ThresholdFilter;

  typedef itk::AbsImageFilter<ImageType, ImageType> AbsFilter;
  AbsFilter::Pointer m_AbsFilter;

  typedef itk::AddImageFilter<ImageType> AddFilter;
  AddFilter::Pointer m_AddFilter;

  typedef itk::Log10ImageFilter< ImageType, ImageType > LogFilter;
  LogFilter::Pointer m_LogFilter;

  typedef itk::MultiplyImageFilter<ImageType> MultiplyFilter;
  MultiplyFilter::Pointer m_MultiplyFilter;



  ImageType::Pointer CreateMModeImage(int width, int height);
  
  void ShiftImage( ImageType::Pointer mMode ); 

  void FillLine( ImageType::Pointer bMode, ImageType::Pointer mMode, 
                 int bModeIndex, int mModeIndex, QLabel *label);

  void MarkMMode(QImage &image, int location);


  void ConnectProbe();
  void StartScan();
  void StopScan();
};

#endif
