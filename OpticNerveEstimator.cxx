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
                          std::string prefix){

  bool fitEyeSucces = FitEye( origImage, prefix, overlay);
  if(!fitEyeSucces){
    return ESTIMATION_FAIL_EYE;
  }
  bool fitStemSucces = FitStem( origImage, eye, prefix, overlay);
  if(!fitStemSucces){
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
  std::cout << "Stem A:  " << clockStemA.GetMean() << std::endl;
  std::cout << "Stem B:  " << clockStemB.GetMean() << std::endl;
  std::cout << "Stem C1: " << clockStemC1.GetMean() << std::endl;
  std::cout << "Stem C2: " << clockStemC2.GetMean() << std::endl;
#endif


  //Return early if no image is required
  if( !overlay ){
     return ESTIMATION_SUCCESS;
  }


  ////
  //4. Create overlay image
  ////
  typedef itk::CastImageFilter< ImageType, RGBImageType> RGBCastFilter;
  RGBCastFilter::Pointer rgbConvert = RGBCastFilter::New();
  rgbConvert->SetInput( origImage );
  rgbConvert->Update();
  overlayImage = rgbConvert->GetOutput(); 


  std::cout << overlayImage->GetLargestPossibleRegion() << std::endl;
  itk::ImageRegionIterator<RGBImageType> overlayIterator2( overlayImage, 
                                             overlayImage->GetLargestPossibleRegion());
  itk::ImageRegionIterator<ImageType> eyeIterator( eye.aligned, 
                                             eye.aligned->GetLargestPossibleRegion() );

  double alpha = 0.15;
  while( !overlayIterator2.IsAtEnd() ){
    RGBImageType::PixelType pixel = overlayIterator2.Get();
    double p = eyeIterator.Get();
    if(p > 25){
      pixel[0] = (1-alpha) * pixel[0];
      pixel[1] = (1-alpha) * pixel[1];
      pixel[2] = alpha * 255 + (1-alpha) * pixel[2];
    }
    overlayIterator2.Set( pixel ); 

    ++eyeIterator; 
    ++overlayIterator2;
  }

  itk::ImageRegionIterator<RGBImageType> overlayIterator( overlayImage, 
                                          stem.originalImageRegion);
  itk::ImageRegionIterator<ImageType> stemIterator( stem.aligned, 
                                          stem.aligned->GetLargestPossibleRegion() );
  alpha = 0.3;
  while( !overlayIterator.IsAtEnd() ){
    RGBImageType::PixelType pixel = overlayIterator.Get();
    double p = stemIterator.Get(); 
    if(p > 5){
      pixel[0] = alpha * 255 + (1-alpha) * pixel[0];
      pixel[1] = (1-alpha) * pixel[1];
      pixel[2] = (1-alpha) * pixel[2];
    }
    overlayIterator.Set( pixel ); 
    
    ++stemIterator; 
    ++overlayIterator;
  }



#ifdef DEBUG_IMAGES
  ImageIO<RGBImageType>::WriteImage( overlayImage, catStrings(prefix, "-overlay.png") );
#endif

   return ESTIMATION_SUCCESS;
}

//Helper function
std::string OpticNerveEstimator::catStrings(std::string s1, std::string s2){
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
                                         double outside, double inside ){

  SpatialObjectToImageFilterType::Pointer imageFilter =
    SpatialObjectToImageFilterType::New();

  //origin[0] -= 0.5 * size[0] * spacing[0];
  //origin[1] -= 0.5 * size[1] * spacing[1];
  //size[0] *= 2;
  //size[1] *= 2;
  ImageType::SizeType smallSize = size;
  smallSize[0] = 100;
  smallSize[1] = 100;
  ImageType::SpacingType smallSpacing = spacing;
  smallSpacing[0] = ( smallSpacing[0] / smallSize[0] ) * size[0];
  smallSpacing[1] = ( smallSpacing[1] / smallSize[1] ) * size[1];

  imageFilter->SetSize( smallSize );
  imageFilter->SetOrigin( origin );
  imageFilter->SetSpacing( smallSpacing );
  imageFilter->SetDirection( direction );

  EllipseType::Pointer ellipse   = EllipseType::New();
  EllipseType::ArrayType radiusArray;
  radiusArray[0] = r1;
  radiusArray[1] = r2;
  ellipse->SetRadius(radiusArray);

  EllipseTransformType::Pointer transform = EllipseTransformType::New();
  transform->SetIdentity();
  EllipseTransformType::OutputVectorType  translation;
  translation[ 0 ] =  center[0];
  translation[ 1 ] =  center[1];
  transform->Translate( translation, false );
  ellipse->SetObjectToParentTransform( transform );

  imageFilter->SetInput(ellipse);
  ellipse->SetDefaultInsideValue( inside );
  ellipse->SetDefaultOutsideValue( outside );
  imageFilter->SetUseObjectValue( true );
  imageFilter->SetOutsideValue( outside );
  imageFilter->Update();

  ResampleFilterType::Pointer resampler = ResampleFilterType::New();
  resampler->SetInput( imageFilter->GetOutput() );
  resampler->SetSize( size );
  resampler->SetOutputOrigin(  origin );
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
                             const std::string &prefix, bool alignEllipse){

#ifdef DEBUG_PRINT
  std::cout << "--- Fitting Eye ---" << std::endl << std::endl;
#endif

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage(inputImage, catStrings(prefix, "-eye-input.tif") );
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

  ImageType::Pointer image = ITKFilterFunctions<ImageType>::Rescale(inputImage, 0, 100);

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
  sigma[0] = algParams.eyeInitialBlurFactor * imageSpacing[0];
  sigma[1] = algParams.eyeInitialBlurFactor * imageSpacing[1];

  ITKFilterFunctions<ImageType>::AddHorizontalBorder(image,
       imageSize[0] * algParams.eyeHorizontalBorderFactor );
  image = ITKFilterFunctions<ImageType>::GaussSmooth(image, sigma);


  //-- Step 4
  //   Binary Thresholding

  image = ITKFilterFunctions<ImageType>::BinaryThreshold(image, -1,
                              algParams.eyeInitialBinaryThreshold, 0, 100);


  //-- Steps 4.1 through 4.4
  //   4.1 Morphological closing
  //   4.2 Adding a vertical border
  //   4.3 Distance transfrom
  //   4.4 Calculate inital center and radius from distance transform (Max)

  StructuringElementType structuringElement;
  structuringElement.SetRadius(
          algParams.eyeClosingRadiusFactor * std::min(imageSize[0], imageSize[1])
         );
  structuringElement.CreateStructuringElement();
  ClosingFilter::Pointer closingFilter = ClosingFilter::New();
  closingFilter->SetInput(image);
  closingFilter->SetKernel(structuringElement);
  closingFilter->SetForegroundValue(100.0);
  closingFilter->Update();
  image = closingFilter->GetOutput();

  CastFilter::Pointer castFilter = CastFilter::New();
  castFilter->SetInput( image );
  castFilter->Update();
  UnsignedCharImageType::Pointer  sdImage = castFilter->GetOutput();


  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorder( sdImage,
           algParams.eyeVerticalBorderFactor * imageSize[0] );

  SignedDistanceFilter::Pointer signedDistanceFilter = SignedDistanceFilter::New();
  signedDistanceFilter->SetInput( sdImage );
  signedDistanceFilter->SetInsideValue(100);
  signedDistanceFilter->SetOutsideValue(0);
  signedDistanceFilter->Update();
  ImageType::Pointer imageDistance = signedDistanceFilter->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage(imageDistance, catStrings(prefix, "-eye-distance.tif") );
#endif

  //Compute max of distance transfrom
  ImageCalculatorFilterType::Pointer imageCalculatorFilter = ImageCalculatorFilterType::New ();
  imageCalculatorFilter->SetImage( imageDistance );
  imageCalculatorFilter->Compute();

  eye.initialRadius = imageCalculatorFilter->GetMaximum() ;
  eye.initialCenterIndex = imageCalculatorFilter->GetIndexOfMaximum();
  image->TransformIndexToPhysicalPoint(eye.initialCenterIndex, eye.initialCenter);


#ifdef DEBUG_PRINT
  std::cout << "Eye inital center index: " << eye.initialCenterIndex << std::endl;
  std::cout << "Eye inital center: " << eye.initialCenter << std::endl;
  std::cout << "Eye initial radius: "<< eye.initialRadius << std::endl;
#endif
  if(eye.initialRadius <= 0 ){
     return false;
  }



  //-- Steps 4.4.1 through 4.4.2
  //   4.4.1 Distance transform in X and Y seperately on region of interest
  //   	  around slabs of the center
  //  4.4.2 Calculate inital x and y radius from those distamnce transforms


  //Compute vertical distance to eye border
  CastFilter::Pointer castFilter2 = CastFilter::New();
  castFilter2->SetInput( image );
  castFilter2->Update();
  UnsignedCharImageType::Pointer  sdImage2 = castFilter2->GetOutput();
  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorder( sdImage2, 2);


  ImageType::SizeType yRegionSize;
  yRegionSize[0] = algParams.eyeYSlab;
  yRegionSize[1] = imageSize[1];
  ImageType::IndexType yRegionIndex;
  yRegionIndex[0] =  eye.initialCenterIndex[0] - algParams.eyeYSlab/2;
  yRegionIndex[1] =  0;
  ImageType::RegionType yRegion(yRegionIndex, yRegionSize);

  ExtractFilter2::Pointer extractFilterY = ExtractFilter2::New();
  extractFilterY->SetRegionOfInterest(yRegion);
  extractFilterY->SetInput( sdImage2 );
  extractFilterY->Update();

  SignedDistanceFilter::Pointer signedDistanceY = SignedDistanceFilter::New();
  signedDistanceY->SetInput( extractFilterY->GetOutput() );
  signedDistanceY->SetInsideValue(100);
  signedDistanceY->SetOutsideValue(0);
  //signedDistanceY->GetOutput()->SetRequestedRegion( yRegion );
  signedDistanceY->Update();
  ImageType::Pointer imageDistanceY = signedDistanceY->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage(imageDistanceY, catStrings(prefix, "-eye-ydistance.tif") );
#endif

  ImageCalculatorFilterType::Pointer imageCalculatorY = ImageCalculatorFilterType::New ();
  imageCalculatorY->SetImage( imageDistanceY );
  imageCalculatorY->Compute();

  eye.initialRadiusY = imageCalculatorY->GetMaximum() ;

#ifdef DEBUG_PRINT
  std::cout << "Eye initial radiusY: "<< eye.initialRadiusY << std::endl;
#endif

  //Compute horizontal distance to eye border
  ImageType::SizeType xRegionSize;
  xRegionSize[0] = imageSize[0];
  xRegionSize[1] = algParams.eyeXSlab;
  ImageType::IndexType xRegionIndex;
  xRegionIndex[0] =  0;
  xRegionIndex[1] =  eye.initialCenterIndex[1] - algParams.eyeXSlab / 2;
  ImageType::RegionType xRegion(xRegionIndex, xRegionSize);

  ExtractFilter2::Pointer extractFilterX = ExtractFilter2::New();
  extractFilterX->SetRegionOfInterest(xRegion);
  extractFilterX->SetInput( sdImage2 );
  extractFilterX->Update();

  SignedDistanceFilter::Pointer signedDistanceX = SignedDistanceFilter::New();
  signedDistanceX->SetInput( extractFilterX->GetOutput() );
  signedDistanceX->SetInsideValue(100);
  signedDistanceX->SetOutsideValue(0);
  //signedDistanceX->GetOutput()->SetRequestedRegion( xRegion );
  signedDistanceX->Update();
  ImageType::Pointer imageDistanceX = signedDistanceX->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage(imageDistanceX, catStrings(prefix, "-eye-xdistance.tif") );
#endif

  ImageCalculatorFilterType::Pointer imageCalculatorX = ImageCalculatorFilterType::New ();
  imageCalculatorX->SetImage( imageDistanceX );
  imageCalculatorX->Compute();

  eye.initialRadiusX = imageCalculatorX->GetMaximum() ;

#ifdef DEBUG_PRINT
  std::cout << "Eye initial radiusX: "<< eye.initialRadiusX << std::endl;
#endif

  eye.initialCenterIndex[0] = eye.initialRadiusX;
  eye.initialCenterIndex[1] = eye.initialRadiusY;
  image->TransformIndexToPhysicalPoint(eye.initialCenterIndex, eye.initialCenter);
#ifdef DEBUG_PRINT
  std::cout << "Eye inital center index: " << eye.initialCenterIndex << std::endl;
  std::cout << "Eye inital center: " << eye.initialCenter << std::endl;
#endif

  //--Step 5
  //  Gaussian smoothing, threshold and rescale

  sigma[0] = 10 * imageSpacing[0];
  sigma[1] = 10 * imageSpacing[1];
  ImageType::Pointer imageSmooth = ITKFilterFunctions<ImageType>::GaussSmooth(image, sigma);
  imageSmooth = ITKFilterFunctions<ImageType>::ThresholdAbove( imageSmooth,
                                                      algParams.eyeThreshold, 100);
  imageSmooth = ITKFilterFunctions<ImageType>::Rescale( imageSmooth, 0, 100);

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage(imageSmooth, catStrings(prefix, "-eye-smooth.tif") );
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
  //intial guess of major axis
  double r1 = eye.initialRadiusX;
  //inital guess of minor axis
  double r2 = eye.initialRadiusY;
  //width of the ellipse ring rf*r1, rf*r2
  ImageType::Pointer e1 = CreateEllipseImage( imageSpacing, imageSize, imageOrigin, imageDirection,
		                               eye.initialCenter, r1, r2, outside);
  ImageType::Pointer e2 = CreateEllipseImage( imageSpacing, imageSize, imageOrigin, imageDirection,
		                              eye.initialCenter, r1 * algParams.eyeRingFactor,
                                                     r2 * algParams.eyeRingFactor,
                                                     outside );

  ImageType::Pointer ellipse = ITKFilterFunctions<ImageType>::Subtract(e1, e2);

  sigma[0] = algParams.eyeInitialBlurFactor * imageSpacing[0];
  sigma[1] = algParams.eyeInitialBlurFactor * imageSpacing[1];
  ellipse = ITKFilterFunctions<ImageType>::GaussSmooth(ellipse, sigma);
  ellipse = ITKFilterFunctions<ImageType>::ThresholdAbove(ellipse,
      algParams.eyeThreshold, 100);
  ellipse = ITKFilterFunctions<ImageType>::Rescale(ellipse, 0, 100);

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( ellipse, catStrings(prefix, "-eye-moving.tif") );
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

  ImageType::Pointer ellipseMask = CreateEllipseImage( imageSpacing, imageSize, imageOrigin, imageDirection,
		                                       eye.initialCenter,
                                                       r1 * ( algParams.eyeRingFactor + 1 ) / 2,
                                                       r2 * ( algParams.eyeRingFactor + 1 ) / 2,
                                                       0, 100 );
  //remove left and right corners from mask
  for(int i=0; i<eye.initialCenterIndex[0] - algParams.eyeMaskCornerXFactor * r1; i++){
    ImageType::IndexType index;
    index[0] = i;
    for(int j = eye.initialCenterIndex[1] - algParams.eyeMaskCornerYFactor * r2;
            j < eye.initialCenterIndex[1] + algParams.eyeMaskCornerYFactor * r2; j++){
      index[1]=j;
      ellipseMask->SetPixel(index, 0);
    }
  }
  for(int i=eye.initialCenterIndex[0] + algParams.eyeMaskCornerXFactor * r1; i < imageSize[0]; i++){
    ImageType::IndexType index;
    index[0] = i;
    for(int j = eye.initialCenterIndex[1] - algParams.eyeMaskCornerYFactor * r2;
            j < eye.initialCenterIndex[1] + algParams.eyeMaskCornerYFactor * r2; j++){
      index[1]=j;
      ellipseMask->SetPixel(index, 0);
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
  transform->SetCenter(eye.initialCenter);


  MetricType::Pointer         metric        = MetricType::New();
  OptimizerType::Pointer      optimizer       = OptimizerType::New();
  InterpolatorType::Pointer   movingInterpolator  = InterpolatorType::New();
  InterpolatorType::Pointer   fixedInterpolator  = InterpolatorType::New();
  RegistrationType::Pointer   registration  = RegistrationType::New();


  optimizer->SetGradientConvergenceTolerance( 0.000001 );
  optimizer->SetLineSearchAccuracy( 0.5 );
  optimizer->SetDefaultStepLength( 0.00001 );
#ifdef DEBUG_PRINT
  optimizer->TraceOn();
#endif
  optimizer->SetMaximumNumberOfFunctionEvaluations( 20000 );

/*
  OptimizerType::ScalesType scales( transform->GetNumberOfParameters() );
  scales[0] = 1.0;
  scales[1] = 1.0;
  scales[2] = 1.0;
  scales[3] = 1.0;
  scales[4] = 1.0;
  scales[5] = 1.0;
  optimizer->SetScales( scales );
*/

  metric->SetMovingInterpolator( movingInterpolator );
  metric->SetFixedInterpolator( fixedInterpolator );


  CastFilter::Pointer castFilter3 = CastFilter::New();
  castFilter3->SetInput( ellipseMask );
  castFilter3->Update();
  MaskType::Pointer  spatialObjectMask = MaskType::New();
  spatialObjectMask->SetImage( castFilter3->GetOutput() );
  metric->SetFixedImageMask( spatialObjectMask );

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( ellipseMask, catStrings(prefix, "-eye-mask.tif")  );
#endif

  registration->SetMetric(        metric        );
  registration->SetOptimizer(     optimizer     );

  registration->SetMovingImage(    imageSmooth    );
  registration->SetFixedImage(   ellipse   );

  std::cout <<  transform->GetParameters()  << std::endl;
  std::cout <<  transform->GetCenter()  << std::endl;
  registration->SetInitialTransform( transform );

  RegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
  shrinkFactorsPerLevel.SetSize( 1 );
  shrinkFactorsPerLevel[0] = std::max(1,
      (int) ( std::min( imageSize[0], imageSize[1]) /algParams.eyeRegistrationSize ) );
  //shrinkFactorsPerLevel[1] = 4;
  //shrinkFactorsPerLevel[2] = 2;
  //shrinkFactorsPerLevel[3] = 1;

  RegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
  smoothingSigmasPerLevel.SetSize( 1 );
  smoothingSigmasPerLevel[0] = 0;
  //smoothingSigmasPerLevel[1] = 0;
  //smoothingSigmasPerLevel[2] = 0.5;
  //smoothingSigmasPerLevel[3] = 0;
  //smoothingSigmasPerLevel[0] = 0;

  registration->SetNumberOfLevels ( 1 );
  registration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
  registration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );

  //Do registration
  try{
	  registration->SetNumberOfThreads(1);
	  registration->Update();
  }
  catch( itk::ExceptionObject & err ){
#ifdef DEBUG_PRINT
	  std::cerr << "ExceptionObject caught !" << std::endl;
	  std::cerr << err << std::endl;
	  //return EXIT_FAILURE;
#endif
  }


#ifdef DEBUG_PRINT
  const double bestValue = optimizer->GetValue();
  std::cout << "Result = " << std::endl;
  std::cout << " Metric value  = " << bestValue          << std::endl;

  std::cout << "Optimized transform paramters:" << std::endl;
  std::cout <<  registration->GetTransform()->GetParameters()  << std::endl;
  std::cout <<  transform->GetCenter()  << std::endl;
#endif

#ifdef REPORT_TIMES
  clockEyeC2.Stop();
#endif




#ifdef REPORT_TIMES
  clockEyeC3.Start();
#endif


  //Created registered ellipse image
  if(alignEllipse){
    AffineTransformType::Pointer inverse = AffineTransformType::New();
    transform->GetInverse( inverse );



    ResampleFilterType::Pointer resampler = ResampleFilterType::New();
    resampler->SetInput( ellipse );
    resampler->SetTransform( inverse );
    resampler->SetSize( imageSize );
    resampler->SetOutputOrigin(  image->GetOrigin() );
    resampler->SetOutputSpacing( imageSpacing );
    resampler->SetOutputDirection( image->GetDirection() );
    resampler->SetDefaultPixelValue( 0 );
    resampler->Update();
    ImageType::Pointer moved = resampler->GetOutput();

    eye.aligned = moved;

#ifdef DEBUG_IMAGES
    ImageIO<ImageType>::WriteImage( moved, catStrings(prefix, "-eye-registred.tif")  );


    ImageType::Pointer ellipseThres = ITKFilterFunctions<ImageType>::ThresholdAbove(moved, 5, 255);
    CastFilter::Pointer teCast = CastFilter::New();
    teCast->SetInput( ellipseThres );

    BinaryImageToLabelMapFilterType::Pointer binaryImageToLabelMapFilter = BinaryImageToLabelMapFilterType::New();
    binaryImageToLabelMapFilter->SetInput( teCast->GetOutput() );
    binaryImageToLabelMapFilter->Update();

    LabelMapToLabelImageFilterType::Pointer labelMapToLabelImageFilter = LabelMapToLabelImageFilterType::New();
    labelMapToLabelImageFilter->SetInput(binaryImageToLabelMapFilter->GetOutput());
    labelMapToLabelImageFilter->Update();

    ImageType::Pointer imageRescale = ITKFilterFunctions<ImageType>::Rescale(inputImage, 0, 255);
    LabelOverlayImageFilterType::Pointer labelOverlayImageFilter = LabelOverlayImageFilterType::New();
    labelOverlayImageFilter->SetInput( imageRescale );
    labelOverlayImageFilter->SetLabelImage(labelMapToLabelImageFilter->GetOutput());
    labelOverlayImageFilter->SetOpacity(.5);
    labelOverlayImageFilter->Update();

    ImageIO<RGBImageType>::WriteImage( labelOverlayImageFilter->GetOutput(), catStrings(prefix, "-eye-overlay.png") );
#endif
  }


  //-- Step 3
  //   Compute minor and major axis by pushing the radii from the created ellipse
  //   image through the computed transform

  AffineTransformType::InputPointType tCenter;
  tCenter[0] = eye.initialCenter[0];
  tCenter[1] = eye.initialCenter[1];

  AffineTransformType::InputVectorType tX;
  tX[0] = r1;
  tX[1] = 0;

  AffineTransformType::InputVectorType tY;
  tY[0] = 0;
  tY[1] = r2;

  eye.center = transform->TransformPoint(tCenter);
  image->TransformPhysicalPointToIndex(eye.center, eye.centerIndex);

  AffineTransformType::OutputVectorType tXO = transform->TransformVector(tX, tCenter);
  AffineTransformType::OutputVectorType tYO = transform->TransformVector(tY, tCenter);

  eye.minor =  sqrt(tXO[0]*tXO[0] + tXO[1]*tXO[1]);
  eye.major =  sqrt(tYO[0]*tYO[0] + tYO[1]*tYO[1]);

  //TODO limit transform
  //if(eye.major < eye.minor){
  //  std::swap(eye.minor, eye.major);
  //}

#ifdef DEBUG_PRINT
  std::cout << "Eye center: " << eye.centerIndex << std::endl;
  std::cout << "Eye minor: "  << eye.minor << std::endl;
  std::cout << "Eye major: "  << eye.major << std::endl;

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
OpticNerveEstimator::OpticNerveEstimator::FitStem(
                OpticNerveEstimator::ImageType::Pointer inputImage,
                OpticNerveEstimator::Eye &eye,
                const std::string &prefix,
                bool alignStem ){

#ifdef DEBUG_PRINT
  std::cout << "--- Fit stem ---" << std::endl << std::endl;
#endif


  ////
  //A) Prepare moving image
  ////

#ifdef REPORT_TIMES
  clockStemA.Start();
#endif

  //-- Step 1
  //   Extract optic nerve region below the eye using the eye location and
  //   size estimates.

  ImageType::SpacingType imageSpacing = inputImage->GetSpacing();
  ImageType::RegionType imageRegion = inputImage->GetLargestPossibleRegion();
  ImageType::SizeType imageSize = imageRegion.GetSize();
  ImageType::PointType imageOrigin = inputImage->GetOrigin();


  ImageType::IndexType desiredStart;
  desiredStart[0] = eye.center[0] - algParams.stemXRegionFactor * eye.minor;
  desiredStart[1] = eye.center[1] + algParams.stemYRegionFactor * eye.major;

  ImageType::SizeType desiredSize;
  desiredSize[0] = 2 * algParams.stemXRegionFactor * eye.minor;
  desiredSize[1] =     algParams.stemYSizeFactor * eye.major;

  if(desiredStart[1] > imageSize[1] ){
    //std::cout << "Could not locate stem area" << std::endl;
    return false;
  }
  if(desiredStart[1] + desiredSize[1] > imageSize[1] ){
    desiredSize[1] = imageSize[1] - desiredStart[1];
  }

  if(desiredStart[0] < 0 ){
    desiredStart[0] = 0;
  }
  if(desiredStart[0] + desiredSize[0] > imageSize[0] ){
    desiredSize[0] = imageSize[0] - desiredStart[0];
  }


  ImageType::RegionType desiredRegion(desiredStart, desiredSize);
  stem.originalImageRegion = desiredRegion;

  ExtractFilter::Pointer extractFilter = ExtractFilter::New();
  //extractFilter->SetExtractionRegion(desiredRegion);
  extractFilter->SetRegionOfInterest(desiredRegion);
  extractFilter->SetInput( inputImage );
  //extractFilter->SetDirectionCollapseToIdentity();
  extractFilter->Update();
  ImageType::Pointer stemImageOrig = extractFilter->GetOutput();



  ImageType::RegionType stemRegion = stemImageOrig->GetLargestPossibleRegion();
  ImageType::SizeType stemSize = stemRegion.GetSize();
  ImageType::PointType stemOrigin = stemImageOrig->GetOrigin();
  ImageType::SpacingType stemSpacing = stemImageOrig->GetSpacing();
  ImageType::DirectionType stemDirection = stemImageOrig->GetDirection();

#ifdef DEBUG_PRINT
  std::cout << "Origin, spacing, size and index of stem image" << std::endl;
  std::cout << stemOrigin << std::endl;
  std::cout << stemSpacing << std::endl;
  std::cout << stemSize << std::endl;
  std::cout << stemRegion.GetIndex() << std::endl;
#endif


#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( stemImageOrig, catStrings(prefix, "-stem.tif") );
#endif



  //-- Step 2 through 3
  //   2. Gaussian smoothing
  //   3. Rescale individual rows to 0 100

  ITKFilterFunctions<ImageType>::SigmaArrayType sigma;
  sigma[0] = std::min( (double) stemSize[0], algParams.stemInitialSmoothXFactor * stemSpacing[0] );
  sigma[1] = std::min( (double) stemSize[1], algParams.stemInitialSmoothYFactor * stemSpacing[1] );
  //sigma[1] = stemSize[1]/12.0 * stemSpacing[1];
  ImageType::Pointer stemImage = ITKFilterFunctions<ImageType>::GaussSmooth( stemImageOrig, sigma);

  //Rescale indiviudal rows
  ITKFilterFunctions<ImageType>::RescaleRows(stemImage);


  stemImage = ITKFilterFunctions<ImageType>::Rescale(stemImage, 0, 100);


#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( stemImage, catStrings(prefix, "-stem-smooth.tif") );
#endif


  //-- Step 3.1 through 3.6
  //   3.1 Binary threshold
  //   3.2 Morphological opening
  //   3.3 Add vertical border
  //   3.4 Add small horizontal border
  //   3.5 Distance transform
  //   3.6 Calcuate inital optic nerve width and center

  ImageType::Pointer stemImageB =
	  ITKFilterFunctions<ImageType>::BinaryThreshold(stemImage, -1,
                                 algParams.stemInitialThreshold, 0, 100);

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( stemImageB, catStrings(prefix, "-stem-sd-thres.tif") );
#endif

  StructuringElementType structuringElement;
  structuringElement.SetRadius( std::min(imageSize[0], imageSize[1]) * algParams.stemOpeningRadiusFactor  );
  structuringElement.CreateStructuringElement();
  OpeningFilter::Pointer openingFilter = OpeningFilter::New();
  openingFilter->SetInput(stemImageB);
  openingFilter->SetKernel(structuringElement);
  openingFilter->SetForegroundValue(100.0);
  openingFilter->Update();
  stemImageB = openingFilter->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( stemImageB, catStrings(prefix, "-stem-morpho.tif") );
#endif

  CastFilter::Pointer stemCastFilter = CastFilter::New();
  stemCastFilter->SetInput( stemImageB );
  stemCastFilter->Update();
  UnsignedCharImageType::Pointer stemImage2 = stemCastFilter->GetOutput();

  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorder(stemImage2,
               std::min(imageSize[0], imageSize[1]) * algParams.stemVerticalBorderFactor );
  ITKFilterFunctions<UnsignedCharImageType>::AddHorizontalBorder(stemImage2, algParams.stemHorizontalBoder );

  SignedDistanceFilter::Pointer stemDistanceFilter = SignedDistanceFilter::New();
  stemDistanceFilter->SetInput( stemImage2 );
  stemDistanceFilter->SetInsideValue(100);
  stemDistanceFilter->SetOutsideValue(0);
  stemDistanceFilter->Update();
  ImageType::Pointer stemDistance = stemDistanceFilter->GetOutput();

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( stemDistance, catStrings(prefix, "-stem-distance.tif") );
#endif

  //Compute max of distance transfrom
  ImageCalculatorFilterType::Pointer stemCalculatorFilter  = ImageCalculatorFilterType::New ();
  stemCalculatorFilter->SetImage( stemDistance );
  stemCalculatorFilter->Compute();

  stem.initialWidth = stemCalculatorFilter->GetMaximum();

  if(stem.initialWidth <= 0 ){
     return false;
  }
  stem.initialCenterIndex = stemCalculatorFilter->GetIndexOfMaximum();
  stemImage->TransformIndexToPhysicalPoint(stem.initialCenterIndex, stem.initialCenter);

#ifdef DEBUG_PRINT
  std::cout << "Approximate width of stem: " <<  2 * stem.initialWidth << std::endl;
  std::cout << "Approximate stem center: " << stem.initialCenter << std::endl;
  std::cout << "Approximate stem center Index: " << stem.initialCenterIndex << std::endl;
#endif

  //-- Step 4
  //   Rescale rows left and right of the approximate center to 0 - 100

  float centerIntensity = stemImage->GetPixel( stem.initialCenterIndex );
#ifdef DEBUG_PRINT
  std::cout << "Approximate stem center intensity: " << centerIntensity << std::endl;
#endif
  for(int i=0; i<stemSize[1]; i++){
    ImageType::IndexType index;
    index[1] = i;
    float maxIntensityLeft = 0;
    for(int j=0; j<stem.initialCenterIndex[0]; j++){
      index[0] = j;
      maxIntensityLeft = std::max( stemImage->GetPixel(index), maxIntensityLeft );
    }
    for(int j=0; j<stem.initialCenterIndex[0]; j++){
      index[0] = j;
      float value = 0;
      if(maxIntensityLeft > centerIntensity ){
        value = ( stemImage->GetPixel(index) - centerIntensity ) /
		    ( maxIntensityLeft - centerIntensity )  ;
        value = std::max(0.f, value)*100;
        value = std::min(100.f, value);
      }
      stemImage->SetPixel(index, value );
    }

    float maxIntensityRight = 0;
    for(int j=stem.initialCenterIndex[0]; j<stemSize[0]; j++){
      index[0] = j;
      maxIntensityRight = std::max( stemImage->GetPixel(index), maxIntensityRight );
    }
    for(int j=stem.initialCenterIndex[0]; j<stemSize[0]; j++){
      index[0] = j;
      float value = 0;
      if(maxIntensityRight > centerIntensity ){
        value = ( stemImage->GetPixel(index) - centerIntensity ) /
		    ( maxIntensityRight - centerIntensity ) ;
        value = std::max(0.f, value) * 100;
        value = std::min(100.f, value);
      }
      stemImage->SetPixel(index, value  );
    }


  }

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( stemImage, catStrings(prefix, "-stem-scaled.tif") );
#endif



  //-- Step 5
  //   Binary threshold

  stemImage =   ITKFilterFunctions<ImageType>::BinaryThreshold(stemImage, -1,
                                             algParams.stemRegistrationThreshold, 0, 100);

#ifdef DEBUG_PRINT
  std::cout << "Stem threshold: " << algParams.stemRegistrationThreshold << std::endl;
#endif

/*
  StructuringElementType structuringElement2;
  structuringElement2.SetRadius( 10 );
  structuringElement2.CreateStructuringElement();
  OpeningFilter::Pointer openingFilter2 = OpeningFilter::New();
  openingFilter2->SetInput(stemImage);
  openingFilter2->SetKernel(structuringElement2);
  openingFilter2->SetForegroundValue(100.0);
  openingFilter2->Update();
  stemImage = openingFilter2->GetOutput();
*/




  //-- Step 5.1 through 5.4
  //  5.1 Add vertica border
  //  5.2 Add horizontal border
  //  5.3 Distance transform
  //  5.4 Refine intial estimates

  CastFilter::Pointer stemCastFilter2 = CastFilter::New();
  stemCastFilter2->SetInput( stemImage );
  stemCastFilter2->Update();
  UnsignedCharImageType::Pointer stemImage3 = stemCastFilter2->GetOutput();

  ITKFilterFunctions<UnsignedCharImageType>::AddVerticalBorder(stemImage3,
            algParams.stemRefineVerticalBorderFactor* std::min(imageSize[0], imageSize[1]) );
  ITKFilterFunctions<UnsignedCharImageType>::AddHorizontalBorder(stemImage3,
            algParams.stemHorizontalBoder );

  SignedDistanceFilter::Pointer stemDistanceFilter2 = SignedDistanceFilter::New();
  stemDistanceFilter2->SetInput( stemImage3 );
  stemDistanceFilter2->SetInsideValue(100);
  stemDistanceFilter2->SetOutsideValue(0);
  stemDistanceFilter2->Update();
  ImageType::Pointer stemDistance2 = stemDistanceFilter2->GetOutput();



#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( stemDistance, catStrings(prefix, "-stem-scaled-distance.tif") );
#endif

  //Compute max of distance transfrom
  ImageCalculatorFilterType::Pointer stemCalculatorFilter2  = ImageCalculatorFilterType::New ();
  stemCalculatorFilter2->SetImage( stemDistance2 );
  stemCalculatorFilter2->Compute();

  stem.initialWidth = stemCalculatorFilter2->GetMaximum();
  if(stem.initialWidth <= 0 ){
     return false;
  }
  stem.initialCenterIndex = stemCalculatorFilter2->GetIndexOfMaximum();
  stemImage->TransformIndexToPhysicalPoint(stem.initialCenterIndex, stem.initialCenter);

#ifdef DEBUG_PRINT
  std::cout << "Refined approximate width of stem: " <<  2 * stem.initialWidth << std::endl;
  std::cout << "Refined approximate stem center: " << stem.initialCenter << std::endl;
  std::cout << "Refined approximate stem center Index: " << stem.initialCenterIndex << std::endl;
#endif








  //-- Step 6
  //   Add a bit of smoothing for the registration process
  sigma[0] = algParams.stemRegsitrationSmooth * stemSpacing[0];
  sigma[1] = algParams.stemRegsitrationSmooth * stemSpacing[1];
  stemImage = ITKFilterFunctions<ImageType>::GaussSmooth(stemImage, sigma);


#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( stemImage, catStrings(prefix, "-stem-thres.tif") );
#endif

#ifdef REPORT_TIMES
  clockStemA.Stop();
#endif


  /////
  //B. Prepare fixed image.
  //  Create artifical stem image to fit to region of interest.
  /////

#ifdef REPORT_TIMES
  clockStemB.Start();
#endif

  //--Step 1 and C) 1
  //  Create a black and white image with two bars that
  //  are an intial estimate of the width apart.
  //  Create registration mask image.


  ImageType::Pointer moving = ImageType::New();
  moving->SetRegions(stemRegion);
  moving->Allocate();
  moving->FillBuffer( 0.0 );
  moving->SetSpacing(stemSpacing);
  moving->SetOrigin(stemOrigin);
  moving->SetDirection( stemDirection );

  UnsignedCharImageType::Pointer movingMask = UnsignedCharImageType::New();
  movingMask->SetRegions(stemRegion);
  movingMask->Allocate();
  movingMask->FillBuffer( itk::NumericTraits< unsigned char >::Zero);
  movingMask->SetSpacing(stemSpacing);
  movingMask->SetOrigin(stemOrigin);

  int stemYStart   = eye.initialRadiusY * (1 - algParams.stemYRegionFactor);
  int stemXStart1  = stem.initialCenterIndex[0] - 1.5 * stem.initialWidth / stemSpacing[0];
  int stemXEnd1    = stem.initialCenterIndex[0] - 1 * stem.initialWidth / stemSpacing[0];
  int stemXStart2  = stem.initialCenterIndex[0] + 1 * stem.initialWidth / stemSpacing[0];
  int stemXEnd2    = stem.initialCenterIndex[0] + 1.5 * stem.initialWidth / stemSpacing[0];


  if(stemXStart1 < 0){
    stemXStart1 = 0;
  }
  if( stemXEnd2 >= stemSize[0] ){
    stemXEnd2 = stemSize[0];
  }
  if(stemXEnd2 < stemXStart2){
    //std::cout << "Failed to locate stem" << std::endl;
    return false;
  }

  for(int i=stemYStart; i<stemSize[1]; i++){
     ImageType::IndexType index;
     index[1] = i;
     for(int j=stemXStart1; j<stemXEnd2; j++){
       index[0]=j;
       movingMask->SetPixel(index, 255);
     }

     for(int j=stemXStart1; j<stemXEnd1; j++){
       index[0]=j;
       moving->SetPixel(index, 100.0);
     }
     for(int j=stemXStart2; j<stemXEnd2; j++){
       index[0]=j;
       moving->SetPixel(index, 100.0);
     }
  }

#ifdef DEBUG_IMAGES
  ImageIO<UnsignedCharImageType>::WriteImage( movingMask, catStrings(prefix, "-stem-mask.tif") );
#endif


  //-- Step 2
  //   Gauss smoothing


  sigma[0] = algParams.stemRegsitrationSmooth * stemSpacing[0];
  sigma[1] = algParams.stemRegsitrationSmooth * stemSpacing[1];
  moving = ITKFilterFunctions<ImageType>::GaussSmooth(moving, sigma);

#ifdef DEBUG_IMAGES
  ImageIO<ImageType>::WriteImage( moving, catStrings( prefix, "-stem-moving.tif" ) );
#endif

#ifdef REPORT_TIMES
  clockStemB.Stop();
#endif

  ////
  //C. Registration of artifical stem image to threhsold stem image
  ////

#ifdef REPORT_TIMES
  clockStemC1.Start();
#endif

  //-- Step 2 (Step 1 was inclued in B)
  //   Similarity transfrom registration centered on the fixed bars image



  SimilarityTransformType::Pointer transform = SimilarityTransformType::New();
  transform->SetCenter( stem.initialCenter );


  MetricType::Pointer         metric        = MetricType::New();
  OptimizerType::Pointer      optimizer       = OptimizerType::New();
  InterpolatorType::Pointer   movingInterpolator  = InterpolatorType::New();
  InterpolatorType::Pointer   fixedInterpolator  = InterpolatorType::New();
  RegistrationType::Pointer   registration  = RegistrationType::New();

  optimizer->SetGradientConvergenceTolerance( 0.000001 );
  optimizer->SetLineSearchAccuracy( 0.5 );
  optimizer->SetDefaultStepLength( 0.00001 );
#ifdef DEBUG_PRINT
  optimizer->TraceOn();
#endif
  optimizer->SetMaximumNumberOfFunctionEvaluations( 20000 );


  //Using a Quasi-Newton method, make sure scales are set to identity to
  //not destory the approximation of the Hessian
  std::cout << transform->GetNumberOfParameters() << std::endl;
  OptimizerType::ScalesType scales( transform->GetNumberOfParameters() );
  scales[0] = 1.0;
  scales[1] = 1.0;
  scales[2] = 1.0;
  scales[3] = 1.0;
  optimizer->SetScales( scales );


  metric->SetMovingInterpolator( movingInterpolator );
  metric->SetFixedInterpolator( fixedInterpolator );

  MaskType::Pointer  spatialObjectMask = MaskType::New();
  spatialObjectMask->SetImage( movingMask );
  metric->SetFixedImageMask( spatialObjectMask );


  registration->SetMetric(        metric        );
  registration->SetOptimizer(     optimizer     );
  registration->SetMovingImage(    stemImage    );
  registration->SetFixedImage(   moving  );

#ifdef DEBUG_PRINT
  std::cout << "Transform parameters: " << std::endl;
  std::cout <<  transform->GetParameters()  << std::endl;
  std::cout <<  transform->GetCenter()  << std::endl;
#endif

  registration->SetInitialTransform( transform );

  RegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
  shrinkFactorsPerLevel.SetSize( 1 );
  //shrinkFactorsPerLevel[0] = 2;
  //shrinkFactorsPerLevel[1] = 1;
  shrinkFactorsPerLevel[0] = 1;

  RegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
  smoothingSigmasPerLevel.SetSize( 1 );
  //smoothingSigmasPerLevel[0] = 0.5;
  //smoothingSigmasPerLevel[1] = 0;
  smoothingSigmasPerLevel[0] = 0;

  registration->SetNumberOfLevels ( 1 );
  registration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
  registration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );

  //Do registration
  try{
	  registration->SetNumberOfThreads(1);
	  registration->Update();
  }
  catch( itk::ExceptionObject & err ){
#ifdef DEBUG_PRINT
	  std::cerr << "ExceptionObject caught !" << std::endl;
	  std::cerr << err << std::endl;
#endif
	  //return EXIT_FAILURE;
  }


#ifdef DEBUG_PRINT
  const double bestValue = optimizer->GetValue();
  std::cout << "Result = " << std::endl;
  std::cout << " Metric value  = " << bestValue          << std::endl;

  std::cout << "Registered transform parameters: " << std::endl;
  std::cout <<  registration->GetTransform()->GetParameters()  << std::endl;
  std::cout <<  transform->GetCenter()  << std::endl;
#endif

#ifdef REPORT_TIMES
  clockStemC1.Stop();
#endif

#ifdef REPORT_TIMES
  clockStemC2.Start();
#endif

  if(alignStem){
    SimilarityTransformType::Pointer inverse = SimilarityTransformType::New();
    transform->GetInverse( inverse );


    // Create registered bars image
    ResampleFilterType::Pointer resampler = ResampleFilterType::New();
    resampler->SetInput( moving );
    resampler->SetTransform( inverse );
    resampler->SetSize( stemSize );
    resampler->SetOutputOrigin(  stemOrigin );
    resampler->SetOutputSpacing( stemSpacing );
    resampler->SetOutputDirection( stemImage->GetDirection() );
    resampler->SetDefaultPixelValue( 0 );
    resampler->Update();
    ImageType::Pointer moved = resampler->GetOutput();

    stem.aligned = moved;


#ifdef DEBUG_IMAGES
    ImageIO<ImageType>::WriteImage(moved, catStrings(prefix, "-stem-registered.tif") );

    moved = ITKFilterFunctions<ImageType>::ThresholdAbove(moved, 5, 255);

    CastFilter::Pointer movingCast = CastFilter::New();
    movingCast->SetInput( moved );

    BinaryImageToLabelMapFilterType::Pointer binaryImageToLabelMapFilter = BinaryImageToLabelMapFilterType::New();
    binaryImageToLabelMapFilter->SetInput( movingCast->GetOutput() );
    binaryImageToLabelMapFilter->Update();

    LabelMapToLabelImageFilterType::Pointer labelMapToLabelImageFilter = LabelMapToLabelImageFilterType::New();
    labelMapToLabelImageFilter->SetInput(binaryImageToLabelMapFilter->GetOutput());
    labelMapToLabelImageFilter->Update();

    ImageType::Pointer stemOrigRescaled = ITKFilterFunctions<ImageType>::Rescale( stemImageOrig, 0, 255);
    LabelOverlayImageFilterType::Pointer labelOverlayImageFilter = LabelOverlayImageFilterType::New();
    labelOverlayImageFilter->SetInput( stemOrigRescaled );
    labelOverlayImageFilter->SetLabelImage(labelMapToLabelImageFilter->GetOutput());
    labelOverlayImageFilter->SetOpacity(.25);
    labelOverlayImageFilter->Update();



    ImageIO<RGBImageType>::WriteImage( labelOverlayImageFilter->GetOutput(), catStrings(prefix, "-stem-overlay.png") );
#endif
  }



  //-- Step 3
  //   Compute stem width by pushing intital width through the transform

  SimilarityTransformType::InputPointType tCenter;
  tCenter[0] = stem.initialCenter[0];
  tCenter[1] = stem.initialCenter[1];

  SimilarityTransformType::InputVectorType tX;
  tX[0] = stem.initialWidth;
  tX[1] = 0;

  stem.center = transform->TransformPoint(tCenter);
  stemImage->TransformPhysicalPointToIndex(stem.center, stem.centerIndex);

  SimilarityTransformType::OutputVectorType tXO = transform->TransformVector(tX, tCenter);

  stem.width =  sqrt(tXO[0]*tXO[0] + tXO[1]*tXO[1]);

#ifdef DEBUG_PRINT
  std::cout << "Stem center: " << stem.centerIndex << std::endl;
  std::cout << "Stem width: "  << stem.width*2 << std::endl;

  std::cout << "--- Done fitting stem ---" << std::endl << std::endl;
#endif

#ifdef REPORT_TIMES
  clockStemC2.Stop();
#endif

   return true;
};

