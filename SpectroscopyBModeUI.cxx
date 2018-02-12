/*=========================================================================

Library:   UltrasoundSpectroscopyBModeRecorder

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

#include "SpectroscopyBModeUI.h"

#include "imageIO.h"
#include "ITKQtHelpers.hxx"
#include "ITKFilterFunctions.h"

#include <QDir>
#include <QFileDialog>

#include <ctime>
#include <sstream>
#include <iomanip>

#define DEBUG_PRINT


void SpectroscopyBModeUI::closeEvent( QCloseEvent *event )
{

#ifdef DEBUG_PRINT
  std::cout << "Close event called" << std::endl;
#endif
  intersonDevice.Stop();
}

SpectroscopyBModeUI::SpectroscopyBModeUI( QWidget *parent )
  : QMainWindow( parent ),
    ui( new Ui::MainWindow ),
    lastIndexRendered( -1 )
{
  recordRF = true;

  //Setup the graphical layout on this current Widget
  ui->setupUi( this );

  this->setWindowTitle( "Ultrasound Optic Nerve Estimation" );

  //Links buttons and actions
  connect( ui->pushButton_ConnectProbe,
    SIGNAL( clicked() ), this, SLOT( ConnectProbe() ) );
  connect( ui->dropDown_Frequency,
    SIGNAL( currentIndexChanged( int ) ), this, SLOT( SetFrequency() ) );

  ui->comboBox_outputDir->addItem( QDir::currentPath() );
  connect( ui->pushButton_outputDir,
    SIGNAL( clicked() ), this, SLOT( BrowseOutputDirectory() ) );
  connect( ui->pushButton_recordRF,
    SIGNAL( clicked() ), this, SLOT( RecordBMode() ) );

  // Timer
  this->timer = new QTimer( this );
  this->timer->setSingleShot( true );
  this->timer->setInterval( 50 );
  this->connect( timer, SIGNAL( timeout() ), SLOT( UpdateImage() ) );
}

SpectroscopyBModeUI::~SpectroscopyBModeUI()
{
  //this->intersonDevice.Stop();
  delete ui;
}

void SpectroscopyBModeUI::ConnectProbe()
{
#ifdef DEBUG_PRINT
  std::cout << "Connect probe called" << std::endl;
#endif
  if( !intersonDevice.ConnectProbe( recordRF ) )
    {
#ifdef DEBUG_PRINT
    std::cout << "Connect probe failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
    }

  for( int i = 0; i < freqCheckBoxes.size(); i++ )
    {
    delete freqCheckBoxes[ i ];
    }
  IntersonArrayDeviceRF::FrequenciesType fs =
    intersonDevice.GetFrequencies();
  ui->dropDown_Frequency->clear();
  freqCheckBoxes.resize( fs.size() );
  for( int i = 0; i < fs.size(); i++ )
    {
    std::ostringstream ftext;
    ftext << std::setprecision( 2 ) << std::setw( 2 ) << std::fixed;
    ftext << fs[ i ] / 1000000.0 << " Mhz";
    ui->dropDown_Frequency->addItem( ftext.str().c_str() );
    //Add check boxes to UI for recording
    freqCheckBoxes[ i ] = new QCheckBox();
    freqCheckBoxes[ i ]->setText( ftext.str().c_str() );
    freqCheckBoxes[ i ]->setChecked( true );
    ui->layout_Frequency->addWidget( freqCheckBoxes[ i ], 1);
    }
  ui->dropDown_Frequency->setCurrentIndex(0);

  if( !intersonDevice.Start() )
    {
    std::cout << "Starting scan failed" << std::endl;
    return;
    }

  this->timer->start();
}

void SpectroscopyBModeUI::UpdateImage()
{
  int currentIndex = -1;
  if( recordRF )
    {
    currentIndex = intersonDevice.GetCurrentRFIndex();
    }
  else
    {
    currentIndex = intersonDevice.GetCurrentBModeIndex();
    }
  if( currentIndex >= 0 && currentIndex != lastIndexRendered )
    {
    lastIndexRendered = currentIndex;

    QImage image1;
    if( recordRF )
      {
      RFImageType::Pointer image =
        intersonDevice.GetRFImage( currentIndex );
      ITKFilterFunctions< RFImageType >::PermuteArray order;
      order[ 0 ] = 1;
      order[ 1 ] = 0;
      image = ITKFilterFunctions< RFImageType >::PermuteImage( image,
        order );
      image = ITKFilterFunctions< RFImageType >::Rescale( image, 0, 255 );
      image1 = ITKQtHelpers::GetQImageColor<RFImageType>(
        image,
        image->GetLargestPossibleRegion(),
        QImage::Format_RGB16
        );
      }
    else
      {
      BModeImageType::Pointer image =
        intersonDevice.GetBModeImage( currentIndex );
      ITKFilterFunctions< BModeImageType >::PermuteArray order;
      order[ 0 ] = 1;
      order[ 1 ] = 0;
      image = ITKFilterFunctions< BModeImageType >::PermuteImage( image,
        order );
      image = ITKFilterFunctions< BModeImageType >::Rescale( image, 0, 255 );
      image1 = ITKQtHelpers::GetQImageColor<BModeImageType>(
        image,
        image->GetLargestPossibleRegion(),
        QImage::Format_RGB16
        );
      }

    ui->label_BModeImage->setPixmap( QPixmap::fromImage( image1 ) );
    ui->label_BModeImage->setScaledContents( true );
    ui->label_BModeImage->setSizePolicy( QSizePolicy::Ignored,
      QSizePolicy::Ignored );
    }

  this->timer->start();
}

void SpectroscopyBModeUI::SetFrequency()
{
  if( this->ui->dropDown_Frequency->currentIndex() >= 0)
    {
    bool success = this->intersonDevice.SetFrequency(
      this->ui->dropDown_Frequency->currentIndex() );
    IntersonArrayDeviceRF::FrequenciesType frequencies =
      intersonDevice.GetFrequencies();
    if( !success )
      {
      std::cout << "Failed to set frequency index: "
        << this->ui->dropDown_Frequency->currentIndex() << std::endl;
      std::cout << "Current frequency: "
        << frequencies[ intersonDevice.GetFrequency() ] << std::endl;
      std::cout << "Current frequency index: "
        << ( int )intersonDevice.GetFrequency() << std::endl;
      }
    }
}

void SpectroscopyBModeUI::BrowseOutputDirectory()
{
  QString outputFolder = QFileDialog::getExistingDirectory( this,
    "Choose directory to save the captured images", "C://" );
  if( outputFolder.isEmpty() || outputFolder.isNull() )
    {
    std::cerr << "Folder not valid." << std::endl;
    return;
    }
  ui->comboBox_outputDir->insertItem( 0, outputFolder );
  ui->comboBox_outputDir->setCurrentIndex( 0 );
}

void SpectroscopyBModeUI::RecordBMode()
{
  QSignalBlocker myTimer( this->timer );
  std::cout << "Recording Started" << std::endl;

  IntersonArrayDeviceRF::FrequenciesType frequencies =
    intersonDevice.GetFrequencies();
  unsigned char voltLow = ui->spinBox_voltLow->value();
  unsigned char voltHigh = ui->spinBox_voltHigh->value();
  unsigned char voltStep = ui->spinBox_voltStep->value();
  unsigned char volt = intersonDevice.GetVoltage();
  unsigned char fi = intersonDevice.GetFrequency();
  std::vector< IntersonArrayDeviceRF::RFImageType::Pointer > rfImages;
  std::vector< IntersonArrayDeviceRF::ImageType::Pointer > bmImages;
  std::vector< std::string > imageNames;

  /**
   * Changing the frequencies is more time consuming. Do it in the outer
   *  loop.
   */
  int numberOfSamples = ui->spinBox_numberOfSamples->value();
  int numVol = (int)((voltHigh-voltLow) / voltStep)+1;
  int numberOfImages = freqCheckBoxes.size() * numVol;
  for( unsigned char i = 0; i < freqCheckBoxes.size(); i++ )
    {
    if( !freqCheckBoxes[ i ]->isChecked() )
      {
      continue;
      }
    for( int v = voltLow; v <= voltHigh; v += voltStep )
      {
      std::cout << "Image " << i * numVol + (int)((v-voltLow)/voltStep)
        << " of " << numberOfImages << std::endl;
      if( v == voltLow )
        {
        std::cout << " Frequency Index = " << (int)i << std::endl;
        std::cout << " Voltage = " << v << std::endl;
        if( !intersonDevice.SetFrequencyAndVoltage( i, v ) )
          {
          std::cout << "Failed to set frequency: " << frequencies[ i ]
            << " and voltage " << v << std::endl;
          continue;
          }
        }
      else
        {
        std::cout << " Voltage = " << v << std::endl;
        if( !intersonDevice.SetVoltage( v ) )
          {
          std::cout << "Failed to set voltage: " << v << std::endl;
          continue;
          }
        }
      if( recordRF )
        {
        int lastIndex = intersonDevice.GetCurrentRFIndex();
        int index = intersonDevice.GetCurrentRFIndex();
        std::cout << " Waiting for probe to reset." << std::endl;
        while( index == lastIndex )
          {
          Sleep( 10 );
          index = intersonDevice.GetCurrentRFIndex();
          }
        }
      else
        {
        int lastIndex = intersonDevice.GetCurrentBModeIndex();
        int index = intersonDevice.GetCurrentBModeIndex();
        std::cout << " Waiting for probe to reset." << std::endl;
        while( index == lastIndex )
          {
          Sleep( 10 );
          index = intersonDevice.GetCurrentBModeIndex();
          }
        }
      for( int sample=0; sample<numberOfSamples; ++sample)
        {
        if( recordRF )
          {
          int lastIndex = intersonDevice.GetCurrentRFIndex();
          int index = intersonDevice.GetCurrentRFIndex();
          rfImages.push_back( intersonDevice.GetRFImage( index ));
          std::cout << " Waiting for probe to reset." << std::endl;
          if( sample+1 < numberOfSamples )
            {
            while( index == lastIndex )
              {
              Sleep( 10 );
              index = intersonDevice.GetCurrentRFIndex();
              }
            }
          }
        else
          {
          int lastIndex = intersonDevice.GetCurrentBModeIndex();
          int index = intersonDevice.GetCurrentBModeIndex();
          bmImages.push_back( intersonDevice.GetBModeImage( index ));
          std::cout << " Waiting for probe to reset." << std::endl;
          if( sample+1 < numberOfSamples )
            {
            while( index == lastIndex )
              {
              Sleep( 10 );
              index = intersonDevice.GetCurrentBModeIndex();
              }
            }
          }
        }
      std::ostringstream ftext;
      ftext << std::setw( 3 ) << std::fixed << std::setfill( '0' );
      ftext << "voltage_" << v;
      ftext << "_freq_" << std::setw( 10 ) << std::fixed << frequencies[i];
      ftext << ".nrrd";
      imageNames.push_back( ftext.str() );
      }
    }
  if( numberOfSamples > 1 )
    {
    if( recordRF )
      {
      typedef itk::ImageRegionIterator< RFImageType > RFImageIteratorType;
      for( int i=0; i < numberOfImages; ++i )
        {
        RFImageIteratorType rfIterTo( rfImages[ i * numberOfSamples ], 
          rfImages[ i * numberOfSamples ]->GetLargestPossibleRegion() );
        for( int j = 1; j<numberOfSamples; ++j )
          {
          rfIterTo.GoToBegin();
          RFImageIteratorType rfIterFrom( rfImages[ i * numberOfSamples + j ], 
            rfImages[ i * numberOfSamples + j ]->GetLargestPossibleRegion() );
          while( !rfIterTo.IsAtEnd() )
            {
            rfIterTo.Set( (double)(rfIterFrom.Get() * j + rfIterTo.Get())
              / (j+1) );
            ++rfIterFrom;
            ++rfIterTo;
            }
          }
        }
      }
    else
      {
      typedef itk::ImageRegionIterator< BModeImageType > BMImageIteratorType;
      for( int i=0; i < numberOfImages; ++i )
        {
        BMImageIteratorType bmIterTo( bmImages[ i * numberOfSamples ], 
          bmImages[ i * numberOfSamples ]->GetLargestPossibleRegion() );
        for( int j = 1; j<numberOfSamples; ++j )
          {
          bmIterTo.GoToBegin();
          BMImageIteratorType bmIterFrom( bmImages[ i * numberOfSamples + j ], 
            bmImages[ i * numberOfSamples + j ]->GetLargestPossibleRegion() );
          while( !bmIterTo.IsAtEnd() )
            {
            bmIterTo.Set( (double)(bmIterFrom.Get() * j + bmIterTo.Get())
              / (j+1) );
            ++bmIterFrom;
            ++bmIterTo;
            }
          }
        }
      }
    }

  /**
   * Create a sub directory for storing this specific collection of data.
   **/
  time_t subDirNow = time( 0 );
  tm *subDirLocalTime = localtime( &subDirNow );
  std::ostringstream subDirName;
  subDirName << std::setw( 4 ) << std::setfill( '0' ) << std::fixed;
  subDirName << std::to_string( 1900 + subDirLocalTime->tm_year ) << "-"
    << std::setw( 2 ) << std::setfill( '0' )
    << std::to_string( 1 + subDirLocalTime->tm_mon ) << "-"
    << std::to_string( subDirLocalTime->tm_mday ) << "_"
    << std::to_string( 1 + subDirLocalTime->tm_hour ) << "-"
    << std::to_string( 1 + subDirLocalTime->tm_min ) << "-"
    << std::to_string( 1 + subDirLocalTime->tm_sec );
  std::string output_directory =
    ui->comboBox_outputDir->currentText().toStdString() + "/"
    + subDirName.str();
  std::string sysMkDir = std::string( "mkdir \"" + output_directory
    + "\"" );
  std::cout << "Saving to directory " << std::endl << "   " << sysMkDir
    << std::endl;
  system( sysMkDir.c_str() );
  
  //Save Images
  for( int i = 0; i < numberOfImages; i++ )
    {
    if( recordRF )
      {
      std::string rfFilename = "rf_" + imageNames[i];
      ImageIO<IntersonArrayDeviceRF::RFImageType>::saveImage(
        rfImages[ i*numberOfSamples ], output_directory + "/" + rfFilename );
      }
    else
      {
      std::string bmFilename = "bm_" + imageNames[i];
      ImageIO<IntersonArrayDeviceRF::ImageType>::saveImage(
        bmImages[ i*numberOfSamples ], output_directory + "/" + bmFilename );
      }
    }

  recordRF = false;
  //reset probe
  intersonDevice.SetFrequencyAndVoltage( fi, volt );

  std::cout << "Recording Stopped" << std::endl;
}
