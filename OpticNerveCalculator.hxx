#ifndef OPTICNERVECALCULATOR_H
#define OPTICNERVECALCULATOR_H

#define DEBUG_PRINT


#include <Windows.h>

#include <QLabel.h>

#include "IntersonArrayDevice.hxx"
#include "ITKQtHelpers.hxx"
#include "OpticNerveEstimator.hxx"

#include <vector>
#include <atomic>

class OpticNerveCalculator
{
public:
  
  OpticNerveCalculator(): current(0), labelImage(NULL) {
    maxNumberOfThreads=4;
    stopThreads = true;
  };
 
  ~OpticNerveCalculator(){
    Stop();
  }

  void Stop(){
    stopThreads = true;
    WaitForMultipleObjects(maxNumberOfThreads, threads, true, INFINITE);
    for(int i=0; i<maxNumberOfThreads; i++){
      CloseHandle( threads[i] );
    }
    stopThreads = false;
  } 

  void SetNumberOfThreads(int nThreads){
    maxNumberOfThreads = nThreads;
  }

  bool StartProcessing( IntersonArrayDevice *source){
#ifdef DEBUG_PRINT
     std::cout << "Startprocessing called" << std::endl;
#endif

    if( !stopThreads ){
      return true;
    }
    stopThreads = false;

    this->device = source;

    //Spawn work threads
#ifdef DEBUG_PRINT
     std::cout << "Spawning worker threads" << std::endl;
#endif
    threads = new HANDLE[maxNumberOfThreads];
    threadsID = new DWORD[maxNumberOfThreads];
    for(int i=0; i<maxNumberOfThreads; i++){
      threads[i] = CreateThread(NULL, 0, OpticNerveCalculator::CalculateOpticNerveWidth, this, 0, &threadsID[i]);
    }

    return true;
  }

  void SetImageLabel( QLabel *label){
    labelImage = label;
  }


  bool ProcessNext(){

     //Might grab an oout of date image if current is increased 
     // after the check here.
     //Would need mutex
     if( current >= device->GetNumberOfImagesAquired() ){
       return !stopThreads;
     }
#ifdef DEBUG_PRINT
     std::cout << "Calculating optiv nerve on next image" << std::endl;
#endif
     int index = current;
     IntersonArrayDevice::ImageType::Pointer image = 
                            device->GetImageAbsolute( current++ );
     
     OpticNerveEstimator one;
     typedef itk::CastImageFilter< IntersonArrayDevice::ImageType, OpticNerveEstimator::ImageType> Caster;
     Caster::Pointer caster = Caster::New();
     caster->SetInput( image );
     caster->Update();
     OpticNerveEstimator::ImageType::Pointer castImage = caster->GetOutput();
#ifdef DEBUG_PRINT
     std::cout << "Doing estimation on image" << index << std::endl;
#endif
     bool success = one.Fit( castImage, true, false );

     if(!success){
#ifdef DEBUG_PRINT
     std::cout << "Estimation failed" << index << std::endl;
#endif
       return !stopThreads;
     }
    
#ifdef DEBUG_PRINT
     std::cout << "Displaying current estimate" << std::endl;
#endif
     //Convert back to QTimage
     typedef OpticNerveEstimator::RGBImageType RGBImageType;
     RGBImageType::Pointer overlay = one.GetOverlay();
     QImage qimage = ITKQtHelpers::GetQImageColor_Vector<RGBImageType>( 
                           overlay,
                           overlay->GetLargestPossibleRegion(), 
                           QImage::Format_RGB16 );
     //Set overlay image to display
     labelImage->setPixmap(QPixmap::fromImage(qimage));
     labelImage->setScaledContents( true );
     labelImage->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
     return !stopThreads;
  };


  void SetQLabel(QLabel *label){
    labelImage = label;
  }

 
  static DWORD WINAPI CalculateOpticNerveWidth(LPVOID lpParam){
    OpticNerveCalculator *calc = (OpticNerveCalculator*) lpParam;
    //Keep processing until ProcessNext says to stop
    while( 
      calc->ProcessNext()
    ){}
    return 0;
  };


private:
  QLabel *labelImage;

  bool stopThreads;

  int maxNumberOfThreads; 
  HANDLE *threads;
  DWORD *threadsID;
  HANDLE toProcessMutex;
 
  IntersonArrayDevice *device;

  std::atomic<int> current;
};

#endif
