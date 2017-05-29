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

#include "OpticNerveEstimator.hxx"


OpticNerveEstimator::Status
OpticNerveEstimator::Fit( OpticNerveEstimator::ImageType::Pointer origImage,
  bool overlay,
  std::string prefix )
{
  bool fitEyeSucces = FitEye( origImage, overlay, prefix );
  if( !fitEyeSucces )
    {
    return ESTIMATION_FAIL_EYE;
    }

  //Setup nerve region
  ImageType::RegionType imageRegion = origImage->GetLargestPossibleRegion();
  ImageType::SizeType imageSize = imageRegion.GetSize();

  ImageType::IndexType desiredStart;
  desiredStart[ 0 ] = eye.center[ 0 ] - algParams.nerveXRegionFactor * eye.radiusX;
  desiredStart[ 1 ] = eye.center[ 1 ] + algParams.nerveYRegionFactor * eye.radiusY;

  ImageType::SizeType desiredSize;
  desiredSize[ 0 ] = 2 * algParams.nerveXRegionFactor * eye.radiusX;
  desiredSize[ 1 ] = algParams.nerveYSizeFactor * eye.radiusY;

  if( desiredStart[ 1 ] > imageSize[ 1 ] )
    {
    return ESTIMATION_FAIL_NERVE;
    }
  if( desiredStart[ 1 ] + desiredSize[ 1 ] > imageSize[ 1 ] )
    {
    desiredSize[ 1 ] = imageSize[ 1 ] - desiredStart[ 1 ];
    }

  if( desiredStart[ 0 ] < 0 )
    {
    desiredStart[ 0 ] = 0;
    }
  if( desiredStart[ 0 ] + desiredSize[ 0 ] > imageSize[ 0 ] )
    {
    desiredSize[ 0 ] = imageSize[ 0 ] - desiredStart[ 0 ];
    }


  ImageType::RegionType desiredRegion( desiredStart, desiredSize );

  bool fitNerveSucces = FitNerve( origImage, desiredRegion, overlay, prefix );
  if( !fitNerveSucces )
    {
    return ESTIMATION_FAIL_NERVE;
    }


#ifdef REPORT_TIMES
  std::cout << "Times" << std::endl;
  std::cout << "Eye A:   " << clockEyeA.GetMean() << std::endl;
  std::cout << "Eye B:   " << clockEyeB.GetMean() << std::endl;
  std::cout << "Eye C1:  " << clockEyeC1.GetMean() << std::endl;
  std::cout << "Eye C2:  " << clockEyeC2.GetMean() << std::endl;
  std::cout << "Eye C3:  " << clockEyeC3.GetMean() << std::endl;
  std::cout << std::endl;
  std::cout << "Nerve A:  " << clockNerveA.GetMean() << std::endl;
  std::cout << "Nerve B:  " << clockNerveB.GetMean() << std::endl;
  std::cout << "Nerve C1: " << clockNerveC1.GetMean() << std::endl;
  std::cout << "Nerve C2: " << clockNerveC2.GetMean() << std::endl;
#endif


  return ESTIMATION_SUCCESS;
}

//Helper function
std::string OpticNerveEstimator::catStrings( std::string s1, std::string s2 )
{
  std::stringstream out;
  out << s1 << s2;
  return out.str();
}

//Create ellipse image
OpticNerveEstimator::ImageType::Pointer
OpticNerveEstimator::CreateEllipseImage( ImageType::SpacingType spacing,
  ImageType::SizeType size,
  ImageType::PointType origin,
  ImageType::DirectionType direction,
  ImageType::PointType center,
  double r1, double r2,
  double outside, double inside )
{
  SpatialObjectToImageFilterType::Pointer imageFilter =
    SpatialObjectToImageFilterType::New();

  //origin[0] -= 0.5 * size[0] * spacing[0];
  //origin[1] -= 0.5 * size[1] * spacing[1];
  //size[0] *= 2;
  //size[1] *= 2;
  ImageType::SizeType smallSize = size;
  smallSize[ 0 ] = 100;
  smallSize[ 1 ] = 100;
  ImageType::SpacingType smallSpacing = spacing;
  smallSpacing[ 0 ] = ( smallSpacing[ 0 ] / smallSize[ 0 ] ) * size[ 0 ];
  smallSpacing[ 1 ] = ( smallSpacing[ 1 ] / smallSize[ 1 ] ) * size[ 1 ];

  imageFilter->SetSize( smallSize );
  imageFilter->SetOrigin( origin );
  imageFilter->SetSpacing( smallSpacing );
  //imageFilter->SetDirection( direction );

  EllipseType::Pointer ellipse = EllipseType::New();
  EllipseType::ArrayType radiusArray;
  radiusArray[ 0 ] = r1;
  radiusArray[ 1 ] = r2;
  ellipse->SetRadius( radiusArray );

  EllipseTransformType::Pointer transform = EllipseTransformType::New();
  transform->SetIdentity();
  EllipseTransformType::OutputVectorType  translation;
  translation[ 0 ] = center[ 0 ];
  translation[ 1 ] = center[ 1 ];
  transform->Translate( translation, false );
  ellipse->SetObjectToParentTransform( transform );

  imageFilter->SetInput( ellipse );
  ellipse->SetDefaultInsideValue( inside );
  ellipse->SetDefaultOutsideValue( outside );
  imageFilter->SetUseObjectValue( true );
  imageFilter->SetOutsideValue( outside );
  imageFilter->Update();

  ResampleFilterType::Pointer resampler = ResampleFilterType::New();
  resampler->SetInput( imageFilter->GetOutput() );
  resampler->SetSize( size );
  resampler->SetOutputOrigin( origin );
  resampler->SetOutputSpacing( spacing );
  resampler->SetOutputDirection( direction );
  resampler->SetDefaultPixelValue( 0 );
  resampler->Update();

  return resampler->GetOutput();

  //return imageFilter->GetOutput();
};

