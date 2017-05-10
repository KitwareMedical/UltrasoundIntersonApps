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

#include "OpticNerveUI.h"

#include <QCloseEvent>

void OpticNerveUI::closeEvent (QCloseEvent *event){

#ifdef DEBUG_PRINT
     std::cout << "Close event called" << std::endl;
#endif
   opticNerveCalculator.Stop();
   intersonDevice.Stop();
}

OpticNerveUI::OpticNerveUI(int numberOfThreads, int bufferSize, QWidget *parent) 
	: QMainWindow(parent), ui(new Ui::MainWindow), 
          lastRendered(-1), lastOverlayRendered(-1)
{

	//Setup the graphical layout on this current Widget
	ui->setupUi(this);

	this->setWindowTitle("Ultrasound Optic Nerve Estimation");

	//Links buttons and actions
	connect( ui->pushButton_ConnectProbe, 
                 SIGNAL(clicked()), this, SLOT(ConnectProbe()));
	//connect( ui->dropDown_Depth, 
        //         SIGNAL(valueChanged(int)), this, SLOT(SetDepth()));
	//connect( ui->comboBox_Frequency, 
        //         SIGNAL(valueChanged(int)), this, SLOT(SetFrequency()));
      
        intersonDevice.SetRingBufferSize( bufferSize );

        opticNerveCalculator.SetNumberOfThreads( numberOfThreads ); 
 
               

	// Timer
	this->timer = new QTimer(this);
	this->timer->setSingleShot(true);
	this->timer->setInterval(50);
	this->connect(timer, SIGNAL(timeout()), SLOT(UpdateImage()));
}

OpticNerveUI::~OpticNerveUI()
{
	//this->intersonDevice.Stop();
	delete ui;
}

void OpticNerveUI::ConnectProbe(){
#ifdef DEBUG_PRINT
  std::cout << "Connect probe called" << std::endl;
#endif
  if ( !intersonDevice.ConnectProbe() ){
#ifdef DEBUG_PRINT
    std::cout << "Connect probe failed" << std::endl;
#endif
    //TODO: Show UI message
  }
  
  if ( !intersonDevice.Start() ){
#ifdef DEBUG_PRINT
    std::cout << "Starting scan failed" << std::endl;
#endif
    //TODO: Show UI message
  }

#ifdef DEBUG_PRINT
    std::cout << "Calling start optic nerve estimation processing" << std::endl;
#endif
  opticNerveCalculator.StartProcessing( &intersonDevice ); 
  
  this->timer->start();
}



void OpticNerveUI::UpdateImage(){

   //display bmode image
  int currentIndex = intersonDevice.GetCurrentIndex();
  if( currentIndex > 0 && currentIndex != lastRendered ){
     lastRendered = currentIndex;
     IntersonArrayDevice::ImageType::Pointer bmode = 
                                 intersonDevice.GetImage( currentIndex); 
     QImage image = ITKQtHelpers::GetQImageColor<IntersonArrayDevice::ImageType>( 
                          bmode,
                          bmode->GetLargestPossibleRegion(), 
                          QImage::Format_RGB16 
                       );
  
#ifdef DEBUG_PRINT
   // std::cout << "Setting Pixmap " << currentIndex << std::endl;
#endif
    ui->label_BModeImage->setPixmap(QPixmap::fromImage(image));
    ui->label_BModeImage->setScaledContents( true );
    ui->label_BModeImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  }


  //display estimate image
  int currentOverlayIndex = opticNerveCalculator.GetCurrentIndex();
  if( currentOverlayIndex > 0 && currentOverlayIndex != lastOverlayRendered ){
#ifdef DEBUG_PRINT
     std::cout << "Display overlay image" << std::endl;
#endif
     lastOverlayRendered = currentOverlayIndex;
     //Set overlay image to display
     typedef OpticNerveEstimator::RGBImageType RGBImageType;
     RGBImageType::Pointer overlay = 
            opticNerveCalculator.GetImageAbsolute( currentOverlayIndex );

     QImage qimage = ITKQtHelpers::GetQImageColor_Vector< RGBImageType>( 
                           overlay,
                           overlay->GetLargestPossibleRegion(), 
                           QImage::Format_RGB16 );
     ui->label_OpticNerveImage->setPixmap(QPixmap::fromImage(qimage));
     ui->label_OpticNerveImage->setScaledContents( true );
     ui->label_OpticNerveImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  }
  this->timer->start();
}



//void OpticNerveUI::SetFrequency(){
  //this->videoDevice->StopRecording();
  //this->videoDevice->SetProbeFrequencyMhz(GetFrequency());
  //this->videoDevice->StartRecording();
//}

//void OpticNerveUI::SetDepth(){
  //this->videoDevice->StopRecording();
  //this->videoDevice->SetDepth( this->ui->dropDown_Depth->value() );
  //this->videoDevice->StartRecording();
//}

