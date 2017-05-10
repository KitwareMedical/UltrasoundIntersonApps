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



//This application estimates the width of the optic nerve from
//a B-mode ultrasound image.
//
//The inut image is expected to be oriented such that the optic nerve
//is towards the bottom of the image and depth is along the y-Axis.
//
//The computation involves two main steps:
// 1. Estimation of the eye orb location and minor and major axis length
// 2. Estimation of the optic nerve width
//Both steps include several substeps which results in many parameters
//that can be tuned if needed.
//
//
//
//EYE ESTIMATION:
//---------------
//(sub-steps indicate that a new image was created and the pipeline will use
//the image from the last step at the same granularity)
//
// A) Prepare moving Image:
//  1. Rescale the image to 0, 100
//  2. Adding a horizontal border
//  3. Gaussian smoothing
//  4. Binary Thresholding
//  4.1 Morphological closing
//  4.2 Adding a vertical border
//  4.3 Distance transfrom
//  4.4 Calculate inital center and radius from distance transform (Max)
//  4.4.1 Distance transform in X and Y seperately on region of interest
//   	  around slabs of the center
//  4.4.2 Calculate inital x and y radius from those distamnce transforms
//  5. Gaussian smoothing, threshold and rescale
//
// B) Prepare fixed image
//  1. Create ellipse ring image by subtract two ellipse with different
//     radii. The radii are based on the intial radius estimation above.
//  2. Gaussian smoothing, threshold, rescale
//
// C) Affine registration
//  1. Create a mask image that only measures mismatch in an ellipse region
//     macthing the create ellipse image, but not including left and right corners
//     of the eye (they are often black but sometimes white)
//  2. Affine registration centered on the fixed ellipse image
//  3. Compute minor and major axis by pushing the radii from the created ellipse
//     image through the computed transform
//
//
//
//OPTIC NERVE ESTIMATION:
//-----------------------
//(sub-steps indicate that a new image was created and the pipeline will use
//the image from the last step at the same granularity)
//
// A) Prepare moving image
//  1. Extract optic nerve region below the eye using the eye location and
//     size estimates
//  2. Gaussian smoothing
//  3. Rescale individual rows to 0 100
//  3.1 Binary threshold
//  3.2 Morphological opening
//  3.3 Add vertical border
//  3.4 Add small horizontal border
//  3.5 Distance transform
//  3.6 Calcuate inital optic nerve width and center
//  4. Scale rows 0, 100 on each side of the optice nerve center independently
//  5. Binary threhsold
//  5.1 Add vertica border
//  5.2 Add horizontal border
//  5.3 Distance transform
//  5.4 Refine intial estimates
//  6. Gaussian smoothing
//
// B) Prepare fixed image
//  1. Create a black and white image with two bars that
//     are an intial estimate of the width apart
//  2. Gauss smoothing
//
// C) Similarity transfrom registration
//  1. Create a mask that includes the two bars only
//  2. Similarity transfrom registration centered on the fixed bars image
//  3. Compute stem width by pushing intital width through the transform
//




//If DEBUG_IMAGES is defined several intermedate images are stored
//#define DEBUG_IMAGES

//If DEBUG_PRINT is defined print out intermediate messages
//#define DEBUG_PRINT

//If REPORT_TIMES is defined perform time measurments of individual steps
//and report them
//#define REPORT_TIMES
#ifdef REPORT_TIMES
#include "itkTimeProbe.h"
#endif



#include "itkImage.h"
#include "itkImageRegistrationMethodv4.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkMeanSquaresImageToImageMetricv4.h"
#include "itkLBFGSOptimizerv4.h"
#include "itkResampleImageFilter.h"
#include "itkApproximateSignedDistanceMapImageFilter.h"
#include "itkCastImageFilter.h"
#include <itkImageMaskSpatialObject.h>
#include <itkBinaryMorphologicalClosingImageFilter.h>
#include <itkBinaryMorphologicalOpeningImageFilter.h>
#include <itkGrayscaleMorphologicalOpeningImageFilter.h>
#include "itkBinaryBallStructuringElement.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkLabelMapToLabelImageFilter.h"
#include "itkLabelOverlayImageFilter.h"
#include "itkBinaryImageToLabelMapFilter.h"
#include "itkRGBPixel.h"
#include "itkRegionOfInterestImageFilter.h"
#include <itkSimilarity2DTransform.h>

#include "itkImageMaskSpatialObject.h"
#include "itkEllipseSpatialObject.h"
#include "itkSpatialObjectToImageFilter.h"

#include <algorithm>

#include "ImageIO.h"
#include "ITKFilterFunctions.h"
#include "itkImageRegionIterator.h"

class OpticNerveEstimator{

public:

  typedef  float  PixelType;
  typedef itk::Image< PixelType, 2 >  ImageType;
  typedef itk::Image<unsigned char, 2>  UnsignedCharImageType;

  typedef itk::CastImageFilter< ImageType, UnsignedCharImageType > CastFilter;
  typedef itk::ApproximateSignedDistanceMapImageFilter< UnsignedCharImageType, ImageType  > SignedDistanceFilter;