//Fit an ellipse to an eye ultrasound image in three main steps
// A) Prepare moving Image
// B) Prepare fixed image
// C) Affine registration
//
//For a detailed descritpion and overview of the whole pipleine
//see the top of this file
bool
OpticNerveEstimator::FitEye( OpticNerveEstimator::ImageType::Pointer inputImage,
  bool alignEllipse, const std::string &prefix )
{
#ifdef DEBUG_PRINT
  std::cout << "--- Fitting Eye ---" << std::endl << std::endl;
#endif


  ////
  //A. Prepare fixed image
  ///

#ifdef REPORT_TIMESS
  clockEyeA.Start();
#endif

  //-- Steps 1 to 3
  //   1. Rescale the image to 0, 100
  //   2. Adding a horizontal border
  //   3. Gaussian smoothing

  ImageType::Pointer image = ITKFilterFunctions<ImageType>::Rescale( inputImage, 0, 100 );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( image, catStrings( prefix, "-eye-input.tif" ) );
#endif
  ImageType::SpacingType imageSpacing = image->GetSpacing();
  ImageType::RegionType imageRegion = image->GetLargestPossibleRegion();
  ImageType::SizeType imageSize = imageRegion.GetSize();
  ImageType::PointType imageOrigin = image->GetOrigin();
  ImageType::DirectionType imageDirection = image->GetDirection();

#ifdef DEBUG_PRINT
  std::cout << "Origin, spacing, size input image" << std::endl;
  std::cout << imageOrigin << std::endl;
  std::cout << imageSpacing << std::endl;
  std::cout << imageSize << std::endl;
#endif

  ITKFilterFunctions<ImageType>::SigmaArrayType sigma;
  sigma[ 0 ] = algParams.eyeInitialBlurFactor * imageSpacing[ 0 ];
  sigma[ 1 ] = algParams.eyeInitialBlurFactor * imageSpacing[ 1 ];

  ITKFilterFunctions<ImageType>::AddHorizontalBorder( image,
    imageSize[ 1 ] * algParams.eyeHorizontalBorderFactor );
  image = ITKFilterFunctions<ImageType>::GaussSmooth( image, sigma );

  //-- Step 4
  //   Binary Thresholding

  image = ITKFilterFunctions<ImageType>::BinaryThreshold( image, -1,
    algParams.eyeInitialBinaryThreshold, 0, 100 );

//-- Steps 4.1 through 4.4
//   4.1 Morphological closing
//   4.2 Adding a vertical border
//   4.3 Distance transfrom
//   4.4 Calculate inital center and radius from distance transform (Max)

  StructuringElementType structuringElement;
  structuringElement.SetRadius(
    algParams.eyeClosingRadiusFactor * std::min( imageSize[ 0 ], imageSize[ 1 ] )
  );
  structuringElement.CreateStructuringElement();
  ClosingFilter::Pointer closingFilter = ClosingFilter::New();
  closingFilter->SetInput( image );
  closingFilter->SetKernel( structuringElement );
  closingFilter->SetForegroundValue( 100.0 );
  closingFilter->Update();
  image = closingFilter->GetOutput();

  CastFilter::Pointer castFilter = CastFilter::New();
  castFilter->SetInput( image );
  castFilter->Update();
  UnsignedCharImageType::Pointer  sdImage = castFilter->GetOutput();

  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorder( sdImage,
    algParams.eyeVerticalBorderFactor * imageSize[ 0 ] );

  SignedDistanceFilter::Pointer signedDistanceFilter = SignedDistanceFilter::New();
  signedDistanceFilter->SetInput( sdImage );
  signedDistanceFilter->SetInsideValue( 100 );
  signedDistanceFilter->SetOutsideValue( 0 );
  signedDistanceFilter->Update();
  ImageType::Pointer imageDistance = signedDistanceFilter->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( imageDistance, catStrings( prefix, "-eye-distance.tif" ) );
#endif

  //Compute max of distance transfrom
  ImageCalculatorFilterType::Pointer imageCalculatorFilter = ImageCalculatorFilterType::New();
  imageCalculatorFilter->SetImage( imageDistance );
  imageCalculatorFilter->Compute();

  eye.initialRadius = imageCalculatorFilter->GetMaximum();
  eye.initialCenterIndex = imageCalculatorFilter->GetIndexOfMaximum();
  image->TransformIndexToPhysicalPoint( eye.initialCenterIndex, eye.initialCenter );

#ifdef DEBUG_PRINT
  std::cout << "Eye inital center index: " << eye.initialCenterIndex << std::endl;
  std::cout << "Eye inital center: " << eye.initialCenter << std::endl;
  std::cout << "Eye initial radius: " << eye.initialRadius << std::endl;
#endif
  if( eye.initialRadius <= 0 )
    {
    return false;
    }

    //-- Steps 4.4.1 through 4.4.2
    //   4.4.1 Distance transform in X and Y seperately on region of interest
    //        around slabs of the center
    //  4.4.2 Calculate inital x and y radius from those distamnce transforms

    //Compute vertical distance to eye border
  CastFilter::Pointer castFilter2 = CastFilter::New();
  castFilter2->SetInput( image );
  castFilter2->Update();
  UnsignedCharImageType::Pointer  sdImage2 = castFilter2->GetOutput();
  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorder( sdImage2, 2 );

  ImageType::SizeType yRegionSize;
  yRegionSize[ 0 ] = algParams.eyeYSlab;
  yRegionSize[ 1 ] = imageSize[ 1 ];
  ImageType::IndexType yRegionIndex;
  yRegionIndex[ 0 ] = eye.initialCenterIndex[ 0 ] - algParams.eyeYSlab / 2;
  yRegionIndex[ 1 ] = 0;
  ImageType::RegionType yRegion( yRegionIndex, yRegionSize );

  ExtractFilter2::Pointer extractFilterY = ExtractFilter2::New();
  extractFilterY->SetRegionOfInterest( yRegion );
  extractFilterY->SetInput( sdImage2 );
  extractFilterY->Update();

  SignedDistanceFilter::Pointer signedDistanceY = SignedDistanceFilter::New();
  signedDistanceY->SetInput( extractFilterY->GetOutput() );
  signedDistanceY->SetInsideValue( 100 );
  signedDistanceY->SetOutsideValue( 0 );
  //signedDistanceY->GetOutput()->SetRequestedRegion( yRegion );
  signedDistanceY->Update();
  ImageType::Pointer imageDistanceY = signedDistanceY->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( imageDistanceY, catStrings( prefix, "-eye-ydistance.tif" ) );
#endif

  ImageCalculatorFilterType::Pointer imageCalculatorY = ImageCalculatorFilterType::New();
  imageCalculatorY->SetImage( imageDistanceY );
  imageCalculatorY->Compute();

  eye.initialRadiusY = imageCalculatorY->GetMaximum();

  eye.initialCenterIndex[ 1 ] = imageCalculatorY->GetIndexOfMaximum()[ 1 ];
#ifdef DEBUG_PRINT
  std::cout << "Eye initial radiusY: " << eye.initialRadiusY << std::endl;
#endif

  //Compute horizontal distance to eye border
  ImageType::SizeType xRegionSize;
  xRegionSize[ 0 ] = imageSize[ 0 ];
  xRegionSize[ 1 ] = algParams.eyeXSlab;
  ImageType::IndexType xRegionIndex;
  xRegionIndex[ 0 ] = 0;
  xRegionIndex[ 1 ] = eye.initialCenterIndex[ 1 ] - algParams.eyeXSlab / 2;
  ImageType::RegionType xRegion( xRegionIndex, xRegionSize );

  ExtractFilter2::Pointer extractFilterX = ExtractFilter2::New();
  extractFilterX->SetRegionOfInterest( xRegion );
  extractFilterX->SetInput( sdImage2 );
  extractFilterX->Update();

  SignedDistanceFilter::Pointer signedDistanceX = SignedDistanceFilter::New();
  signedDistanceX->SetInput( extractFilterX->GetOutput() );
  signedDistanceX->SetInsideValue( 100 );
  signedDistanceX->SetOutsideValue( 0 );
  //signedDistanceX->GetOutput()->SetRequestedRegion( xRegion );
  signedDistanceX->Update();
  ImageType::Pointer imageDistanceX = signedDistanceX->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( imageDistanceX, catStrings( prefix, "-eye-xdistance.tif" ) );
#endif

  ImageCalculatorFilterType::Pointer imageCalculatorX = ImageCalculatorFilterType::New();
  imageCalculatorX->SetImage( imageDistanceX );
  imageCalculatorX->Compute();

  eye.initialRadiusX = imageCalculatorX->GetMaximum();

  eye.initialCenterIndex[ 0 ] = imageCalculatorX->GetIndexOfMaximum()[ 0 ];
#ifdef DEBUG_PRINT
  std::cout << "Eye initial radiusX: " << eye.initialRadiusX << std::endl;
#endif

  image->TransformIndexToPhysicalPoint( eye.initialCenterIndex, eye.initialCenter );
#ifdef DEBUG_PRINT
  std::cout << "Eye inital center index: " << eye.initialCenterIndex << std::endl;
#endif

  //--Step 5
  //  Gaussian smoothing, threshold and rescale

  sigma[ 0 ] = 10 * imageSpacing[ 0 ];
  sigma[ 1 ] = 10 * imageSpacing[ 1 ];
  ImageType::Pointer imageSmooth = ITKFilterFunctions<ImageType>::GaussSmooth( image, sigma );
  imageSmooth = ITKFilterFunctions<ImageType>::ThresholdAbove( imageSmooth,
    algParams.eyeThreshold, 100 );
  imageSmooth = ITKFilterFunctions<ImageType>::Rescale( imageSmooth, 0, 100 );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( imageSmooth, catStrings( prefix, "-eye-smooth.tif" ) );
#endif

#ifdef REPORT_TIMES
  clockEyeA.Stop();
#endif


  ////
  //B. Prepare fixed image
  ////

#ifdef REPORT_TIMES
  clockEyeB.Start();
#endif

  //-- Steps 1 through 2
  //   1. Create ellipse ring image by subtract two ellipse with different
  //      radii. The radii are based on the intial radius estimation above.
  //   2. Gaussian smoothing, threshold, rescale

  double outside = 100;
  //intial guess of radiusY axis
  double r1 = std::min( 1.2 * eye.initialRadiusY, eye.initialRadiusX );
  //inital guess of radiusX axis
  double r2 = eye.initialRadiusY;
  //width of the ellipse ring rf*r1, rf*r2
  ImageType::Pointer e1 = CreateEllipseImage( imageSpacing, imageSize, imageOrigin, imageDirection,
    eye.initialCenter, r1, r2, outside );
  ImageType::Pointer e2 = CreateEllipseImage( imageSpacing, imageSize, imageOrigin, imageDirection,
    eye.initialCenter, r1 * algParams.eyeRingFactor,
    r2 * algParams.eyeRingFactor,
    outside );

  ImageType::Pointer ellipse = ITKFilterFunctions<ImageType>::Subtract( e1, e2 );

  sigma[ 0 ] = algParams.eyeInitialBlurFactor * imageSpacing[ 0 ];
  sigma[ 1 ] = algParams.eyeInitialBlurFactor * imageSpacing[ 1 ];
  ellipse = ITKFilterFunctions<ImageType>::GaussSmooth( ellipse, sigma );
  ellipse = ITKFilterFunctions<ImageType>::ThresholdAbove( ellipse,
    algParams.eyeThreshold, 100 );
  ellipse = ITKFilterFunctions<ImageType>::Rescale( ellipse, 0, 100 );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( ellipse, catStrings( prefix, "-eye-moving.tif" ) );
#endif

#ifdef DEBUG_PRINT
  std::cout << "Origin, spacing, size ellipse image" << std::endl;
  std::cout << ellipse->GetOrigin() << std::endl;
  std::cout << ellipse->GetSpacing() << std::endl;
  std::cout << ellipse->GetLargestPossibleRegion().GetSize() << std::endl;
#endif

#ifdef REPORT_TIMES
  clockEyeB.Stop();
#endif


  ////
  //C. Affine registration
  ////

#ifdef REPORT_TIMES
  clockEyeC1.Start();
#endif

  //-- Step 1
  //   Create a mask image that only measure mismatch in an ellipse region
  //   macthing the create ellipse image, but not including left and right corners
  //   of the eye (they are often black but sometimes white)

  ImageType::Pointer ellipseMask = CreateEllipseImage( imageSpacing, imageSize,
    imageOrigin, imageDirection,
    eye.initialCenter,
    r1 * ( algParams.eyeRingFactor + 1 ) / 2,
    r2 * ( algParams.eyeRingFactor + 1 ) / 2,
    0, 100 );
  //remove left and right corners from mask
  int xlim_l = std::max( 0,
    ( int )( eye.initialCenterIndex[ 0 ] - algParams.eyeMaskCornerXFactor * r1 * imageSpacing[ 0 ] ) );
  int xlim_r = std::min( ( int )imageSize[ 0 ],
    ( int )( eye.initialCenterIndex[ 0 ] + algParams.eyeMaskCornerXFactor * r1 * imageSpacing[ 0 ] ) );

  int ylim_b = std::max( 0,
    ( int )( eye.initialCenterIndex[ 1 ] - algParams.eyeMaskCornerYFactor * r2 * imageSpacing[ 1 ] ) );
  int ylim_t = std::min( ( int )imageSize[ 1 ],
    ( int )( eye.initialCenterIndex[ 1 ] + algParams.eyeMaskCornerYFactor * r2 * imageSpacing[ 1 ] ) );

  for( int i = 0; i < xlim_l; i++ )
    {
    ImageType::IndexType index;
    index[ 0 ] = i;
    for( int j = ylim_b; j < ylim_t; j++ )
      {
      index[ 1 ] = j;
      ellipseMask->SetPixel( index, 0 );
      }
    }
  for( int i = xlim_r; i < imageSize[ 0 ]; i++ )
    {
    ImageType::IndexType index;
    index[ 0 ] = i;
    for( int j = ylim_b; j < ylim_t; j++ )
      {
      index[ 1 ] = j;
      ellipseMask->SetPixel( index, 0 );
      }
    }

#ifdef REPORT_TIMES
  clockEyeC1.Stop();
#endif


#ifdef REPORT_TIMES
  clockEyeC2.Start();
#endif

  //-- Step 2
  //   Affine registration centered on the fixed ellipse image

  AffineTransformType::Pointer transform = AffineTransformType::New();
  transform->SetCenter( eye.initialCenter );


  MetricType::Pointer         metric = MetricType::New();
  OptimizerType::Pointer      optimizer = OptimizerType::New();
  InterpolatorType::Pointer   movingInterpolator = InterpolatorType::New();
  InterpolatorType::Pointer   fixedInterpolator = InterpolatorType::New();
  RegistrationType::Pointer   registration = RegistrationType::New();


  optimizer->SetGradientConvergenceTolerance( 0.0000001 );
  optimizer->SetLineSearchAccuracy( 0.5 );
  optimizer->SetDefaultStepLength( 0.00001 );
#ifdef DEBUG_PRINT
  optimizer->TraceOn();
#endif
  optimizer->SetMaximumNumberOfFunctionEvaluations( 20000 );

  metric->SetMovingInterpolator( movingInterpolator );
  metric->SetFixedInterpolator( fixedInterpolator );

  CastFilter::Pointer castFilter3 = CastFilter::New();
  castFilter3->SetInput( ellipseMask );
  castFilter3->Update();
  MaskType::Pointer  spatialObjectMask = MaskType::New();
  spatialObjectMask->SetImage( castFilter3->GetOutput() );
  metric->SetFixedImageMask( spatialObjectMask );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( ellipseMask, catStrings( prefix, "-eye-mask.tif" ) );
#endif

  registration->SetMetric( metric );
  registration->SetOptimizer( optimizer );

  registration->SetMovingImage( imageSmooth );
  registration->SetFixedImage( ellipse );

  registration->SetInitialTransform( transform );

  RegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
  shrinkFactorsPerLevel.SetSize( 1 );
  shrinkFactorsPerLevel[ 0 ] = std::max( 1,
    ( int )( std::min( imageSize[ 0 ], imageSize[ 1 ] ) / algParams.eyeRegistrationSize ) );
  //shrinkFactorsPerLevel[1] = 4;
  //shrinkFactorsPerLevel[2] = 2;
  //shrinkFactorsPerLevel[3] = 1;

  RegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
  smoothingSigmasPerLevel.SetSize( 1 );
  smoothingSigmasPerLevel[ 0 ] = 0;
  //smoothingSigmasPerLevel[1] = 0;
  //smoothingSigmasPerLevel[2] = 0.5;
  //smoothingSigmasPerLevel[3] = 0;
  //smoothingSigmasPerLevel[0] = 0;

  registration->SetNumberOfLevels( 1 );
  registration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
  registration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );

  //Do registration
  try
    {
    registration->SetNumberOfThreads( 1 );
    registration->Update();
    }
  catch( itk::ExceptionObject & err )
    {
#ifdef DEBUG_PRINT
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    //return EXIT_FAILURE;
#endif
    }

