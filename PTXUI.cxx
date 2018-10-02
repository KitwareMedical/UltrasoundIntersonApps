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

// Interson Array probe
//const double IntersonFrequencies[] = { 5.0, 7.5, 10.0 };
// Interson probe
//const double frequencies[] = { 2.5, 3.5, 5.0 };

#include "PTXUI.h"
#include "PTXUISaveDialog.h"

#include "ImageIO.h"
#include "ITKQtHelpers.hxx"
#include "ITKFilterFunctions.h"

#include <QDir>
#include <QDateTime>

#include <ctime>
#include <sstream>
#include <iomanip>



#define DEBUG_PRINT


void PTXUI::closeEvent( QCloseEvent *event )
{

#ifdef DEBUG_PRINT
  std::cout << "Close event called" << std::endl;
#endif
  intersonDevice.Stop();
}

PTXUI::PTXUI( int bufferSize, QWidget *parent, bool runBMode)
  : QMainWindow( parent ), ui( new PTXUILayout() ), runInBMode(runBMode),
  lastRenderedIndex( -1 ),
  lastMModeIndex( -1 ),
  mMode1Location( 32 ),
  mMode2Location( 64 ),
  mMode3Location( 96 )
{

  nFramesRendered = 0;
  nSkippedFrames = 0;
 
  patientData.filepath = QDir::currentPath().toStdString();

  //Setup the graphical layout on this current Widget
  try
    {
    ui->setupUi( this );
    }
  catch( std::exception &e )
    {
    std::cout << e.what() << std::endl;
    }
  catch( ... )
    {
    std::cout << "something failed" << std::endl;
    }

  this->setWindowTitle( "Pneumothorax Scanner" );

  //Links buttons and actions
  connect( ui->pushButton_ConnectProbe,
    SIGNAL( clicked() ), this, SLOT( ToggleProbe() ) );
  connect( ui->spinBox_Depth,
    SIGNAL( valueChanged( int ) ), this, SLOT( SetDepth() ) );
  connect( ui->spinBox_Power,
    SIGNAL( valueChanged( int ) ), this, SLOT( SetPower() ) );
  connect( ui->dropDown_Frequency,
    SIGNAL( currentIndexChanged( int ) ), this, SLOT( SetFrequency() ) );

  connect( ui->pushButton_Save,
    SIGNAL( clicked() ), this, SLOT( Save() ) );
  
   intersonDevice.SetRingBufferSize( bufferSize );

  m_BModeCastFilter = BModeCastFilter::New();

  //Setup Bmode filtering
  m_CastFilter = CastFilter::New();
  m_BModeFilter = BModeImageFilter::New();

  //add frequency filter
  m_BandpassFilter = ButterworthBandpassFilter::New();
  m_BandpassFilter->SetLowerFrequency( 0.2 );
  m_BandpassFilter->SetUpperFrequency( 0.8 );
  m_BandpassFilter->SetOrder( 3 );

  typedef BModeImageFilter::FrequencyFilterType  FrequencyFilterType;
  FrequencyFilterType::Pointer freqFilter = FrequencyFilterType::New();
  freqFilter->SetFilterFunction( m_BandpassFilter );

  m_BModeFilter->SetFrequencyFilter( freqFilter );
  m_BModeFilter->SetInput( m_CastFilter->GetOutput() );

  this->processing = new QTimer( this );
  this->processing->setSingleShot( false );
  this->processing->setInterval( 1000 );
  this->connect( processing, SIGNAL( timeout() ), SLOT( UpdateFrameRate() ) );
  

  // Timer
  this->timer = new QTimer( this );
  this->timer->setSingleShot( true );
  this->timer->setInterval( 50 );
  this->connect( timer, SIGNAL( timeout() ), SLOT( UpdateImage() ) );
}

PTXUI::~PTXUI()
{
  //this->intersonDevice.Stop();
  delete ui;
}

