#ifndef PTXDETECTOR_H
#define PTXDETECTOR_H


#include "itkImage.h"
#include "itkDerivativeImageFilter.h"
#include "itkDivideImageFilter.h"
#include "itkAbsImageFilter.h"
#include "itkAddImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkThresholdImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkRGBPixel.h"
#include "itkImageRegionIterator.h"
#include "itkMedianImageFilter.h"


template <typename PixelType>
class PTXDetector{

public:
  typedef itk::Image< PixelType, 2 > ImageType2d;
  typedef typename ImageType2d::Pointer ImageType2dPointer;
  typedef itk::Image< PixelType, 3 > ImageType3d;

  typedef itk::RGBPixel< unsigned char > RGBPixelType;
  typedef typename itk::Image< RGBPixelType, 2 > RGBImageType;
  typedef typename RGBImageType::Pointer RGBImageTypePointer;


  static RGBImageTypePointer DetectMMode( ImageType2dPointer mmode){

    ImageType2d::SizeType size = mmode->GetLargestPossibleRegion().GetSize();
    typedef itk::RescaleIntensityImageFilter< ImageType2d, ImageType2d> RescaleFilter;
    RescaleFilter::Pointer rescale = RescaleFilter::New();
    rescale->SetInput( mmode);
    rescale->SetOutputMaximum( 1.0 );
    rescale->SetOutputMinimum( 0 );
    rescale->Update();


    typedef itk::DerivativeImageFilter< ImageType2d, ImageType2d > DerivativeFilter;
    DerivativeFilter::Pointer derivativeZ = DerivativeFilter::New();
    derivativeZ->SetDirection(0);
    derivativeZ->SetOrder( 2 );
    derivativeZ->SetInput( rescale->GetOutput() );


    typedef itk::AbsImageFilter<ImageType2d, ImageType2d> AbsFilter;
    AbsFilter::Pointer absZ = AbsFilter::New();
    absZ->SetInput( derivativeZ->GetOutput() );
  
    typedef itk::SmoothingRecursiveGaussianImageFilter< ImageType2d, ImageType2d> GaussianFilter;
    GaussianFilter::Pointer gaussianZ = GaussianFilter::New();
    gaussianZ->SetInput( absZ->GetOutput() );
    GaussianFilter::SigmaArrayType sigma;
    sigma[0] = 5;
    sigma[1] = 40;
    gaussianZ->SetSigmaArray( sigma );

    double thresholdValue = 0.1;
    typedef itk::ThresholdImageFilter<ImageType2d> ThresholdFilter;
    ThresholdFilter::Pointer threshold = ThresholdFilter::New();
    threshold->SetInput( gaussianZ->GetOutput() );
    threshold->ThresholdAbove( thresholdValue);
    threshold->SetOutsideValue( thresholdValue );
    threshold->Update();

    ImageType2d::Pointer ptx= threshold->GetOutput();


    typedef itk::CastImageFilter< ImageType2d, RGBImageType> RGBCastFilter;
    RGBCastFilter::Pointer rgbConvert = RGBCastFilter::New();
    rgbConvert->SetInput( mmode );
    rgbConvert->Update();
    RGBImageType::Pointer overlayImage = rgbConvert->GetOutput();
    itk::ImageRegionIterator<RGBImageType> overlayIterator( overlayImage, overlayImage->GetLargestPossibleRegion() );
    itk::ImageRegionIterator<ImageType2d> ptxIterator( ptx, ptx->GetLargestPossibleRegion() );
    double alpha = 0.3;
    while( !overlayIterator.IsAtEnd() )
      {
      RGBImageType::PixelType pixel = overlayIterator.Get();
      double p = 1.0 - ptxIterator.Get() / thresholdValue;
      pixel[0] = alpha * p * 255 + (1-alpha) * pixel[0];
      pixel[1] = (1-alpha) * pixel[1];
      pixel[2] = (1-alpha) * pixel[2];
      overlayIterator.Set( pixel );

      ++ptxIterator;
      ++overlayIterator;
      } 
    return overlayImage; 
  };

};
#endif
