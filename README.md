# Ultrasound Optic Nerve Estimation UI

A QT user interface for doing optic nerve width estimation.

Uses copied code from the UltrsoundOpticNerveEstimation repository.



# Requirements and Build

Build each in "Release" configuration and in the following order:
1. Qt5 (Yup, 5)
   + Use VS 2013 or 2015 and QT 5.8 make sure you use the QT installer that matches your VS version (otherwise you'll get a linker error).
     + with VS 2012 QT complains about c++11 compliancy.
     + VS 2017 complains about a QT bug thats fixed in 5.9 but with 5.9 VTK complains.   
2. ITK
   + github.com:/Kitware/ITK
3. IntersonArraySDK
4. IntersonArraySDKCxx (Must be INSTALLED - cannot use build dir)
   + github.com:/KitwareMedical/IntersonArraySDKCxx
5. UltrasoundOpticNerveUI
    + I had to copy IntersonArraySDKcxx.lib IntersonSDK.lib from their build dircetories to the build dir. They couldn't be found otherwise.
    + IntersonArrayCxxControlsHWControls.cxx has a line #using IntersonArray.dll, this requires that IntersonArray.dll is either in the LIBPATH or in directory the exectubale is run in. 
    + For running from commandline all dll's used also need ito be in the PATH (see below).
    + Run SeeMoreArraySetup.exe (from the IntersonArraySDK)

Usage:
UltrasoundOpticNerveUI

# Software Architecture 

This is a multithreaded architecture with three main threads.
1. The QT user interface thread that displayes the images (in OpticNerveUI. )
2. The image aqusition thread that stores images into a RingBuffer (IntersonArrayDevice.hxx)
3. The optic nerve width estimator that reads images from the ringbuffer and does the calculations (OpticNerveCalculator.hxx). This threads spawns multiple worker threads to estimate the width on multiple images in parallel.
