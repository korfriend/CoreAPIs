# CoreAPIs for VisMotive framework
<br/>
Currently, only "static" build environments based on  visual studio (2017) are supported. ...

### Build environment
Current build environment assumes the following structure of the developement folders. As external dependencies, our VisMotive-based projects use this core APIs and libraries for most of the volumetric and polygonal processing tasks. To be clear your folder structure should be something quite similar to:

    yourdevelfolder/
     |
     ├──CoreAPIs (https://github.com/korfriend/VisMotive-CoreAPIs/)
     │   ├──CommonApi
     │   ├──CommonUnits
     │   └──GpuManager
     ├──bin (our projects use these debug/release folders as external libs folder)
     │   ├──X64_Debug
     │   └──X64_Release
     └──External Projects (based on our VisMotive data structures and libraries) 
         ├──project1
         ├──project2
         ├──project3
         ├──...
         └──...

...

##### Compiling
...

##### Platform specific notes
...
# Local Isosurface Modeler

<br/>
This project is for a high-fidelity surface extraction technique for volumetric (CT) scans, which is based on the paper entitled "Confidence-controlled Local Isosurfacing" (under review. minor revisions in TVCG).
Detailed "getting started" will be prepared with an example code project. 
You can download the code by using Git and cloning the repository, or downloading it as zip. This will give you the full C++ source code that you must build for yourself. 

### NOTICE:
