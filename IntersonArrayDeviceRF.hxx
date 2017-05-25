
#ifndef INTERSONARRAYDEVICERF_H 
#define INTERSONARRAYDEVICERF_H 

#include <vector>

#include "IntersonArrayCxxControlsHWControls.h"
#include "IntersonArrayCxxImagingContainer.h"

#include "itkImage.h"

class IntersonArrayDeviceRF{

 
public:
 
  typedef IntersonArrayCxx::Imaging::Container ContainerType;
  
  typedef ContainerType::PixelType PixelType;
  typedef itk::Image< PixelType, 2 > ImageType;

  typedef ContainerType::RFImagePixelType RFPixelType;
  typedef itk::Image< RFPixelType, 2 > RFImageType;

  typedef IntersonArrayCxx::Controls::HWControls HWControlsType;
  typedef HWControlsType::FrequenciesType FrequenciesType;

  IntersonArrayDeviceRF(): bModeCurrent(-1), rfCurrent(-1), nBModeImagesAquired(0) {
    //Setup defaults
    frequencyIndex = 0;
    focusIndex = 0; 
    highVoltage = 50;
    gain = 100;
    depth = 50;
    steering = 0;
    probeId = -1;

    SetRingBufferSize( 10 );
    probeIsConnected = false;
  };
 
  ~IntersonArrayDeviceRF(){
  };


  void Stop(){
    hwControls.StopAcquisition();
    container.StopReadScan();
    //Sleep( 100 ); // "time to stop"
  };

 
  bool Start(){
    std::cout << "Starting probe" << std::endl;
    container.DisposeScan();
    container.StartReadScan();
    if( container.GetRFData() ){
      container.StartRFReadScan();
      return hwControls.StartRFmode();
    }
    else{
      return hwControls.StartBmode();
    }
  };

  bool GetDoubler(){
    return container.GetDoubler();
  }
  
  void SetDoubler(bool doubler){
    if( doubler != GetDoubler() ){
      Stop();
      container.SetDoubler( doubler );
      Start();
    }
  }

  unsigned char GetFrequency(){
    return frequencyIndex;
  }

  bool SetFrequency(unsigned char fIndex){
    if(frequencyIndex == fIndex){
      return true;
    }
    bool success = false;
    if(fIndex >= 0 && fIndex < frequencies.size() ){
      Stop(); 
      hwControls.GetFrequency( frequencies );
      std::cout << frequencies.size() << std::endl;
      success = hwControls.SetFrequencyAndFocus( frequencyIndex, focusIndex,
         steering );
      Start();
    }
    if(success){
      frequencyIndex = fIndex;
    }
    return success;
  }

  unsigned char GetVoltage(){
    return highVoltage;
  }
 
  bool SetVoltage( unsigned char voltage ){
    bool success = true;
    if(highVoltage != voltage){
      Stop();
      success = hwControls.SendHighVoltage( voltage, voltage );
      Start();
    }
    return success;
  }

  void SetBMode(){
    if( container.GetRFData() ){
      Stop();
      container.SetRFData(false);
      Start();   
    }
  }

  void SetRFMode(){
    if( !container.GetRFData() ){
      Stop();
      container.SetRFData( true );
      Start();
    }   
  }


  int SetDepth(int d){
    if(d == depth){
      return d;
    }
    Stop();
    depth = hwControls.ValidDepth( d );
    SetupScanConverter();
    InitalizeBModeRingBuffer();
    InitalizeRFRingBuffer();

    Start();

    return depth;
  };
 
  void SetRingBufferSize(int size){
    //TODO: would need to allcoate images if called after probe is connected
    rfRingBuffer.resize(size);
    bModeRingBuffer.resize(size);
  }


