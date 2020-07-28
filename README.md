# CoreAPIs for VisMotive framework
Currently, only "static" build environments based on  visual studio (2017) are supported. ...

####Dependencies
Current build environment assumes the following structure of the developement folders. As external dependencies, our VisMotive-based projects use this core APIs and libraries for most of the volumetric and polygonal processing tasks. To be clear your folder structure should be something quite similar to:

    yourdevelfolder/
     |
     ├──CoreAPIs
     │   ├──CommonApi
     │   ├──CommonUnits
     │   └──GpuManager
     ├──bin (... providing binary files which do not require individual builds of projects)
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
