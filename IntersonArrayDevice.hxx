
#ifndef INTERSONARRAYDEVICE_H 
#define INTERSONARRAYDEVICE_H 

#include <vector>

#include "IntersonArrayCxxControlsHWControls.h"
#include "IntersonArrayCxxImagingContainer.h"

#include "itkImage.h"

class IntersonArrayDevice{

 
public:
 
  typedef IntersonArrayCxx::Imaging::Container ContainerType;
  //typedef ContainerType::PixelType PixelType;
  typedef unsigned char PixelType;
  typedef itk::Image< PixelType, 2 > ImageType;


  IntersonArrayDevice(): current(-1),
                         nImagesAquired(0)
  {
    //Setup defaults
    frequencyIndex = 1;
    focusIndex = 0; 
    highVoltage = 50;
    gain = 100;
    depth = 100;

    probeIsConnected = false;
  };
 
  ~IntersonArrayDevice(){
  };

  void Stop(){
    hwControls.StopAcquisition();
    container.StopReadScan();
    //Sleep( 100 ); // "time to stop"
  };
 
  bool Start(){
    container.DisposeScan();
  
    container.StartReadScan();
    return hwControls.StartBmode(); 
  };
 
  int GetCurrentIndex(){
    return current;
  };

  void SetRingBufferSize(int size){
    ringBuffer.resize(size);
  }

  bool ConnectProbe(){
    if(probeIsConnected){
      return true;
    }

#ifdef DEBUG_PRINT
  std::cout << "Checking for probes" << std::endl;
#endif
    const int steering = 0;
    typedef HWControlsType::FoundProbesType FoundProbesType;
    FoundProbesType foundProbes;
    hwControls.FindAllProbes( foundProbes );
    if( foundProbes.empty() ){
      return false;
    }

    hwControls.FindMyProbe( 0 );

    const unsigned int probeId = hwControls.GetProbeID();
    if( probeId == 0 ){
      std::cerr << "Could not find the probe." << std::endl;
      return false;

    }

    HWControlsType::FrequenciesType frequencies;
    hwControls.GetFrequency( frequencies );
    if( !hwControls.SetFrequencyAndFocus( frequencyIndex, focusIndex,
         steering ) ){
      return false;
    }


#ifdef DEBUG_PRINT
  std::cout << "Sending high voltage" << std::endl;
#endif

    if( !hwControls.SendHighVoltage( highVoltage, highVoltage ) ){
      return false;
    }

#ifdef DEBUG_PRINT
  std::cout << "Enabling high voltage" << std::endl;
#endif
    if( !hwControls.EnableHighVoltage() ){
      return false;
    }

#ifdef DEBUG_PRINT
  std::cout << "Setting dynamic gain" << std::endl;
#endif
    if( !hwControls.SendDynamic( gain ) ){
      std::cerr << "Could not set dynamic gain." << std::endl;
    }

#ifdef DEBUG_PRINT
  std::cout << "Disable hard button" << std::endl;
#endif
    hwControls.DisableHardButton();

#ifdef DEBUG_PRINT
  std::cout << "Abort scan" << std::endl;
#endif
    container.AbortScan();
    container.SetRFData( false );

#ifdef DEBUG_PRINT
  std::cout << "Checking depth" << std::endl;
#endif
    const int height = hwControls.GetLinesPerArray();
    const int width = 512; //container.MAX_SAMPLES;
    if( hwControls.ValidDepth( depth ) == depth ){
      ContainerType::ScanConverterError converterErrorIdle =
        container.IdleInitScanConverter( depth, width, height, probeId,
                                         steering, false, false, 0 );

      ContainerType::ScanConverterError converterError =
         container.HardInitScanConverter( depth, width, height, steering );

      if( converterError != ContainerType::SUCCESS ){
        return false;
      }
    }
    else{
       std::cerr << "Invalid requested depth for probe." << std::endl;
    }


#ifdef DEBUG_PRINT
  std::cout << "Image size: " << height << " x " << depth << std::endl;
#endif

#ifdef DEBUG_PRINT
  std::cout << "Setting up ringBuffer" << std::endl;
#endif
    const int scanWidth = container.GetWidthScan();
    const int scanHeight = container.GetHeightScan();

    for(int i=0;  i<ringBuffer.size(); i++){
      ImageType::Pointer image = ImageType::New();
      
      ImageType::IndexType imageIndex;
      imageIndex.Fill( 0 );
      
      ImageType::SizeType imageSize;
      imageSize[0] = width;
      imageSize[1] = height;
      
      ImageType::RegionType imageRegion;
      imageRegion.SetIndex( imageIndex );
      imageRegion.SetSize( imageSize );
      
      image->SetRegions( imageRegion );
      image->Allocate();
      
      ringBuffer[i] = image;
    }

#ifdef DEBUG_PRINT
  std::cout << "Add callback" << std::endl;
#endif
    container.SetNewImageCallback( &AquireImage , this );
   

#ifdef DEBUG_PRINT
  std::cout << "Start scanning" << std::endl;
#endif

    probeIsConnected = true;
    return true;
  }


  ImageType::Pointer GetImage(int ringBufferIndex){
    return ringBuffer[ringBufferIndex];
  };

  ImageType::Pointer GetImageAbsolute(int absoluteIndex){
    return GetImage( absoluteIndex % ringBuffer.size() );
  };
  
  
  long GetNumberOfImagesAquired(){
    return nImagesAquired;
  };

  
  void AddImageToBuffer(PixelType *buffer){

    int index = current + 1;
#ifdef DEBUG_PRINT
    //std::cout << "Adding Image to Buffer" <<  index << std::endl;
#endif
    if(index >= ringBuffer.size() ){
      index = 0;
    }
    ImageType::Pointer image = ringBuffer[index];
    const ImageType::RegionType & largestRegion =
      image->GetLargestPossibleRegion();

    const ImageType::SizeType imageSize = largestRegion.GetSize();

    PixelType *imageBuffer = image->GetPixelContainer()->GetBufferPointer();
    std::memcpy( imageBuffer, buffer, 
                 imageSize[0] * imageSize[1] * sizeof( PixelType ) );
    
    nImagesAquired++;  
    current = index;
  }


  static void __stdcall AquireImage(PixelType *buffer, void *instance){
#ifdef DEBUG_PRINT
   // std::cout << "Interson probe callback" << std::endl;
#endif
     IntersonArrayDevice *device = (IntersonArrayDevice*) instance;
     device->AddImageToBuffer( buffer );    
  };



private:

  //Ringbuffer for storing images continuously 
  std::atomic< int > current;
  std::vector< ImageType::Pointer > ringBuffer;
  
  //Probe setups
  bool probeIsConnected;

  int frequencyIndex;
  int focusIndex;
  int highVoltage;
  int gain;
  int depth;
    
  typedef IntersonArrayCxx::Controls::HWControls HWControlsType;
  HWControlsType hwControls;
  
  ContainerType container;

  long nImagesAquired;
};

#endif
