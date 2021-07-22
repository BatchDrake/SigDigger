# SigDigger - The free digital signal analyzer
SigDigger is a free digital signal analyzer for GNU/Linux and macOS, designed to extract information of unknown radio signals. It supports a variety of SDR devices through SoapySDR, and allows adjustable demodulation of FSK, PSK and ASK signals, decode analog video, analyze bursty signals and listen to analog voice channels (all in real time).

![](Screenshots/0.2.0.png) 

## Wait, why does it look like Gqrx?
Because I'm a terrible person. Also, because after dealing with a lot of software of the sort, I realized that Gqrx had the best UI of them all: minimalistic yet operative. Earlier versions of the UI were somewhat different, but after a lot of debugging I came to the conclusion that it would be better off if I just tried to mimic existing (and successful) software, reducing the learning curve to the new feature set.

You may notice that the spectrum widget looks a lot like Gqrx's. This is because the specturm widget is actually Gqrx's with minimal modifications (like support for configurable Waterfall palettes). I tried to code my own Waterfall widget in Suscan and Xorg ended up hogging the CPU, so I'm not reinventing the wheel anymore: I decided to extend the existing Gqrx's Plotter widget so it fits SigDigger's set of features.

Apart from the UI layout and the plotter widget, SigDigger bears little resemblance to Gqrx: Gqrx depends on GNU Radio, while SigDigger is a Qt5 frontend for Suscan. The DSP chains are totally different and Suscan's thread model is optimized for a very specific set of tasks.

## How am I supposed to compile this?
SigDigger depends on three different projects: **Sigutils**, **Suscan** and **SuWidgets**. You need to build and install these projects in your computer prior to compile SigDigger.

* Sigutils build instructions can be found [here](https://github.com/BatchDrake/sigutils/blob/master/README.md).
* Suscan build instructions can be found [here](https://github.com/BatchDrake/suscan/blob/master/README.md).

Before even attempting to run `cmake`, you may want to decide which branch you want to build. If you build from `master` (this is the default for SigDigger and all its dependencies), you will get a fresh executable of the latest stable release of SigDigger. If you build from `develop` (by running `git fetch origin develop` and `git checkout develop` in all four projects, right after `git clone`), you will get SigDigger with the latest experimental features. Plese note that the `develop` branch is still under validation, so if you want to build from it, expect bugs, inconsistent behaviors and crashes. Any feedback is welcome though.

After successfully building Sigutils and Suscan, you can now proceed to build **SuWidgets**. SuWidgets is Qt 5 graphical library containing all SigDigger's custom widgets. In order to build it, ensure you have the Qt 5 (>= 5.9) development framework installed in your system and then run:

```
% git clone https://github.com/BatchDrake/SuWidgets
% cd SuWidgets
% qmake SuWidgetsLib.pro
% make
% sudo make install
```

If you the above steps were successful, chances are that you will success on building SigDigger too. In order to build SigDigger, do:

```
% git clone https://github.com/BatchDrake/SigDigger
% cd SigDigger
% qmake SigDigger.pro
% make
% sudo make install
```

And, in order to run SigDigger, just type:

```
% SigDigger
```

If the command above fails, it is possible that you got SigDigger installed somewhere else, like /opt/SigDigger. In that case, you can try to run instead:

```
% /opt/SigDigger/bin/SigDigger
```

## Precompiled releases
You can find precompiled releases under the "Releases" tab in this repository. For the time being, these releases are meant for x64 Linux only (preferably Debian-like distributions) and have been minimally tested. Although I have plans to port Sigutils, Suscan and SigDigger to other platforms, I'd like to have a stable codebase before going any further.

Obviously, your feedback will make my job easier. If you experience any issues building or using SigDigger, go ahead and use the fantastic GitHub's bug tracking system. It works pretty well and it is way easier for me to keep track of the existing issues.

Looking forward for your feedback! :)

---
73 de EA1IYR
