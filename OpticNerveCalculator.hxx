/*=========================================================================
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

#ifndef OPTICNERVECALCULATOR_H
#define OPTICNERVECALCULATOR_H

//#define DEBUG_PRINT

#include <cmath>
#include <Windows.h>

#include <QLabel.h>

#include "IntersonArrayDeviceRF.hxx"
#include "ITKQtHelpers.hxx"
#include "OpticNerveEstimator.hxx"

#include <vector>
#include <atomic>

class OpticNerveCalculator
{

public:

  struct Statistics
    {
    double mean = -1;
    double stdev = -1;
    double median = -1;
    double lowerQuartile = -1;
    double upperQuartile = -1;
    };

  OpticNerveCalculator() : currentWrite( -1 ), nTotalWrite( 0 ),
    currentRead( 0 )
    {
    maxNumberOfThreads = 1;
    stopThreads = true;
    ringBuffer.resize( 10 );
    runningSum = 0;
    currentEstimate = -1;
    mean = 0;
    nerveOnly = false;
    depth = 80;
    height = 100;
    };

  ~OpticNerveCalculator()
    {
    Stop();
    }

  void SetNumberOfThreads( int n )
    {
    maxNumberOfThreads = n;
    }

  int GetMaximumNumberOfThreads()
    {
    return maxNumberOfThreads;
    }

  void Stop()
    {
    if( !stopThreads )
      {
      stopThreads = true;
      WaitForMultipleObjects( maxNumberOfThreads, threads, true, INFINITE );
      for( int i = 0; i < maxNumberOfThreads; i++ )
        {
        CloseHandle( threads[ i ] );
        }
      CloseHandle( toProcessMutex );
      }
    }

  bool StartProcessing( IntersonArrayDeviceRF *source )
    {
#ifdef DEBUG_PRINT
    std::cout << "Startprocessing called" << std::endl;
#endif

    if( !stopThreads )
      {
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

    toProcessMutex = CreateMutex( NULL, false, NULL );
    threads = new HANDLE[ maxNumberOfThreads ];
    threadsID = new DWORD[ maxNumberOfThreads ];

    DWORD spawnThreadID;
    HANDLE spawnThread = CreateThread( NULL, 0, OpticNerveCalculator::StartThreads, this, 0, &spawnThreadID );
    CloseHandle( spawnThread );

    return true;
    }

  bool ProcessNext()
    {
    //No need for mutex anymore
    int index = currentRead++;
    while( index >= device->GetNumberOfBModeImagesAquired() )
      {
      Sleep( 10 );
      }
#ifdef DEBUG_PRINT
    //std::cout << "Calculating optic nerve on next image" << std::endl;
#endif
    //This might grab the same image twice if the index in another thread
    //is the same modulo ringbuffer size
    IntersonArrayDeviceRF::ImageType::Pointer image =
      device->GetBModeImageAbsolute( index );

/*
    ITKFilterFunctions<IntersonArrayDevice::ImageType>::FlipArray flip;
    flip[0] = false;
    flip[1] = true;
    image = ITKFilterFunctions<IntersonArrayDevice::ImageType>::FlipImage(image, flip);
*/
    IntersonArrayDeviceRF::ImageType::DirectionType direction = image->GetDirection();
    ITKFilterFunctions< IntersonArrayDeviceRF::ImageType >::PermuteArray order;
    order[ 0 ] = 1;
    order[ 1 ] = 0;
    image = ITKFilterFunctions< IntersonArrayDeviceRF::ImageType>::PermuteImage( image, order );

    image->SetDirection( direction );

    //Copy in case it changes during calculation
    bool doNerveOnly = this->nerveOnly;


    //TODO: Avoid instantion of filters every time -> setup a pipeline
    OpticNerveEstimator one;
    one.algParams = algParams;

    typedef itk::CastImageFilter< IntersonArrayDeviceRF::ImageType, OpticNerveEstimator::ImageType> Caster;
    Caster::Pointer caster = Caster::New();
    caster->SetInput( image );
    caster->Update();
    OpticNerveEstimator::ImageType::Pointer castImage = caster->GetOutput();

#ifdef DEBUG_PRINT
    std::cout << "Doing estimation on image" << index << std::endl;
#endif
    OpticNerveEstimator::Status status =
      OpticNerveEstimator::ESTIMATION_UNKNOWN;
    try
      {
      if( doNerveOnly )
        {
        //Setup nerve region
        OpticNerveEstimator::ImageType::RegionType imageRegion = castImage->GetLargestPossibleRegion();
        OpticNerveEstimator::ImageType::SizeType imageSize = imageRegion.GetSize();

        OpticNerveEstimator::ImageType::IndexType desiredStart;
        desiredStart[ 0 ] = 0;
        //desiredStart[1] = std::max(0, std::min(depth, (int) (imageSize[1] - height) ) );
        desiredStart[ 1 ] = imageSize[ 1 ] - height;

        OpticNerveEstimator::ImageType::SizeType desiredSize;
        desiredSize[ 0 ] = imageSize[ 0 ];
        //desiredSize[1] = std::min( height, (int) ( imageSize[1] - desiredStart[1]) );
        desiredSize[ 1 ] = height;

        OpticNerveEstimator::ImageType::RegionType desiredRegion( desiredStart, desiredSize );

        bool fitNerve = one.FitNerve( castImage, desiredRegion, true, "debug" );
        if( fitNerve )
          {
          status = OpticNerveEstimator::ESTIMATION_SUCCESS;
          }
        else
          {
          status = OpticNerveEstimator::ESTIMATION_FAIL_NERVE;
          }
        }
      else
        {
        status = one.Fit( castImage, true, "debug" );
        }
      }
    catch( itk::ExceptionObject & err )
      {
#ifdef DEBUG_PRINT
      std::cerr << "ExceptionObject caught !" << std::endl;
      std::cerr << err << std::endl;
#endif
      }

    if( status != OpticNerveEstimator::ESTIMATION_SUCCESS )
      {
#ifdef DEBUG_PRINT
      std::cout << "Estimation failed " << index << std::endl;
#endif
      return !stopThreads;
      }