void PTXUI::ToggleProbe()
{
  if( ui->pushButton_ConnectProbe->isChecked() )
  {
    StartScan();
    ui->pushButton_ConnectProbe->setText( "Stop Scan" );
  }
  else
  {
    StopScan();
    ui->pushButton_ConnectProbe->setText( "Start Scan" );
  }
}

void PTXUI::StartScan()
{
  if( !intersonDevice.Start() )
  {
  intersonDevice.Stop();
  ConnectProbe();
  }
}

void PTXUI::StopScan()
{
  intersonDevice.Stop();
}

void PTXUI::ConnectProbe()
{
  //if( intersonDevice.IsProbeConnected() ){
  //  return; 
  //}
#ifdef DEBUG_PRINT
  std::cout << "Connect probe called" << std::endl;
#endif
  if( !intersonDevice.ConnectProbe(  !runInBMode ) )
    {
#ifdef DEBUG_PRINT
    std::cout << "Connect probe failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
    }

  
  int height = 0;
  int width = intersonDevice.GetRingBufferSize();
  if( runInBMode )
    {
    height = intersonDevice.GetBModeDepthResolution();
    }
  else
    {
    height = intersonDevice.GetRFModeDepthResolution();
    }
  mMode1 = CreateMModeImage(width, height);
  mMode2 = CreateMModeImage(width, height);
  mMode3 = CreateMModeImage(width, height);

  IntersonArrayDeviceRF::FrequenciesType fs = intersonDevice.GetFrequencies();
  ui->dropDown_Frequency->clear();
  for( unsigned int i = 0; i < fs.size(); i++ )
    {
    std::ostringstream ftext;
    ftext << std::setprecision( 2 ) << std::setw( 2 ) << std::fixed;
    ftext << fs[ i ] / 1000000.0 << " Mhz";
    ui->dropDown_Frequency->addItem( ftext.str().c_str() );
    //Add check boxes to UI for recording
    }
  
  intersonDevice.Start();
  this->timer->start();
  this->processing->start();
}



