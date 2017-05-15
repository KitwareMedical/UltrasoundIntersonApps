/*=========================================================================

Library:   UltrasoundSpectroscopyRecorder

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


#include "OpticNerveUI.h"

// Qt includes
#include <QApplication>
#include <QDebug>


// STD includes
#include <iostream>



int main(int argc,char *argv[])
{
  qDebug() << "Starting ...";
  QApplication app(argc,argv);
 
  int nThreads = 4; 
  int ringBufferSize = 20;
  OpticNerveUI window( nThreads, ringBufferSize, nullptr);
  window.show();

  try{
  return app.exec();
  }
  catch(std::exception &e){
    std::cout << e.what() << std::endl;
  }
  catch( ...  ){
    std::cout << "something failed" << std::endl;
  }
  
}
