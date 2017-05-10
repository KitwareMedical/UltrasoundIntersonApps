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

// Print debug statements
// Using print because I am to inept to setup visual studio
#define DEBUG_PRINT

#pragma once

#ifndef OPTICNERVEUI_H 
#define OPTICNERVEUI_H

#include <QMainWindow>
#include <QTimer>
#include <QCheckBox>
#include <qgroupbox.h>
#include <QCloseEvent>


#include "ui_OpticNerve.h"
#include "ITKQtHelpers.hxx"
#include "OpticNerveCalculator.hxx"

//Forward declaration of Ui::MainWindow;
namespace Ui{
	class MainWindow;
}

//Declaration of OpticNerveUI
class OpticNerveUI: public QMainWindow
{
	Q_OBJECT

public:
  OpticNerveUI(int numberOfThreads, int bufferSize, QWidget *parent = nullptr);
  ~OpticNerveUI();

protected:
  void  closeEvent(QCloseEvent * event);

protected slots:
  /** Quit the application */
  void ConnectProbe();
  /** Start the application */
  //void SetFrequency();
  /** Update the images displayed from the probe */
  void UpdateImage();

  
private:
  /** Layout for the Window */
  Ui::MainWindow *ui;
  OpticNerveCalculator opticNerveCalculator;
  IntersonArrayDevice intersonDevice;
  QTimer *timer;

  int lastRendered;
};

#endif