void PTXUI::UpdateImage()
{
  int currentIndex;
  if(runInBMode)
    {
    currentIndex = intersonDevice.GetCurrentBModeIndex();
    }
  else
    {
    currentIndex = intersonDevice.GetCurrentRFIndex();
    }
  if( currentIndex >= 0 && currentIndex != lastRenderedIndex )
    {
    
    nSkippedFrames = (currentIndex - lastRenderedIndex) - 1;
    if(nSkippedFrames < 0 ){
      nSkippedFrames += (intersonDevice.GetRingBufferSize() - 1);
    }
    lastRenderedIndex = currentIndex;
    
    ImageType::Pointer bmode; 
    if(!runInBMode)
      {
      RFImageType::Pointer rf = intersonDevice.GetRFImage( currentIndex );

      //Create BMode image
      m_CastFilter->SetInput( rf );
      m_BModeFilter->Update();
      bmode = m_BModeFilter->GetOutput();
      }
    else
      {
      BModeImageType::Pointer probeBMode = intersonDevice.GetBModeImage( currentIndex );
      m_BModeCastFilter->SetInput( probeBMode);
      m_BModeCastFilter->Update();
      bmode = m_BModeCastFilter->GetOutput();
      }
    ITKFilterFunctions< ImageType >::PermuteArray order;
    order[ 0 ] = 1;
    order[ 1 ] = 0;
    bmode = ITKFilterFunctions< ImageType>::PermuteImage( bmode, order );
    bmode = ITKFilterFunctions< ImageType >::Rescale( bmode, 0, 255 );
    ImageType::SizeType bmodeSize = bmode->GetLargestPossibleRegion().GetSize();

    QImage image1 = ITKQtHelpers::GetQImageColor<ImageType>(
      bmode,
      bmode->GetLargestPossibleRegion(),
      QImage::Format_RGB16
      );
    MarkMMode(image1, mMode1Location); 
    MarkMMode(image1, mMode2Location); 
    MarkMMode(image1, mMode3Location); 

    //if( !runInBMode )
      {
      double scale = intersonDevice.GetMmPerPixel();
      double rfsamples = intersonDevice.GetRFModeDepthResolution();
      int lines = intersonDevice.GetNumberOfLines();
      double scaleX = 38.0 / (lines - 1.0) * bmodeSize[0] ;
      double scaleY = 1540.0 / ( 2.0 * 30000.0 ) * bmodeSize[1];
    //double height = ui->centralwidget->height() * ( 1.0 - ui->verticalSlider_Scale->value() / 100.0 ) ;
      double height = ui->splitter->sizes().at(0) - 50;
      image1 = image1.scaled( height / scaleY * scaleX, height ) ;
      }

    int w = ui->label_BModeImage->width();
    int h = ui->label_BModeImage->height();
    ui->label_BModeImage->setPixmap( QPixmap::fromImage( image1 ) );
    //ui->label_BModeImage->resize( height * scaleX + 5, height + 5);
    //ui->label_BModeImage->setScaledContents( true );
    //ui->label_BModeImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );

    int width = mMode1->GetLargestPossibleRegion().GetSize()[0];
    //Update M-Mode Images
    if( lastMModeIndex < width - 1 )
      {
      lastMModeIndex++;
      }
    else
      {
      ShiftImage( mMode1 );
      ShiftImage( mMode2 );
      ShiftImage( mMode3 );
      }
    FillLine( bmode, mMode1, mMode1Location, lastMModeIndex, this->ui->label_MMode1 );
    FillLine( bmode, mMode2, mMode2Location, lastMModeIndex, this->ui->label_MMode2 );
    FillLine( bmode, mMode3, mMode3Location, lastMModeIndex, this->ui->label_MMode3 );

    nFramesRendered++;
    }

  this->timer->start();
}

void PTXUI::SetFrequency()
{
  bool success = this->intersonDevice.SetFrequency(
    this->ui->dropDown_Frequency->currentIndex() );
  IntersonArrayDeviceRF::FrequenciesType frequencies = intersonDevice.GetFrequencies();
  if( !success )
    {
    std::cout << "Failed to set frequency index: " << this->ui->dropDown_Frequency->currentIndex() << std::endl;
    std::cout << "Current frequency: " << frequencies[ intersonDevice.GetFrequency() ] << std::endl;
    std::cout << "Current frequency index: " << ( int )intersonDevice.GetFrequency() << std::endl;
    }
}

void PTXUI::SetDepth()
{
  int depth = this->intersonDevice.SetDepth(
    this->ui->spinBox_Depth->value() );
  this->ui->spinBox_Depth->setValue( depth );
}

void PTXUI::SetPower()
{
  this->intersonDevice.SetVoltage(
    this->ui->spinBox_Power->value() );
}





void PTXUI::Save()
{
  if( !intersonDevice.IsProbeConnected() )
    {
    return;
    }
   StopScan(); 

   PTXUISaveDialog dialog(patientData, this);
   int ret = dialog.exec();
   if( ret > 0)
     {
     //Save data
     RFImageType3d::Pointer rf = intersonDevice.GetRingBufferRFOrdered();
     std::stringstream filename;
     filename << patientData.filepath  << "/";  
     filename << patientData.id        << "_";  
     filename << std::setw(3) << std::setfill('0');
     filename << patientData.session   << "_";  
     filename << std::setw(3) << std::setfill('0');
     filename << patientData.age       << "_";  
     filename << patientData.diagnosis << "_";
     QString format = QString::fromStdString( "yy-mm-dd-hh-mm-ss");
     filename << QDateTime::currentDateTime().toString( format ).toStdString();
     filename << ".nrrd"; 
     std::cout << filename.str() << std::endl; 
     ImageIO<RFImageType3d>::WriteImage( rf, filename.str() );
     
     if( ret == 2 )
       {
       patientData = PTXPatientData();
       }
     else
       {
       patientData.session += 1;
       }
     }

   StartScan();   
}


