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
  
  OpticNerveCalculator(): currentWrite(-1), nTotalWrite(0), 
                          currentRead(0){
    maxNumberOfThreads=1;
    stopThreads = true;
    ringBuffer.resize(10);
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
    CloseHandle( toProcessMutex );
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

    toProcessMutex = CreateMutex(NULL, false, NULL);
    threads = new HANDLE[maxNumberOfThreads];
    threadsID = new DWORD[maxNumberOfThreads];
    for(int i=0; i<maxNumberOfThreads; i++){
      threads[i] = CreateThread(NULL, 0, OpticNerveCalculator::CalculateOpticNerveWidth, this, 0, &threadsID[i]);
    }

    return true;
  }


  bool ProcessNext(){

     //No need for mutex anymore
     int index = currentRead++;
     while( index >= device->GetNumberOfImagesAquired() ){
       Sleep(10);
     }
#ifdef DEBUG_PRINT
     //std::cout << "Calculating optic nerve on next image" << std::endl;
#endif
     //This might grab the same image twice if the index in another thread 
     //is the same modulo ringbuffer size
     IntersonArrayDevice::ImageType::Pointer image = 
                            device->GetImageAbsolute( index );

     //TODO: Avoid instantion of filters every time -> setup a pipeline 
     OpticNerveEstimator one;
     typedef itk::CastImageFilter< IntersonArrayDevice::ImageType, OpticNerveEstimator::ImageType> Caster;
     Caster::Pointer caster = Caster::New();
     caster->SetInput( image );
     caster->Update();
     OpticNerveEstimator::ImageType::Pointer castImage = caster->GetOutput();
#ifdef DEBUG_PRINT
     std::cout << "Doing estimation on image" << index << std::endl;
#endif
     bool success = false;
     try{
       success = one.Fit( castImage, true, "debug" );
     }
     catch( itk::ExceptionObject & err ){
#ifdef DEBUG_PRINT
	  std::cerr << "ExceptionObject caught !" << std::endl;
	  std::cerr << err << std::endl;
#endif
      }
    
     if(!success){
#ifdef DEBUG_PRINT
       std::cout << "Estimation failed" << index << std::endl;
#endif
       return !stopThreads;
     }
    
#ifdef DEBUG_PRINT
     std::cout << "Storing current estimate" << std::endl;
#endif
     
     typedef OpticNerveEstimator::RGBImageType RGBImageType;
     RGBImageType::Pointer overlay = one.GetOverlay();

    
     DWORD waitForMutex = WaitForSingleObject(toProcessMutex, INFINITE); 

     //TODO: insert in order?
     int toAdd = currentWrite+1;
     if(toAdd >= ringBuffer.size() ){
       toAdd = 0;
     }
     ringBuffer[toAdd] = overlay;
     currentWrite = toAdd; 
     ++nTotalWrite;

     ReleaseMutex( toProcessMutex );

 
     return !stopThreads;
  };


 
  static DWORD WINAPI CalculateOpticNerveWidth(LPVOID lpParam){
    OpticNerveCalculator *calc = (OpticNerveCalculator*) lpParam;
    //Keep processing until ProcessNext says to stop
    while( 
      calc->ProcessNext()
    ){}
    return 0;
  };



  OpticNerveEstimator::RGBImageType::Pointer GetImage(int ringBufferIndex){
    return ringBuffer[ringBufferIndex];
  };



  OpticNerveEstimator::RGBImageType::Pointer GetImageAbsolute(int absoluteIndex){
    return GetImage( absoluteIndex % ringBuffer.size() );
  };


  
  int GetCurrentIndex(){
    return currentWrite;
  };



  void SetRingBufferSize(int size){
    ringBuffer.resize(size);
  };


  
  long GetNumberOfEstimates(){
    return nTotalWrite;
  };



private:

  std::atomic<int> currentWrite;
  long nTotalWrite; 
  std::vector< OpticNerveEstimator::RGBImageType::Pointer > ringBuffer;


  bool stopThreads;

  int maxNumberOfThreads; 
  HANDLE *threads;
  DWORD *threadsID;
  HANDLE toProcessMutex;
 
  std::atomic<int> currentRead;
  IntersonArrayDevice *device;

};

#endif
