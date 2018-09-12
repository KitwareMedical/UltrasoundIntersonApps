/*=========================================================================

Library:   UltrasoundPTXRecorder

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

#pragma once

#ifndef PTXPATIENTDATA_H
#define PTXPATIENTDATA_H

#include "itkImage.h"

class PTXPatientData{

  public:
   
    PTXPatientData()
      {
      id = "";
      diagnosis = "No-PTX";
      filepath = "";
      session = 0;
      age = 0;
      };
 
    typedef itk::Image<double, 3> ImageType;

    std::string id;
    unsigned int age;
    unsigned int session;
    std::string diagnosis;
    std::string filepath;
    
    ImageType::Pointer movie;    

};

#endif