PTXUI::ImageType::Pointer PTXUI::CreateMModeImage(int width, int height)
  {
  ImageType::Pointer image = ImageType::New();

  ImageType::IndexType imageIndex;
  imageIndex.Fill( 0 );

  ImageType::SizeType imageSize;
  imageSize[ 0 ] = width;
  imageSize[ 1 ] = height;

  ImageType::RegionType imageRegion;
  imageRegion.SetIndex( imageIndex );
  imageRegion.SetSize( imageSize );

  image->SetRegions( imageRegion );
  image->Allocate();
  image->FillBuffer(0);

  return image;
  }

void PTXUI::ShiftImage(ImageType::Pointer mMode){
    int height = mMode->GetLargestPossibleRegion().GetSize()[1];
    int width  = mMode->GetLargestPossibleRegion().GetSize()[0];
    ImageType::IndexType mIndex1;
    ImageType::IndexType mIndex2;
    for(int j=1; j<width; j++)
      {
      mIndex1[0] = j-1;
      mIndex2[0] = j;
      for(int i=0; i<height; i++)
        {
        mIndex1[1] = i;
        mIndex2[1] = i;
        mMode->SetPixel(mIndex1, mMode->GetPixel( mIndex2 ) );
        }
      }

}



void PTXUI::FillLine( ImageType::Pointer bMode, ImageType::Pointer mMode, 
                      int bModeIndex, int mModeIndex, QLabel *label)
  {
    int height = mMode->GetLargestPossibleRegion().GetSize()[1];
    int width = mMode->GetLargestPossibleRegion().GetSize()[0];
    ImageType::IndexType mIndex;
    mIndex[0] = mModeIndex;
    ImageType::IndexType bIndex;
    bIndex[0] = bModeIndex;
    for(int i=0; i<height; i++)
      {
      mIndex[1] = i;
      bIndex[1] = i;
      mMode->SetPixel(mIndex, bMode->GetPixel(bIndex) );
      }
  
    QImage image;
    if( ui->checkbox_Overlay->checkState() == Qt::Checked ) 
      {
      RGBImageType::Pointer overlay =  PTXDetectorType::DetectMMode(mMode);
        
      image= ITKQtHelpers::GetQImageColor_Vector<RGBImageType>(
         overlay,
         overlay->GetLargestPossibleRegion(),
        QImage::Format_RGB16
        );
      }
    else{
      image= ITKQtHelpers::GetQImageColor<ImageType>(
        mMode,
        mMode->GetLargestPossibleRegion(),
        QImage::Format_RGB16
        );
    }
    
    label->setPixmap( QPixmap::fromImage( image ) );
    label->setScaledContents( true );
    label->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  }

void PTXUI::MarkMMode(QImage &image, int location){
  
  for(int i=0; i<image.height(); i++)
    {
    QColor col = image.pixelColor(location, i);
    col.setRedF( col.redF() * 0.5 );
    col.setGreenF( col.greenF() * 0.5 + 0.3 );
    col.setBlueF( col.blueF() * 0.5 );
    image.setPixelColor(location, i, col);
    }
}


void PTXUI::UpdateFrameRate()
{
  std::ostringstream processingRate;
  processingRate << std::setprecision( 1 ) << std::setw( 3 ) << std::fixed;
  processingRate <<  nFramesRendered  << " frames / sec";
  std::ostringstream skippedRate;
  skippedRate << std::setprecision( 1 ) << std::setw( 3 ) << std::fixed;
  skippedRate <<  nSkippedFrames << " frames / sec";
  std::cout << processingRate.str() << " | " << skippedRate.str() << std::endl;
  nFramesRendered = 0;
  nSkippedFrames = 0;
}

