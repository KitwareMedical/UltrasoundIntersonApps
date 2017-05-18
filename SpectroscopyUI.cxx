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

#include "ITKQtHelpers.hxx"
#include "ITKFilterFunctions.h"

#include "itkBModeImageFilter.h"
#include "itkCastImageFilter.h"

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

        intersonDevice.SetRingBufferSize( bufferSize );

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
  if ( !intersonDevice.ConnectProbe() ){
#ifdef DEBUG_PRINT
    std::cout << "Connect probe failed" << std::endl;
#endif
    return;
    //TODO: Show UI message
  }
  
  IntersonArrayDeviceRF::FrequenciesType fs = intersonDevice.GetFrequencies();
  ui->dropDown_Frequency->clear();
  for(int i=0; i<fs.size(); i++){
     std::ostringstream ftext;   
     ftext << std::setprecision(1) << std::setw(3) << std::fixed;
     ftext << fs[i] << " hz";
     ui->dropDown_Frequency->addItem( ftext.str().c_str() );
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
     typedef IntersonArrayDeviceRF::RFImageType  RFImageType;
     RFImageType::Pointer rf = intersonDevice.GetRFImage( currentIndex); 
 
     //Rotate 90 degrees
     ITKFilterFunctions< RFImageType >::PermuteArray order;
     order[0] = 1;
     order[1] = 0;
     
     rf = ITKFilterFunctions< RFImageType>::PermuteImage(rf, order);

     
     //Create BMode image
     typedef itk::Image<double, 2> ImageType;
     typedef itk::CastImageFilter<RFImageType, ImageType> CastFilter;
     CastFilter::Pointer caster = CastFilter::New();
     caster->SetInput( rf );
     caster->Update();
     ImageType::Pointer crf = caster->GetOutput();
     //crf = ITKFilterFunctions<ImageType>::Rescale(crf, 0, 1);

     typedef itk::BModeImageFilter< ImageType >  BModeFilter;
     BModeFilter::Pointer convertToBMode = BModeFilter::New();
     convertToBMode->SetInput( crf );
     convertToBMode->Update();
     ImageType::Pointer bmode = convertToBMode->GetOutput();

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
     rf = ITKFilterFunctions< RFImageType >::Rescale(rf, 0, 255);
     QImage image2 = ITKQtHelpers::GetQImageColor<RFImageType>( 
                          rf,
                          rf->GetLargestPossibleRegion(), 
                          QImage::Format_RGB16 
                       );
  
    ui->label_rfImage->setPixmap(QPixmap::fromImage(image2));
    ui->label_rfImage->setScaledContents( true );
    ui->label_rfImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  } 

  this->timer->start();
}

void SpectroscopyUI::SetFrequency(){
  this->intersonDevice.SetFrequency( 
                     this->ui->dropDown_Frequency->currentIndex() + 1 ); 
}

void SpectroscopyUI::SetDepth(){
  int depth = this->intersonDevice.SetDepth( 
                                this->ui->spinBox_Depth->value() );
  this->ui->spinBox_Depth->setValue(depth);
}