  bool ConnectProbe(bool rfData ){
    if(probeIsConnected){
      return true;
    }
    std::cout << "MAX SAMPLES: " << container.MAX_SAMPLES << std::endl;
    std::cout << "MAX RF SAMPLES: " << container.MAX_RFSAMPLES << std::endl;

    std::cout << "Finding all probes" << std::endl;
    const int steering = 0;
    typedef HWControlsType::FoundProbesType FoundProbesType;
    FoundProbesType foundProbes;
    hwControls.FindAllProbes( foundProbes );
    if( foundProbes.empty() ){
      return false;
    }

    std::cout << "Finding probe 0" << std::endl;   
    hwControls.FindMyProbe( 0 );

    std::cout << "Getting probe id" << std::endl;   
    probeId = hwControls.GetProbeID();
    if( probeId == 0 ){
      std::cerr << "Could not find the probe." << std::endl;
      return false;

    }

    std::cout << "Getting Frequencies" << std::endl;    
    hwControls.GetFrequency( frequencies );
    
    if( !hwControls.SetFrequencyAndFocus( frequencyIndex, focusIndex,
         steering ) ){
      return false;
    }

    std::cout << "Sending high voltage" << std::endl;    
    if( !hwControls.SendHighVoltage( highVoltage, highVoltage ) ){
      return false;
    }

    std::cout << "Enabling high voltage" << std::endl;    
    if( !hwControls.EnableHighVoltage() ){
      return false;
    }
 
    std::cout << "Sending dynamic gain" << std::endl;    
    if( !hwControls.SendDynamic( gain ) ){
      std::cerr << "Could not set dynamic gain." << std::endl;
    }

    //std::cout << "Disabling hard button" << std::endl;   
    //hwControls.DisableHardButton();
    hwControls.EnableHardButton();

    std::cout << "Setting hw controls" << std::endl;   
    container.SetHWControls( &hwControls );

    container.AbortScan();
    container.SetRFData( rfData );

    std::cout << "Getting lines per array" << std::endl;   
    height = hwControls.GetLinesPerArray();
    std::cout <<"Height: " <<  height << std::endl; 
 
    std::cout << "Valid depth" << std::endl;   
    if( hwControls.ValidDepth( depth ) == depth ){
      ContainerType::ScanConverterError converterError
         = SetupScanConverter(); 
      if( converterError != ContainerType::SUCCESS ){
        return false;
      }
    }
    else{
       std::cerr << "Invalid requested depth for probe." << std::endl;
    }


    std::cout << "Initalizing Buffers" << std::endl;   
    InitalizeBModeRingBuffer();
    InitalizeRFRingBuffer();


    std::cout << "Setting callbacks" << std::endl;   
    container.SetNewImageCallback( &AquireBModeImage , this );
    container.SetNewRFImageCallback( &AquireRFImage , this );


    probeIsConnected = true;
    return true;
  }


  ImageType::Pointer GetBModeImage(int ringBufferIndex){
    return bModeRingBuffer[ringBufferIndex];
  };

  ImageType::Pointer GetBModeImageAbsolute(int absoluteIndex){
    return GetBModeImage( absoluteIndex % bModeRingBuffer.size() );
  };
  
  
  long GetNumberOfBModeImagesAquired(){
    return nBModeImagesAquired;
  };

  int GetCurrentBModeIndex(){
    return bModeCurrent;
  };




  RFImageType::Pointer GetRFImage(int ringBufferIndex){
    return rfRingBuffer[ringBufferIndex];
  };

  RFImageType::Pointer GetRFImageAbsolute(int absoluteIndex){
    return GetRFImage( absoluteIndex % rfRingBuffer.size() );
  };
  
  long GetRFBModeImagesAquired(){
    return nRFImagesAquired;
  };

  int GetCurrentRFIndex(){
    return rfCurrent;
  };

  
  void AddBModeImageToBuffer(PixelType *buffer){

    int index = bModeCurrent + 1;
    if(index >= bModeRingBuffer.size() ){
      index = 0;
    }
    ImageType::Pointer image = bModeRingBuffer[index];
    const ImageType::RegionType & largestRegion =
      image->GetLargestPossibleRegion();

    const ImageType::SizeType imageSize = largestRegion.GetSize();

    PixelType *imageBuffer = image->GetPixelContainer()->GetBufferPointer();
    std::memcpy( imageBuffer, buffer, 
                 imageSize[0] * imageSize[1] * sizeof( PixelType ) );
   

    nBModeImagesAquired++;  
    bModeCurrent = index;
  }


