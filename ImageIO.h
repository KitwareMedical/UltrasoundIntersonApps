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




#ifndef IMAGEIO_H
#define IMAGEIO_H


#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>



#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageSeriesReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkCastImageFilter.h"


#include <vector>

template <typename TImage>
class ImageIO{


  public:

    typedef TImage Image;
    typedef typename Image::Pointer ImagePointer;
    typedef typename Image::ConstPointer ImageConstPointer;
    typedef typename Image::IndexType ImageIndex;
    typedef typename Image::RegionType ImageRegion;
    typedef typename ImageRegion::SizeType ImageSize;
    typedef typename ImageSize::SizeValueType ImageSizeValue;
    typedef typename Image::SpacingType ImageSpacing;
    typedef typename Image::PixelType Precision;

    typedef typename itk::ImageFileReader<Image> ImageReader;
    typedef typename ImageReader::Pointer ImageReaderPointer;

    typedef typename itk::ImageSeriesReader<Image> ImageSeriesReader;
    typedef typename ImageSeriesReader::Pointer ImageSeriesReaderPointer;

    typedef typename itk::ImageFileWriter<Image> ImageWriter;
    typedef typename ImageWriter::Pointer ImageWriterPointer;

    typedef typename itk::ImageRegionIteratorWithIndex<Image> ImageRegionIteratorWithIndex;
    typedef typename itk::ImageRegionIterator<Image> ImageRegionIterator;

    typedef typename itk::CastImageFilter<Image, Image> CastFilter;
    typedef typename CastFilter::Pointer CastFilterPointer;

  //Save an image from a vector
  static void WriteImage(ImagePointer image, const std::string &filename){
    ImageWriterPointer writer = ImageWriter::New();
    writer->SetFileName(filename);
    writer->SetInput(image);
    writer->Update();
  };




  static ImagePointer ReadImage(const std::string &filename){
    ImagePointer input = NULL;
    ImageReaderPointer imageReader = ImageReader::New();
    imageReader->SetFileName( filename );
    imageReader->Update();
    input = imageReader->GetOutput();
    return input;
  };


  static ImagePointer CopyImage(ImagePointer image){
    CastFilterPointer cast = CastFilter::New();
    cast->SetInput(image);
    cast->Update();
    return cast->GetOutput();  
  };

  //Save an image from a vector
  static void saveImage(ImagePointer image, const std::string &filename){
    ImageWriterPointer writer = ImageWriter::New();
    writer->SetFileName(filename);
    writer->SetInput(image);
    writer->Update();
  };




  static ImagePointer readImage(const std::string &filename){
    ImagePointer input = NULL;
    ImageReaderPointer imageReader = ImageReader::New();
    imageReader->SetFileName( filename );
    imageReader->Update();
    input = imageReader->GetOutput();
    return input;
  };


  static ImagePointer copyImage(ImagePointer image){
    CastFilterPointer cast = CastFilter::New();
    cast->SetInput(image);
    cast->Update();
    return cast->GetOutput();  
  };



};


#endif