  typedef itk::BinaryBallStructuringElement<ImageType::PixelType, ImageType::ImageDimension> StructuringElementType;
  typedef itk::BinaryMorphologicalClosingImageFilter<ImageType, ImageType, StructuringElementType> ClosingFilter;
  typedef itk::BinaryMorphologicalOpeningImageFilter<ImageType, ImageType, StructuringElementType> OpeningFilter;
  typedef itk::GrayscaleMorphologicalOpeningImageFilter<ImageType, ImageType, StructuringElementType> GrayOpeningFilter;

  typedef itk::MinimumMaximumImageCalculator <ImageType> ImageCalculatorFilterType;

  //typedef itk::ExtractImageFilter< ImageType, ImageType > ExtractFilter;
  typedef itk::RegionOfInterestImageFilter< ImageType, ImageType > ExtractFilter;
  typedef itk::RegionOfInterestImageFilter< UnsignedCharImageType, UnsignedCharImageType > ExtractFilter2;


  //Overlay
  typedef itk::RGBPixel<unsigned char> RGBPixelType;
  typedef itk::Image<RGBPixelType> RGBImageType;
  typedef itk::LabelOverlayImageFilter<ImageType, UnsignedCharImageType, RGBImageType> LabelOverlayImageFilterType;
  typedef itk::BinaryImageToLabelMapFilter<UnsignedCharImageType> BinaryImageToLabelMapFilterType;
  typedef itk::LabelMapToLabelImageFilter<BinaryImageToLabelMapFilterType::OutputImageType, UnsignedCharImageType> LabelMapToLabelImageFilterType;


  //registration
  //typedef itk::GradientDescentOptimizer       OptimizerType;
  //typedef itk::ConjugateGradientOptimizer       OptimizerType;
  typedef itk::LBFGSOptimizerv4       OptimizerType;

  typedef itk::MeanSquaresImageToImageMetricv4< ImageType, ImageType >  MetricType;
  typedef itk::LinearInterpolateImageFunction< ImageType, double >    InterpolatorType;
  typedef itk::ImageRegistrationMethodv4< ImageType, ImageType >    RegistrationType;

  typedef itk::Similarity2DTransform< double >     SimilarityTransformType;
  typedef itk::AffineTransform< double, 2 >     AffineTransformType;

  typedef itk::ResampleImageFilter< ImageType, ImageType >    ResampleFilterType;
  //Mask image spatical object
  typedef itk::ImageMaskSpatialObject< 2 >   MaskType;

  //Elipse object
  typedef itk::EllipseSpatialObject< 2 >   EllipseType;
  typedef itk::SpatialObjectToImageFilter< EllipseType, ImageType >   SpatialObjectToImageFilterType;
  typedef EllipseType::TransformType EllipseTransformType;


  //Storage for eye and stem location and sizes
  struct Eye{
    ImageType::IndexType initialCenterIndex;
    ImageType::PointType initialCenter;
    ImageType::IndexType centerIndex;
    ImageType::PointType center;
    double initialRadius = -1;
    double minor = -1;
    double major = -1;

    double initialRadiusX = -1;
    double initialRadiusY = -1;

    ImageType::Pointer aligned;
  };



  struct Stem{
    ImageType::IndexType initialCenterIndex;
    ImageType::PointType initialCenter;
    ImageType::IndexType centerIndex;
    ImageType::PointType center;
    double initialWidth = -1;
    double width = -1;

    ImageType::Pointer aligned;
    ImageType::RegionType originalImageRegion;
  };








  bool Fit( ImageType::Pointer origImage, bool overlay = false,
            bool intermediateOverlays = false, std::string prefix = "");


   Eye GetEye(){
     return eye;
   };

   Stem GetStem(){
     return stem;
   };

   RGBImageType::Pointer GetOverlay(){
      return overlayImage;
   };


private:
#ifdef REPORT_TIMES
  itk::TimeProbe clockEyeA;
  itk::TimeProbe clockEyeB;
  itk::TimeProbe clockEyeC1;
  itk::TimeProbe clockEyeC2;
  itk::TimeProbe clockEyeC3;


  itk::TimeProbe clockStemA;
  itk::TimeProbe clockStemB;
  itk::TimeProbe clockStemC1;
  itk::TimeProbe clockStemC2;
  itk::TimeProbe clockStemC3;
#endif

  Eye eye;
  Stem stem;
  RGBImageType::Pointer overlayImage;

  //Helper function
  std::string catStrings(std::string s1, std::string s2);


  //Create ellipse image
  ImageType::Pointer CreateEllipseImage( ImageType::SpacingType spacing,
		                         ImageType::SizeType size,
				         ImageType::PointType origin,
				         ImageType::PointType center,
				         double r1, double r2,
                                         double outside = 100,
                                         double inside = 0
                                       );


  //Fit an ellipse to an eye ultrasound image in three main steps
  // A) Prepare moving Image
  // B) Prepare fixed image
  // C) Affine registration
  //
  //For a detailed descritpion and overview of the whole pipleine
  //see the top of this file
  bool FitEye( ImageType::Pointer inputImage, const std::string &prefix,
               bool alignEllipse);





  //Fit two bars to an ultrasound image based on eye location and size
  // A) Prepare moving Image
  // B) Prepare fixed image
  // C) Similarity registration
  //
  //For a detailed descritpion and overview of the whole pipleine
  //see the top of this file
  bool FitStem( ImageType::Pointer inputImage, Eye &eye,
                const std::string &prefix, bool alignStem);




};
