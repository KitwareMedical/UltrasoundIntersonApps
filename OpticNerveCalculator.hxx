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
    runningSum = 0;
    currentEstimate = -1;
    mean = 0;
  };
 
  ~OpticNerveCalculator(){
    Stop();
  }

  void SetNumberOfThreads( int n){
    maxNumberOfThreads = n;
  }

  void Stop(){
    if( !stopThreads ){
      stopThreads = true;
      WaitForMultipleObjects(maxNumberOfThreads, threads, true, INFINITE);
      for(int i=0; i<maxNumberOfThreads; i++){
        CloseHandle( threads[i] );
      }
      CloseHandle( toProcessMutex );
    }
  }

 bool StartProcessing( IntersonArrayDevice *source){
#ifdef DEBUG_PRINT
     std::cout << "Startprocessing called" << std::endl;
#endif

    if( !stopThreads ){
      return true;
    }
    stopThreads = false;

    //ringBuffer.clear();
    currentWrite = -1;
    nTotalWrite = 0;
    currentRead = 0;  
  
    runningSum = 0;
    currentEstimate = -1;
    mean = 0;
    estimates.clear();
    
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

/* 
     ITKFilterFunctions<IntersonArrayDevice::ImageType>::FlipArray flip;
     flip[0] = false;
     flip[1] = true;
     image = ITKFilterFunctions<IntersonArrayDevice::ImageType>::FlipImage(image, flip);
*/
     IntersonArrayDevice::ImageType::DirectionType direction = image->GetDirection();
     ITKFilterFunctions< IntersonArrayDevice::ImageType >::PermuteArray order;
     order[0] = 1;
     order[1] = 0;
     image = ITKFilterFunctions< IntersonArrayDevice::ImageType>::PermuteImage(image, order);

     image->SetDirection( direction );

    //TODO: Avoid instantion of filters every time -> setup a pipeline 
     OpticNerveEstimator one;
     //TODO: do it more centralized
     //change algorithm defaults
     //one.algParams.eyeInitialBlurFactor = 3;
     one.algParams.eyeVerticalBorderFactor = 1/20.0;      
     //one.algParams.eyeRingFactor = 1.3;
     one.algParams.eyeInitialBinaryThreshold = 50;
     //one.algParams.eyeMaskCornerXFactor = 0.9;
     //one.algParams.eyeMaskCornerYFactor = 0.1;

     typedef itk::CastImageFilter< IntersonArrayDevice::ImageType, OpticNerveEstimator::ImageType> Caster;
     Caster::Pointer caster = Caster::New();
     caster->SetInput( image );
     caster->Update();
     OpticNerveEstimator::ImageType::Pointer castImage = caster->GetOutput();
#ifdef DEBUG_PRINT
     std::cout << "Doing estimation on image" << index << std::endl;
#endif
     OpticNerveEstimator::Status status = 
               OpticNerveEstimator::ESTIMATION_UNKNOWN;
     try{
        status = one.Fit( castImage, true, "debug" );
     }
     catch( itk::ExceptionObject & err ){
#ifdef DEBUG_PRINT
	  std::cerr << "ExceptionObject caught !" << std::endl;
	  std::cerr << err << std::endl;
#endif
      }
    
     if( status != OpticNerveEstimator::ESTIMATION_SUCCESS ){
#ifdef DEBUG_PRINT
       std::cout << "Estimation failed" << index << std::endl;
#endif
       return !stopThreads;
     }
    
     typedef OpticNerveEstimator::RGBImageType RGBImageType;
     RGBImageType::Pointer overlay = one.GetOverlay();

    
     DWORD waitForMutex = WaitForSingleObject(toProcessMutex, INFINITE); 

     currentEstimate = one.GetStem().width;
     runningSum += currentEstimate;
     estimates.insert( currentEstimate );
     mean = runningSum / nTotalWrite; 
 
     //TODO: insert in order?
     int toAdd = currentWrite+1;
     if(toAdd >= ringBuffer.size() ){
       toAdd = 0;
     }
     ringBuffer[toAdd] = overlay;
     currentWrite = toAdd; 
     ++nTotalWrite;

#ifdef DEBUG_PRINT
     std::cout << "Storing current estimate " << currentWrite << std::endl;
#endif
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


   double GetMeanEstimate(){
     return mean;
   };

   double GetCurrentEstimate(){
     return currentEstimate;
   };

   double GetMedianEstimate(){ 
     double median = -1;
     if(estimates.size() > 0 ){
        //Need to make sure estimates doesn't get modified
        DWORD wresult = WaitForSingleObject( toProcessMutex, INFINITE);
        std::set<double>::iterator it = estimates.begin();
        for(int i=0; i<estimates.size() / 2; i++){
          ++it;
        }
        median = *it;
        ReleaseMutex(toProcessMutex);
     }
     return median;
   }

   bool isRunning(){
     return !stopThreads;
   }

private:

  //RingBuffer
  int currentWrite;
  long nTotalWrite; 
  std::vector< OpticNerveEstimator::RGBImageType::Pointer > ringBuffer;

  //Estimation resutl
  std::set<double> estimates;
  double runningSum;
  double mean;
  double currentEstimate;
  
  //Threading
  bool stopThreads;

  int maxNumberOfThreads; 
  HANDLE *threads;
  DWORD *threadsID;
  HANDLE toProcessMutex;
 
  //device reading
  std::atomic<int> currentRead;
  IntersonArrayDevice *device;

};

#endif
