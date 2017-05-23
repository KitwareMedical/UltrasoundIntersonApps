/*=========================================================================

Library:   UltrasoundSpectroscopyRecorder

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

#include "SpectroscopyUI.h"

#include "imageIO.h"
#include "ITKQtHelpers.hxx"
#include "ITKFilterFunctions.h"


#include <sstream>
#include <iomanip>

#define DEBUG_PRINT


void SpectroscopyUI::closeEvent (QCloseEvent *event){

#ifdef DEBUG_PRINT
     std::cout << "Close event called" << std::endl;
#endif
   intersonDevice.Stop();
}


SpectroscopyUI::SpectroscopyUI(int bufferSize, QWidget *parent) 
	: QMainWindow(parent), ui(new Ui::MainWindow), 
          lastBModeRendered(-1), lastRFRendered(-1)
{

	//Setup the graphical layout on this current Widget
	ui->setupUi(this);

	this->setWindowTitle("Ultrasound Optic Nerve Estimation");

	//Links buttons and actions
	connect( ui->pushButton_ConnectProbe, 
                 SIGNAL(clicked()), this, SLOT(ConnectProbe()));
	connect( ui->spinBox_Depth, 
                 SIGNAL(valueChanged(int)), this, SLOT(SetDepth()));
	connect( ui->dropDown_Frequency, 
                 SIGNAL(currentIndexChanged(int)), this, SLOT(SetFrequency()));

	connect( ui->slider_lowerFrequency, 
                 SIGNAL(valueChanged(int)), this, SLOT(SetLowerFrequency()));
	connect( ui->slider_upperFrequency, 
                 SIGNAL(valueChanged(int)), this, SLOT(SetUpperFrequency()));
	connect( ui->spinBox_order, 
                 SIGNAL(valueChanged(int)), this, SLOT(SetOrder()));

        connect( ui->pushButton_recordRF, 
                 SIGNAL( clicked() ), this, SLOT( RecordRF() ) );

        intersonDevice.SetRingBufferSize( bufferSize );

 
        //Setup Bmode filtering 
        m_CastFilter = CastFilter::New();
        m_BModeFilter = BModeImageFilter::New();
        //add frequency filter
        m_BandpassFilter =  ButterworthBandpassFilter::New();

        typedef BModeImageFilter::FrequencyFilterType  FrequencyFilterType;
        FrequencyFilterType::Pointer freqFilter= FrequencyFilterType::New();
        freqFilter->SetFilterFunction( m_BandpassFilter );

        m_BModeFilter->SetFrequencyFilter( freqFilter );
     
        m_BModeFilter->SetInput( m_CastFilter->GetOutput() );

        //Setup RF filtering
        m_CastFilterRF = CastFilterRF::New();

        m_ForwardFFT = Forward1DFFTFilter::New();
        m_ForwardFFT->SetInput( m_CastFilterRF->GetOutput() );

        m_FrequencyFilter = FrequencyFilter::New();
        m_BandpassFilterRF = ButterworthBandpassFilterRF::New(); 
        m_FrequencyFilter->SetFilterFunction( m_BandpassFilterRF );
        m_FrequencyFilter->SetInput( m_ForwardFFT->GetOutput() );

        m_InverseFFT = Inverse1DFFTFilter::New();
        m_InverseFFT->SetInput( m_FrequencyFilter->GetOutput() );
     
        m_ThresholdFilter= ThresholdFilter::New();
        m_ThresholdFilter->SetLowerThreshold( 0 );
        //m_ThresholdFilter->SetUpperThreshold(  );
        m_ThresholdFilter->SetInsideValue( 1 );
        m_ThresholdFilter->SetOutsideValue( -1 );
        m_ThresholdFilter->SetInput( m_InverseFFT->GetOutput() );

        m_AbsFilter = AbsFilter::New();        
        m_AbsFilter->SetInput( m_InverseFFT->GetOutput() );

        m_AddFilter = AddFilter::New();
        m_AddFilter->SetConstant2( 1 );        
        m_AddFilter->SetInput( m_AbsFilter->GetOutput() );
        
        m_LogFilter = LogFilter::New();
        m_LogFilter->SetInput( m_AddFilter->GetOutput() );
        
        m_MultiplyFilter = MultiplyFilter::New();
        m_MultiplyFilter->SetInput1( m_LogFilter->GetOutput() );
        m_MultiplyFilter->SetInput2( m_ThresholdFilter->GetOutput() );
                    

        //Set bandpass filter paramaters
        SetLowerFrequency();
        SetUpperFrequency();
        SetOrder();

	// Timer
	this->timer = new QTimer(this);
	this->timer->setSingleShot(true);
	this->timer->setInterval(50);
	this->connect(timer, SIGNAL(timeout()), SLOT(UpdateImage()));
}

SpectroscopyUI::~SpectroscopyUI()
{
	//this->intersonDevice.Stop();
	delete ui;
}

void SpectroscopyUI::ConnectProbe(){
#ifdef DEBUG_PRINT
  std::cout << "Connect probe called" << std::endl;
#endif
  if ( !intersonDevice.ConnectProbe(true) ){
#ifdef DEBUG_PRINT
    std::cout << "Connect probe failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
  }
 
  for(int i=0; i<freqCheckBoxes.size(); i++){
    delete freqCheckBoxes[i];
  }
  IntersonArrayDeviceRF::FrequenciesType fs = intersonDevice.GetFrequencies();
  ui->dropDown_Frequency->clear();
  freqCheckBoxes.resize( fs.size() ); 
  for(int i=0; i<fs.size(); i++){
     std::ostringstream ftext;   
     ftext << std::setprecision(2) << std::setw(2) << std::fixed;
     ftext << fs[i] / 1000000.0 << " Mhz";
     ui->dropDown_Frequency->addItem( ftext.str().c_str() );
     //Add check boxes to UI for recording
     freqCheckBoxes[i] = new QCheckBox();
     freqCheckBoxes[i]->setText( ftext.str().c_str() );
     freqCheckBoxes[i]->setChecked( true );
     ui->layout_record->addWidget( freqCheckBoxes[i], 1, i );
  }
  

  if ( !intersonDevice.Start() ){
#ifdef DEBUG_PRINT
    std::cout << "Starting scan failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
  }

  this->timer->start();
}



void SpectroscopyUI::UpdateImage(){

    int currentIndex = intersonDevice.GetCurrentRFIndex();
  if( currentIndex >= 0 && currentIndex != lastRFRendered ){
     lastRFRendered = currentIndex;
     RFImageType::Pointer rf = intersonDevice.GetRFImage( currentIndex); 
 

     //Create BMode image
     
     m_CastFilter->SetInput( rf );
     m_BModeFilter->Update(); 
     ImageType::Pointer bmode = m_BModeFilter->GetOutput();
     
     ITKFilterFunctions< ImageType >::PermuteArray order;
     order[0] = 1;
     order[1] = 0;
     bmode = ITKFilterFunctions< ImageType>::PermuteImage(bmode, order);
     bmode = ITKFilterFunctions< ImageType >::Rescale(bmode, 0, 255);
     QImage image1 = ITKQtHelpers::GetQImageColor<ImageType>( 
                          bmode,
                          bmode->GetLargestPossibleRegion(), 
                          QImage::Format_RGB16 
                       );
  
      ui->label_BModeImage->setPixmap(QPixmap::fromImage(image1));
      ui->label_BModeImage->setScaledContents( true );
      ui->label_BModeImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );


     //Scale Rf image for display
     //Rotate 90 degrees
     //ITKFilterFunctions< RFImageType >::PermuteArray order;
     //order[0] = 1;
     //order[1] = 0;
     QImage image2;

     if( ui->checkBox_filterRF->isChecked() ){
       m_CastFilterRF->SetInput( rf );
       m_MultiplyFilter->Update();
       ImageType::Pointer rff = m_MultiplyFilter->GetOutput();
       rff = ITKFilterFunctions< ImageType>::PermuteImage(rff, order);
       rff = ITKFilterFunctions< ImageType >::Rescale(rff, 0, 255);
       image2 = ITKQtHelpers::GetQImageColor<ImageType>( 
                          rff,
                          rff->GetLargestPossibleRegion(), 
                          QImage::Format_RGB16 
                       );

     }
     else{
       rf = ITKFilterFunctions< RFImageType>::PermuteImage(rf, order);
       rf = ITKFilterFunctions< RFImageType >::Rescale(rf, 0, 255);
       image2 = ITKQtHelpers::GetQImageColor<RFImageType>( 
                          rf,
                          rf->GetLargestPossibleRegion(), 
                          QImage::Format_RGB16 
                       );
     }
  
    ui->label_rfImage->setPixmap( QPixmap::fromImage(image2) );
    ui->label_rfImage->setScaledContents( true );
    ui->label_rfImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  } 

  this->timer->start();
}

void SpectroscopyUI::SetFrequency(){
  bool success = this->intersonDevice.SetFrequency( 
                     this->ui->dropDown_Frequency->currentIndex() ); 
  IntersonArrayDeviceRF::FrequenciesType frequencies = intersonDevice.GetFrequencies();
  if( !success ){
    std::cout << "Failed to set frequency index: " << this->ui->dropDown_Frequency->currentIndex() << std::endl;
    std::cout << "Current frequency: " << frequencies[ intersonDevice.GetFrequency() ] << std::endl;
    std::cout << "Current frequency index: " << (int) intersonDevice.GetFrequency() << std::endl;
  } 
}

void SpectroscopyUI::SetDepth(){
  int depth = this->intersonDevice.SetDepth( 
                                this->ui->spinBox_Depth->value() );
  this->ui->spinBox_Depth->setValue(depth);
}

void SpectroscopyUI::SetUpperFrequency(){
  double f = this->ui->slider_upperFrequency->value() / 
             (double) this->ui->slider_upperFrequency->maximum(); 
  this->m_BandpassFilter->SetUpperFrequency( f );
  this->m_BandpassFilterRF->SetUpperFrequency( f );
}

void SpectroscopyUI::SetLowerFrequency(){
  double f = this->ui->slider_lowerFrequency->value() / 
             (double) this->ui->slider_lowerFrequency->maximum(); 
  this->m_BandpassFilter->SetLowerFrequency( f );
  this->m_BandpassFilterRF->SetLowerFrequency( f );
}

void SpectroscopyUI::SetOrder(){
  this->m_BandpassFilter->SetOrder( this->ui->spinBox_order->value() );
  this->m_BandpassFilterRF->SetOrder( this->ui->spinBox_order->value() );
}

void SpectroscopyUI::RecordRF(){
  IntersonArrayDeviceRF::FrequenciesType frequencies = intersonDevice.GetFrequencies();
  unsigned char voltLow  = ui->spinBox_voltLow->value();
  unsigned char voltHigh = ui->spinBox_voltHigh->value();
  unsigned char voltStep = ui->spinBox_voltStep->value();
  unsigned char volt = intersonDevice.GetVoltage();
  unsigned char fi = intersonDevice.GetFrequency();
  std::vector< IntersonArrayDeviceRF::RFImageType::Pointer > images;
  std::vector< std::string > imageNames;
  for(unsigned char i=0; i<freqCheckBoxes.size(); i++){
    if( !freqCheckBoxes[i]->isChecked() ){
      continue;
    }
    if( !intersonDevice.SetFrequency( i ) ){
      //TODO: report to ui
      std::cout << "Failed to set frequency: " << frequencies[i] << std::endl;
      continue;
    } 
    
    for(int v = voltLow; v <= voltHigh; v+=voltStep){

       if( !intersonDevice.SetVoltage( v ) ){
         std::cout << "Failed to set voltage: " << v << std::endl;
         continue;
       }
       int lastIndex = intersonDevice.GetCurrentRFIndex();
       int index = intersonDevice.GetCurrentRFIndex();
       while(index == lastIndex){
         Sleep(10);
         index = intersonDevice.GetCurrentRFIndex();
       }
       images.push_back( intersonDevice.GetRFImage( intersonDevice.GetCurrentRFIndex() ) );
       
       std::ostringstream ftext;   
       ftext << std::setw(3) << std::fixed << std::setfill('0');
       ftext << "rf_voltage_" << v << "_freq_";
       ftext << std::setw(10) << std::fixed; 
       ftext << frequencies[i] << ".nrrd";
       imageNames.push_back( ftext.str() );

    }
  }

  //Save Images
  for( int i=0; i<images.size(); i++){
   ImageIO<IntersonArrayDeviceRF::RFImageType>::saveImage( images[i], imageNames[i] );
  }

  //reset probe
  intersonDevice.SetFrequency( fi );
  intersonDevice.SetVoltage( volt );
  
}