#ifdef DEBUG_PRINT
    std::cout << "Getting overlay image" << index << std::endl;
#endif
    typedef OpticNerveEstimator::RGBImageType RGBImageType;
    RGBImageType::Pointer overlay = one.GetOverlay( castImage, doNerveOnly );
#ifdef DEBUG_PRINT
    std::cout << "Getting overlay image done" << index << std::endl;
#endif


    DWORD waitForMutex = WaitForSingleObject( toProcessMutex, INFINITE );

    currentEstimate = one.GetNerve().width;
    runningSum += currentEstimate;
    estimates.insert( currentEstimate );
    mean = runningSum / nTotalWrite;

    //TODO: insert in order?
    int toAdd = currentWrite + 1;
    if( toAdd >= ringBuffer.size() )
      {
      toAdd = 0;
      }
    ringBuffer[ toAdd ] = overlay;
    currentWrite = toAdd;
    ++nTotalWrite;

#ifdef DEBUG_PRINT
    std::cout << "Storing current estimate " << currentWrite << std::endl;
#endif
    ReleaseMutex( toProcessMutex );

    return !stopThreads;
    };

  OpticNerveEstimator::RGBImageType::Pointer GetImage( int ringBufferIndex )
    {
    return ringBuffer[ ringBufferIndex ];
    };

  OpticNerveEstimator::RGBImageType::Pointer GetImageAbsolute( int absoluteIndex )
    {
    return GetImage( absoluteIndex % ringBuffer.size() );
    };

  int GetCurrentIndex()
    {
    return currentWrite;
    };

  void SetRingBufferSize( int size )
    {
    ringBuffer.resize( size );
    };

  long GetNumberOfEstimates()
    {
    return nTotalWrite;
    };

  double GetMeanEstimate()
    {
    return mean;
    };

  double GetCurrentEstimate()
    {
    return currentEstimate;
    };

  Statistics GetEstimateStatistics()
    {
    Statistics stats;
    if( estimates.size() > 0 )
      {
      //Need to make sure estimates doesn't get modified
      DWORD wresult = WaitForSingleObject( toProcessMutex, INFINITE );
      stats.mean = GetMeanEstimate();
      double m2 = stats.mean * stats.mean;
      stats.stdev = 0;
      int low = estimates.size() / 4;
      int med = estimates.size() / 2;
      int high = 3 * estimates.size() / 4;
      std::set<double>::iterator it = estimates.begin();
      for( int i = 0; i < estimates.size() ; i++, ++it )
        {
        double tmp = *it - stats.mean;
        stats.stdev += tmp * tmp;
        if( i == low )
          {
          stats.lowerQuartile = *it;
          }
        else if( i == med )
          {
          stats.median = *it;
          }
        else if( i == high )
          {
          stats.upperQuartile = *it;
          }
        }
      if( estimates.size() > 1 )
        {
        stats.stdev = sqrt( stats.stdev / ( estimates.size() - 1 ) );
        }
      else
        {
        stats.stdev = 0;
        }
      ReleaseMutex( toProcessMutex );
      }
    return stats;
    }

  bool isRunning()
    {
    return !stopThreads;
    };

  void SetDepth( int d )
    {
    depth = d;
    }

  void SetHeight( int h )
    {
    height = h;
    }

  void SetNerveOnly( bool nOnly )
    {
    nerveOnly = nOnly;
    };

  void SetAlgorithmParameters( OpticNerveEstimator::Parameters &params )
    {
    algParams = params;
    };

private:

  OpticNerveEstimator::Parameters algParams;
  bool nerveOnly;
  int depth;
  int height;

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
  IntersonArrayDeviceRF *device;

  static DWORD WINAPI StartThreads( LPVOID lpParam )
    {
#ifdef DEBUG_PRINT
    std::cout << "Spawning worker threads" << std::endl;
#endif

    OpticNerveCalculator *calc = ( OpticNerveCalculator* )lpParam;
    for( int i = 0; i < calc->GetMaximumNumberOfThreads(); i++ )
      {
      calc->StartThread( i );
      Sleep( 1300 / ( calc->GetMaximumNumberOfThreads() + 1 ) );
      }
    return 0;
    }

  static DWORD WINAPI CalculateOpticNerveWidth( LPVOID lpParam )
    {
    OpticNerveCalculator *calc = ( OpticNerveCalculator* )lpParam;
    //Keep processing until ProcessNext says to stop
    while( calc->ProcessNext() )
      {
      }
    return 0;
    };

  void StartThread( int i )
    {
    threads[ i ] = CreateThread( NULL, 0, OpticNerveCalculator::CalculateOpticNerveWidth, this, 0, &threadsID[ i ] );
    }

};

#endif
