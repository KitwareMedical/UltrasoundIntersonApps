/*=========================================================================
 *
 *  Copyright David Doria 2012 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef ITKQTHELPERS_H
#define ITKQTHELPERS_H

// Qt
#include <QColor>
#include <QImage>
#include <QMetaType>
class QGraphicsView;
class QTableWidget;

// ITK
#include "itkImageRegion.h"
#include "itkImage.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"

namespace ITKQtHelpers
{
  template <typename TImage>
  QImage GetQImageColor( const TImage* const image, QImage::Format format )
    {
    return GetQImageColor( image, image->GetLargestPossibleRegion(), format );
    }

  /** Get a color QImage from a scalar ITK image. */
  template <typename TImage>
  QImage GetQImageColor( const TImage* const image,
    const itk::ImageRegion<2>& region, QImage::Format format )
    {
    QImage qimage( region.GetSize()[ 0 ], region.GetSize()[ 1 ], format );

    typedef itk::RegionOfInterestImageFilter< TImage, TImage >
      RegionOfInterestImageFilterType;
    typename RegionOfInterestImageFilterType::Pointer
      regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
    regionOfInterestImageFilter->SetRegionOfInterest( region );
    regionOfInterestImageFilter->SetInput( image );
    regionOfInterestImageFilter->Update();

    itk::ImageRegionIterator<TImage>
      imageIterator( regionOfInterestImageFilter->GetOutput(),
        regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion() );

    while( !imageIterator.IsAtEnd() )
      {
      typename TImage::PixelType pixel = imageIterator.Get();

      itk::Index<2> index = imageIterator.GetIndex();

      int gray = pixel;

      QColor pixelColor( gray, gray, gray );
      qimage.setPixel( index[ 0 ], index[ 1 ], pixelColor.rgb() );

      ++imageIterator;
      }

    return qimage; // The actual image region
    // The flipped image region - the logic for this needs to be outside of this function
    //return qimage.mirrored(false, true); // (horizontal, vertical)
    }

  /** Get a color QImage from a multi-channel ITK image. */
  template <typename TImage>
  QImage GetQImageColor_Vector( const TImage* const image,
    const itk::ImageRegion<2>& region, QImage::Format format )
    {
    QImage qimage( region.GetSize()[ 0 ], region.GetSize()[ 1 ], format );

    typedef itk::RegionOfInterestImageFilter< TImage, TImage >
      RegionOfInterestImageFilterType;
    typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter =
      RegionOfInterestImageFilterType::New();
    regionOfInterestImageFilter->SetRegionOfInterest( region );
    regionOfInterestImageFilter->SetInput( image );
    regionOfInterestImageFilter->Update();

    itk::ImageRegionIterator<TImage>
      imageIterator( regionOfInterestImageFilter->GetOutput(),
        regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion() );

    while( !imageIterator.IsAtEnd() )
      {
      typename TImage::PixelType pixel = imageIterator.Get();

      itk::Index<2> index = imageIterator.GetIndex();

      QColor pixelColor( pixel[ 0 ], pixel[ 1 ], pixel[ 2 ] );
      qimage.setPixel( index[ 0 ], index[ 1 ], pixelColor.rgb() );

      ++imageIterator;
      }

    return qimage;
    }

} // end namespace

#endif