#ifdef DEBUG_PRINT
  const double bestValue = optimizer->GetValue();
  std::cout << "Result = " << std::endl;
  std::cout << " Metric value  = " << bestValue << std::endl;

  std::cout << "Optimized transform paramters:" << std::endl;
  std::cout << registration->GetTransform()->GetParameters() << std::endl;
  std::cout << transform->GetCenter() << std::endl;
#endif

#ifdef REPORT_TIMES
  clockEyeC2.Stop();
#endif

#ifdef REPORT_TIMES
  clockEyeC3.Start();
#endif

  //Created registered ellipse image
  if( alignEllipse )
    {
    AffineTransformType::Pointer inverse = AffineTransformType::New();
    transform->GetInverse( inverse );

    ResampleFilterType::Pointer resampler = ResampleFilterType::New();
    resampler->SetInput( ellipse );
    resampler->SetTransform( inverse );
    resampler->SetSize( imageSize );
    resampler->SetOutputOrigin( image->GetOrigin() );
    resampler->SetOutputSpacing( imageSpacing );
    resampler->SetOutputDirection( image->GetDirection() );
    resampler->SetDefaultPixelValue( 0 );
    resampler->Update();
    ImageType::Pointer moved = resampler->GetOutput();

    eye.aligned = moved;

#ifdef DEBUG_IMAGES
    ImageIO<ImageType>::WriteImage( moved, catStrings( prefix, "-eye-registred.tif" ) );
#endif

    }

  //-- Step 3
  //   Compute radiusX and radiusY axis by pushing the radii from the created ellipse
  //   image through the computed transform

  AffineTransformType::InputPointType tCenter;
  tCenter[ 0 ] = eye.initialCenter[ 0 ];
  tCenter[ 1 ] = eye.initialCenter[ 1 ];

  AffineTransformType::InputVectorType tX;
  tX[ 0 ] = r1;
  tX[ 1 ] = 0;

  AffineTransformType::InputVectorType tY;
  tY[ 0 ] = 0;
  tY[ 1 ] = r2;

  eye.center = transform->TransformPoint( tCenter );
  image->TransformPhysicalPointToIndex( eye.center, eye.centerIndex );

  AffineTransformType::OutputVectorType tXO = transform->TransformVector( tX, tCenter );
  AffineTransformType::OutputVectorType tYO = transform->TransformVector( tY, tCenter );

  eye.radiusX = sqrt( tXO[ 0 ] * tXO[ 0 ] + tXO[ 1 ] * tXO[ 1 ] );
  eye.radiusY = sqrt( tYO[ 0 ] * tYO[ 0 ] + tYO[ 1 ] * tYO[ 1 ] );

  //TODO limit transform
  //if(eye.radiusY < eye.radiusX){
  //  std::swap(eye.radiusX, eye.radiusY);
  //}

