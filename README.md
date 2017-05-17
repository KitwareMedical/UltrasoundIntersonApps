# Ultrasound Optic Nerve Estimation UI

A QT user interface for automatic optic nerve width estimation suing the INterson linear array ultrasound probe.

Uses copied code from the UltrsoundOpticNerveEstimation repository.



# Requirements and Build

Build each in "Release" configuration and in the following order:
1. Qt5 (Yup, 5)
   + Use VS 2013 or 2015 and QT 5.8 make sure you use the QT installer that matches your VS version (otherwise you'll get a linker error).
     + with VS 2012 QT complains about c++11 compliancy.
     + VS 2017 complains about a QT bug thats fixed in 5.9 but with 5.9 VTK complains.   
2. ITK
   + https://github.com/Kitware/ITK
3. IntersonArraySDK
   + From Interson (https://interson.com/)
   + To run the probe the Interson drivers need to be installed using SeeMoreArraySetup.exe (from the IntersonArraySDK)
4. IntersonArraySDKCxx (Must be INSTALLED - cannot use build dir)
   + https://github.com/KitwareMedical/IntersonArraySDKCxx
5. UltrasoundOpticNerveUI
    + For running from commandline all dll's used also need ito be in the PATH. 
      The CMakefiles.txt contains a package target that will collect all dlls and create a zip file (See creating a binary package).
    

Usage:
UltrasoundOpticNerveUI

## Creating a Binary Package

The CMakelists.txt conatins instructions for cpack to create a zip file that
includes all required dlls and the exectubale.

Running the target "package" (i.e. ninja package if you used ninja as your cmake generator) 
will create that zip file. 
 

# Software Architecture 

This is a multithreaded architecture with three main threads.
1. The QT user interface thread that displayes the images (in OpticNerveUI. ) by grabbing images from the ringbuffers in 2 and 3 below.
2. The image aqusition thread that stores images into a RingBuffer (IntersonArrayDevice.hxx)
3. The optic nerve width estimator that reads images from the ringbuffer and does the calculations (OpticNerveCalculator.hxx). This threads spawns multiple worker threads to estimate the width on multiple images in parallel adn stores the result ina ringbuffer.


# TODO
1. Cleanup code
2. Simplify optic nerev estimator. 
3. The OpticNerveEstimator is allocated and deallcoated every time (and all the filters required). Shouls set this up as a a pipeline 
4. The optic nerve width estimates are not stored in order (display of estimate sis out order)

## Done
1. A mutex lock (or maybe some other clever option) is required to guarantee that the optic nerve estimate is not done on the same image twice. (Is relatively unliley to happen and does not cause nay further harm but wasted computation)