  static void __stdcall AquireBModeImage(PixelType *buffer, void *instance){
     IntersonArrayDeviceRF *device = (IntersonArrayDeviceRF*) instance;
     device->AddBModeImageToBuffer( buffer );    
  };


  
  void AddRFImageToBuffer(RFPixelType *buffer){

    int index = rfCurrent + 1;
    if(index >= rfRingBuffer.size() ){
      index = 0;
    }
    RFImageType::Pointer image = rfRingBuffer[index];
    const ImageType::RegionType & largestRegion =
      image->GetLargestPossibleRegion();

    const ImageType::SizeType imageSize = largestRegion.GetSize();

    RFPixelType *imageBuffer = image->GetPixelContainer()->GetBufferPointer();
    std::memcpy( imageBuffer, buffer, 
                 imageSize[0] * imageSize[1] * sizeof( RFPixelType ) );
   

    nRFImagesAquired++;  
    rfCurrent = index;
  }


  static void __stdcall AquireRFImage(RFPixelType *buffer, void *instance){
     IntersonArrayDeviceRF *device = (IntersonArrayDeviceRF*) instance;
     device->AddRFImageToBuffer( buffer );    
  };




  FrequenciesType GetFrequencies(){
    return frequencies;
  };


  float GetMmPerPixel(){
    return container.GetMmPerPixel();
  };


  HWControlsType &GetHWControls(){
    return hwControls;
  };



private:

  //BMode Ringbuffer for storing images continuously 
  std::atomic< int > bModeCurrent;
  std::vector< ImageType::Pointer > bModeRingBuffer;
  long nBModeImagesAquired;
  
  //RF Ringbuffer for storing images continuously 
  std::atomic< int > rfCurrent;
  std::vector< RFImageType::Pointer > rfRingBuffer;
  long nRFImagesAquired;
  
  //Probe setups
  bool probeIsConnected;

  //Probe settings
  FrequenciesType frequencies;
  int steering;
  unsigned char frequencyIndex;
  unsigned char focusIndex;
  unsigned char highVoltage;
  int gain;
  int depth;
  //int width;
  int height;
  //int bModeHeight;
  //int bModeWidth;
  //int rfHeight;
  //int rfWidth;
  unsigned int probeId;
  
  HWControlsType hwControls;
  ContainerType container;


  void InitalizeBModeRingBuffer(){
  
    for(int i=0;  i<bModeRingBuffer.size(); i++){
      ImageType::Pointer image = ImageType::New();
      
      ImageType::IndexType imageIndex;
      imageIndex.Fill( 0 );
      
      ImageType::SizeType imageSize;
      imageSize[0] = ContainerType::MAX_SAMPLES;
      imageSize[1] = height;
      
      ImageType::RegionType imageRegion;
      imageRegion.SetIndex( imageIndex );
      imageRegion.SetSize( imageSize );
      
      image->SetRegions( imageRegion );
      image->Allocate();
      
      bModeRingBuffer[i] = image;
    }
  };

  void InitalizeRFRingBuffer(){
#ifdef DEBUG_PRINT
    std::cout << "Setting up RFRingBuffer" << std::endl;
#endif

    for(int i = 0;  i < rfRingBuffer.size(); i++){
      RFImageType::Pointer image = RFImageType::New();
      
      RFImageType::IndexType imageIndex;
      imageIndex.Fill( 0 );
      
      RFImageType::SizeType imageSize;
      imageSize[0] = ContainerType::MAX_RFSAMPLES;
      imageSize[1] = height;
      
      RFImageType::RegionType imageRegion;
      imageRegion.SetIndex( imageIndex );
      imageRegion.SetSize( imageSize );
      
      image->SetRegions( imageRegion );
      image->Allocate();
      
      rfRingBuffer[i] = image;
    }
  }


  ContainerType::ScanConverterError SetupScanConverter(){
    int scanWidth = ContainerType::MAX_SAMPLES; //Does not matter
    int scanHeight = height; //Does not matter
    if( container.GetRFData() ){
      scanWidth = ContainerType::MAX_RFSAMPLES;
    }
    ContainerType::ScanConverterError converterErrorIdle =
         container.IdleInitScanConverter( depth, scanWidth, scanHeight, probeId,
                                         steering, false, false, 0 );

    ContainerType::ScanConverterError converterError =
         container.HardInitScanConverter( depth, scanWidth, scanHeight, steering );
     
    return converterError;  

  }

};

#endif