#ifdef DEBUG_PRINT
  std::cout << "Eye center: " << eye.centerIndex << std::endl;
  std::cout << "Eye radiusX: " << eye.radiusX << std::endl;
  std::cout << "Eye radiusY: " << eye.radiusY << std::endl;

  std::cout << "--- Done Fitting Eye ---" << std::endl << std::endl;
#endif

#ifdef REPORT_TIMES
  clockEyeC3.Stop();
#endif

  return true;
};

  //Fit two bars to an ultrasound image based on eye location and size
  // A) Prepare moving Image
  // B) Prepare fixed image
  // C) Similarity registration
  //
  //For a detailed descritpion and overview of the whole pipleine
  //see the top of this file
bool
OpticNerveEstimator::FitNerve(
  OpticNerveEstimator::ImageType::Pointer inputImage,
  OpticNerveEstimator::ImageType::RegionType &desiredRegion,
  bool alignNerve,
  const std::string &prefix )
{
#ifdef DEBUG_PRINT
  std::cout << "--- Fit nerve ---" << std::endl << std::endl;
#endif


  ////
  //A) Prepare moving image
  ////

#ifdef REPORT_TIMES
  clockNerveA.Start();
#endif

  ImageType::SpacingType imageSpacing = inputImage->GetSpacing();
  ImageType::RegionType imageRegion = inputImage->GetLargestPossibleRegion();
  ImageType::SizeType imageSize = imageRegion.GetSize();
  ImageType::PointType imageOrigin = inputImage->GetOrigin();

  //-- Step 1
  //Estract nerve region
  nerve.originalImageRegion = desiredRegion;
  ExtractFilter::Pointer extractFilter = ExtractFilter::New();
  //extractFilter->SetExtractionRegion(desiredRegion);
  extractFilter->SetRegionOfInterest( desiredRegion );
  extractFilter->SetInput( inputImage );
  //extractFilter->SetDirectionCollapseToIdentity();
  extractFilter->Update();
  ImageType::Pointer nerveImageOrig = extractFilter->GetOutput();

  ImageType::RegionType nerveRegion = nerveImageOrig->GetLargestPossibleRegion();
  ImageType::SizeType nerveSize = nerveRegion.GetSize();
  ImageType::PointType nerveOrigin = nerveImageOrig->GetOrigin();
  ImageType::SpacingType nerveSpacing = nerveImageOrig->GetSpacing();
  ImageType::DirectionType nerveDirection = nerveImageOrig->GetDirection();
  ImageType::IndexType nerveIndex = nerveRegion.GetIndex();

#ifdef DEBUG_PRINT
  std::cout << "Origin, spacing, size and index of nerve image" << std::endl;
  std::cout << nerveOrigin << std::endl;
  std::cout << nerveSpacing << std::endl;
  std::cout << nerveSize << std::endl;
  std::cout << nerveIndex << std::endl;
#endif

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( nerveImageOrig, catStrings( prefix, "-nerve.tif" ) );
#endif

  //-- Step 2 through 3
  //   2. Gaussian smoothing
  //   3. Rescale individual rows to 0 100

  ITKFilterFunctions<ImageType>::SigmaArrayType sigma;
  sigma[ 0 ] = algParams.nerveInitialSmoothXFactor * nerveSpacing[ 0 ];
  sigma[ 1 ] = algParams.nerveInitialSmoothYFactor * nerveSpacing[ 1 ];
  //sigma[1] = nerveSize[1]/12.0 * nerveSpacing[1];
  ImageType::Pointer nerveImage = ITKFilterFunctions<ImageType>::GaussSmooth( nerveImageOrig, sigma );

  //Rescale indiviudal rows
  ITKFilterFunctions<ImageType>::RescaleRows( nerveImage );

  nerveImage = ITKFilterFunctions<ImageType>::Rescale( nerveImage, 0, 100 );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( nerveImage, catStrings( prefix, "-nerve-smooth.tif" ) );
#endif

  //-- Step 3.1 through 3.6
  //   3.1 Binary threshold
  //   3.2 Morphological opening
  //   3.3 Add vertical border
  //   3.4 Add small horizontal border
  //   3.5 Distance transform
  //   3.6 Calcuate inital optic nerve width and center

  ImageType::Pointer nerveImageB =
    ITKFilterFunctions<ImageType>::BinaryThreshold( nerveImage, -1,
      algParams.nerveInitialThreshold, 0, 100 );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( nerveImageB, catStrings( prefix, "-nerve-sd-thres.tif" ) );
#endif

  StructuringElementType structuringElement;
  structuringElement.SetRadius( std::min( imageSize[ 0 ], imageSize[ 1 ] ) * algParams.nerveOpeningRadiusFactor );
  structuringElement.CreateStructuringElement();
  OpeningFilter::Pointer openingFilter = OpeningFilter::New();
  openingFilter->SetInput( nerveImageB );
  openingFilter->SetKernel( structuringElement );
  openingFilter->SetForegroundValue( 100.0 );
  openingFilter->Update();
  nerveImageB = openingFilter->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( nerveImageB, catStrings( prefix, "-nerve-morpho.tif" ) );
#endif

  CastFilter::Pointer nerveCastFilter = CastFilter::New();
  nerveCastFilter->SetInput( nerveImageB );
  nerveCastFilter->Update();
  UnsignedCharImageType::Pointer nerveImage2 = nerveCastFilter->GetOutput();

  //Compute left and right border
  int leftBorder = 5;
  ImageType::IndexType borderIndex;
  borderIndex[ 1 ] = nerveSize[ 1 ] / 3;
  for( int i = 5; i < nerveSize[ 0 ]; i++ )
    {
    borderIndex[ 0 ] = i;
    if( nerveImage->GetPixel( borderIndex ) < algParams.nerveBorderThreshold )
      {
      leftBorder++;
      }
    else
      {
      break;
      }
    }
  int rightBorder = 5;
  for( int i = nerveSize[ 0 ] - 5; i >= 0; i-- )
    {
    borderIndex[ 0 ] = i;
    if( nerveImage->GetPixel( borderIndex ) < algParams.nerveBorderThreshold )
      {
      rightBorder++;
      }
    else
      {
      break;
      }
    }
#ifdef DEBUG_PRINT
  std::cout << "Borders: " << leftBorder << " | " << rightBorder << std::endl;
#endif

  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorderLeft( nerveImage2, leftBorder );
  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorderRight( nerveImage2, rightBorder );

  ITKFilterFunctions<UnsignedCharImageType>::AddHorizontalBorder( nerveImage2, algParams.nerveHorizontalBoder );

  SignedDistanceFilter::Pointer nerveDistanceFilter = SignedDistanceFilter::New();
  nerveDistanceFilter->SetInput( nerveImage2 );
  nerveDistanceFilter->SetInsideValue( 100 );
  nerveDistanceFilter->SetOutsideValue( 0 );
  nerveDistanceFilter->Update();
  ImageType::Pointer nerveDistance = nerveDistanceFilter->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( nerveDistance, catStrings( prefix, "-nerve-distance.tif" ) );
#endif

  //Compute max of distance transfrom
  ImageCalculatorFilterType::Pointer nerveCalculatorFilter = ImageCalculatorFilterType::New();
  nerveCalculatorFilter->SetImage( nerveDistance );
  nerveCalculatorFilter->Compute();

  nerve.initialWidth = nerveCalculatorFilter->GetMaximum();

  if( nerve.initialWidth <= 0 )
    {
    return false;
    }
  nerve.initialCenterIndex = nerveCalculatorFilter->GetIndexOfMaximum();
  nerveImage->TransformIndexToPhysicalPoint( nerve.initialCenterIndex, nerve.initialCenter );

#ifdef DEBUG_PRINT
  std::cout << "Approximate width of nerve: " << 2 * nerve.initialWidth << std::endl;
  std::cout << "Approximate nerve center: " << nerve.initialCenter << std::endl;
  std::cout << "Approximate nerve center Index: " << nerve.initialCenterIndex << std::endl;
#endif

  //-- Step 4
  //   Rescale rows left and right of the approximate center to 0 - 100

  float centerIntensity = nerveImage->GetPixel( nerve.initialCenterIndex );
#ifdef DEBUG_PRINT
  std::cout << "Approximate nerve center intensity: " << centerIntensity << std::endl;
#endif
  for( int i = 0; i < nerveSize[ 1 ]; i++ )
    {
    ImageType::IndexType index;
    index[ 1 ] = i;
    float maxIntensityLeft = 0;
    for( int j = 0; j < nerve.initialCenterIndex[ 0 ]; j++ )
      {
      index[ 0 ] = j;
      maxIntensityLeft = std::max( nerveImage->GetPixel( index ), maxIntensityLeft );
      }
    for( int j = 0; j < nerve.initialCenterIndex[ 0 ]; j++ )
      {
      index[ 0 ] = j;
      float value = 0;
      if( maxIntensityLeft > centerIntensity )
        {
        value = ( nerveImage->GetPixel( index ) - centerIntensity ) /
          ( maxIntensityLeft - centerIntensity );
        value = std::max( 0.f, value ) * 100;
        value = std::min( 100.f, value );
        }
      nerveImage->SetPixel( index, value );
      }

    float maxIntensityRight = 0;
    for( int j = nerve.initialCenterIndex[ 0 ]; j < nerveSize[ 0 ]; j++ )
      {
      index[ 0 ] = j;
      maxIntensityRight = std::max( nerveImage->GetPixel( index ), maxIntensityRight );
      }
    for( int j = nerve.initialCenterIndex[ 0 ]; j < nerveSize[ 0 ]; j++ )
      {
      index[ 0 ] = j;
      float value = 0;
      if( maxIntensityRight > centerIntensity )
        {
        value = ( nerveImage->GetPixel( index ) - centerIntensity ) /
          ( maxIntensityRight - centerIntensity );
        value = std::max( 0.f, value ) * 100;
        value = std::min( 100.f, value );
        }
      nerveImage->SetPixel( index, value );
      }
    }

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( nerveImage, catStrings( prefix, "-nerve-scaled.tif" ) );
#endif

  //-- Step 5
  //   Binary threshold

  nerveImage = ITKFilterFunctions<ImageType>::BinaryThreshold( nerveImage, -1,
    algParams.nerveRegistrationThreshold, 0, 100 );

#ifdef DEBUG_PRINT
  std::cout << "Nerve threshold: " << algParams.nerveRegistrationThreshold << std::endl;
#endif

/*
  StructuringElementType structuringElement2;
  structuringElement2.SetRadius( 10 );
  structuringElement2.CreateStructuringElement();
  OpeningFilter::Pointer openingFilter2 = OpeningFilter::New();
  openingFilter2->SetInput(nerveImage);
  openingFilter2->SetKernel(structuringElement2);
  openingFilter2->SetForegroundValue(100.0);
  openingFilter2->Update();
  nerveImage = openingFilter2->GetOutput();
*/

  //-- Step 5.1 through 5.4
  //  5.1 Add vertica border
  //  5.2 Add horizontal border
  //  5.3 Distance transform
  //  5.4 Refine intial estimates

  CastFilter::Pointer nerveCastFilter2 = CastFilter::New();
  nerveCastFilter2->SetInput( nerveImage );
  nerveCastFilter2->Update();
  UnsignedCharImageType::Pointer nerveImage3 = nerveCastFilter2->GetOutput();

  //ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorder(nerveImage3,
  //          algParams.nerveRefineVerticalBorderFactor* std::min(imageSize[0], imageSize[1]) );

  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorderLeft( nerveImage3, leftBorder );
  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorderRight( nerveImage3, rightBorder );


  ITKFilterFunctions<UnsignedCharImageType>::AddHorizontalBorder( nerveImage3,
    algParams.nerveHorizontalBoder );

  SignedDistanceFilter::Pointer nerveDistanceFilter2 = SignedDistanceFilter::New();
  nerveDistanceFilter2->SetInput( nerveImage3 );
  nerveDistanceFilter2->SetInsideValue( 100 );
  nerveDistanceFilter2->SetOutsideValue( 0 );
  nerveDistanceFilter2->Update();
  ImageType::Pointer nerveDistance2 = nerveDistanceFilter2->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( nerveDistance, catStrings( prefix, "-nerve-scaled-distance.tif" ) );
#endif

  //Compute max of distance transfrom
  ImageCalculatorFilterType::Pointer nerveCalculatorFilter2 = ImageCalculatorFilterType::New();
  nerveCalculatorFilter2->SetImage( nerveDistance2 );
  nerveCalculatorFilter2->Compute();

  nerve.initialWidth = nerveCalculatorFilter2->GetMaximum();
  if( nerve.initialWidth <= 0 )
    {
    return false;
    }
  nerve.initialCenterIndex = nerveCalculatorFilter2->GetIndexOfMaximum();
  nerveImage->TransformIndexToPhysicalPoint( nerve.initialCenterIndex, nerve.initialCenter );

#ifdef DEBUG_PRINT
  std::cout << "Refined approximate width of nerve: " << 2 * nerve.initialWidth << std::endl;
  std::cout << "Refined approximate nerve center: " << nerve.initialCenter << std::endl;
  std::cout << "Refined approximate nerve center Index: " << nerve.initialCenterIndex << std::endl;
#endif

  //-- Step 6
  //   Add a bit of smoothing for the registration process
  sigma[ 0 ] = algParams.nerveRegsitrationSmooth * nerveSpacing[ 0 ];
  sigma[ 1 ] = algParams.nerveRegsitrationSmooth * nerveSpacing[ 1 ];
  nerveImage = ITKFilterFunctions<ImageType>::GaussSmooth( nerveImage, sigma );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( nerveImage, catStrings( prefix, "-nerve-thres.tif" ) );
#endif

#ifdef REPORT_TIMES
  clockNerveA.Stop();
#endif


  /////
  //B. Prepare fixed image.
  //  Create artifical nerve image to fit to region of interest.
  /////

#ifdef REPORT_TIMES
  clockNerveB.Start();
#endif

  //--Step 1 and C) 1
  //  Create a black and white image with two bars that
  //  are an intial estimate of the width apart.
  //  Create registration mask image.

  ImageType::Pointer moving = ImageType::New();
  moving->SetRegions( nerveRegion );
  moving->Allocate();
  moving->FillBuffer( 0.0 );
  moving->SetSpacing( nerveSpacing );
  moving->SetOrigin( nerveOrigin );
  moving->SetDirection( nerveDirection );

  UnsignedCharImageType::Pointer movingMask = UnsignedCharImageType::New();
  movingMask->SetRegions( nerveRegion );
  movingMask->Allocate();
  movingMask->FillBuffer( itk::NumericTraits< unsigned char >::Zero );
  movingMask->SetSpacing( nerveSpacing );
  movingMask->SetOrigin( nerveOrigin );

  int nerveYStart = nerveSize[ 0 ] * algParams.nerveYOffsetFactor;
  int nerveXStart1 = nerve.initialCenterIndex[ 0 ] - 1.5 * nerve.initialWidth / nerveSpacing[ 0 ];
  int nerveXEnd1 = nerve.initialCenterIndex[ 0 ] - 1 * nerve.initialWidth / nerveSpacing[ 0 ];
  int nerveXStart2 = nerve.initialCenterIndex[ 0 ] + 1 * nerve.initialWidth / nerveSpacing[ 0 ];
  int nerveXEnd2 = nerve.initialCenterIndex[ 0 ] + 1.5 * nerve.initialWidth / nerveSpacing[ 0 ];

  if( nerveXStart1 < 0 )
    {
    nerveXStart1 = 0;
    }
  if( nerveXEnd2 >= nerveSize[ 0 ] )
    {
    nerveXEnd2 = nerveSize[ 0 ];
    }
  if( nerveXEnd2 < nerveXStart2 )
    {
    //std::cout << "Failed to locate nerve" << std::endl;
    return false;
    }

  for( int i = nerveYStart; i < nerveSize[ 1 ]; i++ )
    {
    ImageType::IndexType index;
    index[ 1 ] = i;
    for( int j = nerveXStart1; j < nerveXEnd2; j++ )
      {
      index[ 0 ] = j;
      movingMask->SetPixel( index, 255 );
      }

    for( int j = nerveXStart1; j < nerveXEnd1; j++ )
      {
      index[ 0 ] = j;
      moving->SetPixel( index, 100.0 );
      }
    for( int j = nerveXStart2; j < nerveXEnd2; j++ )
      {
      index[ 0 ] = j;
      moving->SetPixel( index, 100.0 );
      }
    }

#ifdef DEBUG_IMAGES
  ImageIO<UnsignedCharImageType>::WriteImage( movingMask, catStrings( prefix, "-nerve-mask.tif" ) );
#endif

  //-- Step 2
  //   Gauss smoothing

  sigma[ 0 ] = algParams.nerveRegsitrationSmooth * nerveSpacing[ 0 ];
  sigma[ 1 ] = algParams.nerveRegsitrationSmooth * nerveSpacing[ 1 ];
  moving = ITKFilterFunctions<ImageType>::GaussSmooth( moving, sigma );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( moving, catStrings( prefix, "-nerve-moving.tif" ) );
#endif

#ifdef REPORT_TIMES
  clockNerveB.Stop();
#endif


  ////
  //C. Registration of artifical nerve image to threhsold nerve image
  ////

#ifdef REPORT_TIMES
  clockNerveC1.Start();
#endif

  //-- Step 2 (Step 1 was inclued in B)
  //   Similarity transfrom registration centered on the fixed bars image

  SimilarityTransformType::Pointer transform = SimilarityTransformType::New();
  transform->SetCenter( nerve.initialCenter );


  MetricType::Pointer         metric = MetricType::New();
  OptimizerType::Pointer      optimizer = OptimizerType::New();
  InterpolatorType::Pointer   movingInterpolator = InterpolatorType::New();
  InterpolatorType::Pointer   fixedInterpolator = InterpolatorType::New();
  RegistrationType::Pointer   registration = RegistrationType::New();

  optimizer->SetGradientConvergenceTolerance( 0.000001 );
  optimizer->SetLineSearchAccuracy( 0.5 );
  optimizer->SetDefaultStepLength( 0.00001 );
#ifdef DEBUG_PRINT
  optimizer->TraceOn();
#endif
  optimizer->SetMaximumNumberOfFunctionEvaluations( 20000 );

  metric->SetMovingInterpolator( movingInterpolator );
  metric->SetFixedInterpolator( fixedInterpolator );

  MaskType::Pointer  spatialObjectMask = MaskType::New();
  spatialObjectMask->SetImage( movingMask );
  metric->SetFixedImageMask( spatialObjectMask );

  registration->SetMetric( metric );
  registration->SetOptimizer( optimizer );
  registration->SetMovingImage( nerveImage );
  registration->SetFixedImage( moving );

#ifdef DEBUG_PRINT
  std::cout << "Transform parameters: " << std::endl;
  std::cout << transform->GetParameters() << std::endl;
  std::cout << transform->GetCenter() << std::endl;
#endif

  registration->SetInitialTransform( transform );

  RegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
  shrinkFactorsPerLevel.SetSize( 1 );
  //shrinkFactorsPerLevel[0] = 2;
  //shrinkFactorsPerLevel[1] = 1;
  shrinkFactorsPerLevel[ 0 ] = 1;

  RegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
  smoothingSigmasPerLevel.SetSize( 1 );
  //smoothingSigmasPerLevel[0] = 0.5;
  //smoothingSigmasPerLevel[1] = 0;
  smoothingSigmasPerLevel[ 0 ] = 0;

  registration->SetNumberOfLevels( 1 );
  registration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
  registration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );

  //Do registration
  try
    {
    registration->SetNumberOfThreads( 1 );
    registration->Update();
    }
  catch( itk::ExceptionObject & err )
    {
#ifdef DEBUG_PRINT
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
#endif
      //return EXIT_FAILURE;
    }

#ifdef DEBUG_PRINT
  const double bestValue = optimizer->GetValue();
  std::cout << "Result = " << std::endl;
  std::cout << " Metric value  = " << bestValue << std::endl;

  std::cout << "Registered transform parameters: " << std::endl;
  std::cout << registration->GetTransform()->GetParameters() << std::endl;
  std::cout << transform->GetCenter() << std::endl;
#endif

#ifdef REPORT_TIMES
  clockNerveC1.Stop();
#endif

#ifdef REPORT_TIMES
  clockNerveC2.Start();
#endif

  if( alignNerve )
    {
    SimilarityTransformType::Pointer inverse = SimilarityTransformType::New();
    transform->GetInverse( inverse );

    // Create registered bars image
    ResampleFilterType::Pointer resampler = ResampleFilterType::New();
    resampler->SetInput( moving );
    resampler->SetTransform( inverse );
    resampler->SetSize( nerveSize );
    resampler->SetOutputOrigin( nerveOrigin );
    resampler->SetOutputSpacing( nerveSpacing );
    resampler->SetOutputDirection( nerveImage->GetDirection() );
    resampler->SetDefaultPixelValue( 0 );
    resampler->Update();
    ImageType::Pointer moved = resampler->GetOutput();

    nerve.aligned = moved;

#ifdef DEBUG_IMAGES
    ImageIO<ImageType>::WriteImage( moved, catStrings( prefix, "-nerve-registered.tif" ) );
#endif
    }

  //-- Step 3
  //   Compute nerve width by pushing intital width through the transform

  SimilarityTransformType::InputPointType tCenter;
  tCenter[ 0 ] = nerve.initialCenter[ 0 ];
  tCenter[ 1 ] = nerve.initialCenter[ 1 ];

  SimilarityTransformType::InputVectorType tX;
  tX[ 0 ] = nerve.initialWidth;
  tX[ 1 ] = 0;

  nerve.center = transform->TransformPoint( tCenter );
  nerveImage->TransformPhysicalPointToIndex( nerve.center, nerve.centerIndex );

  SimilarityTransformType::OutputVectorType tXO = transform->TransformVector( tX, tCenter );

  nerve.width = sqrt( tXO[ 0 ] * tXO[ 0 ] + tXO[ 1 ] * tXO[ 1 ] );

  //Check validity of estimates

  //limit rotation to 45 degress
  double rotation = transform->GetRotation();
  if( std::fabs( rotation ) > M_PI / 3.0  && std::fabs( rotation ) < M_PI * 2.0 / 3.0 )
    {
#ifdef DEBUG_PRINT
    std::cout << "Estimation failed: Nerve rotated too much " << rotation << std::endl;
#endif
    return false;
    }

  //Check if nerve width falls our of bounds
  SimilarityTransformType::OutputVectorType tW = tXO;
  tW[ 0 ] = std::fabs( tW[ 0 ] * imageSpacing[ 0 ] );
  tW[ 1 ] = std::fabs( tW[ 1 ] * imageSpacing[ 1 ] );
  if( nerve.centerIndex[ 0 ] < nerveIndex[ 0 ] ||
    nerve.centerIndex[ 0 ] > nerveIndex[ 0 ] + nerveSize[ 0 ] )
    {
#ifdef DEBUG_PRINT
    std::cout << "Estimation failed: Nerve out of bounds" << std::endl;
    std::cout << nerve.centerIndex << std::endl;
    std::cout << tW << std::endl;
#endif
    return false;
    }
/*
    if( nerve.centerIndex[1] < nerveIndex[1] ||
        nerve.centerIndex[1] > nerveIndex[1] + nerveSize[1] ){
  #ifdef DEBUG_PRINT
    std::cout << "Estimation failed: Nerve out of bounds" << std::endl;
    std::cout << nerve.centerIndex << std::endl;
    std::cout << tW << std::endl;
  #endif
    return false;
    }
*/

#ifdef DEBUG_PRINT
  std::cout << "Nerve center: " << nerve.centerIndex << std::endl;
  std::cout << "Nerve width: " << nerve.width * 2 << std::endl;

  std::cout << "--- Done fitting nerve ---" << std::endl << std::endl;
#endif

#ifdef REPORT_TIMES
  clockNerveC2.Stop();
#endif

  return true;
};
