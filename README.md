# CoreAPIs for VisMotive framework
<br/>
Currently, only "static" build environments based on  visual studio (2017) are supported. ...

### Build environment
Current build environment assumes the following structure of the developement folders. As external dependencies, our VisMotive-based projects use this core APIs and libraries for most of the volumetric and polygonal processing tasks. To be clear your folder structure should be something quite similar to:

    yourdevfolder/
     |
     ├──CoreAPIs (https://github.com/korfriend/VisMotive-CoreAPIs/)
     │   ├──CommonApi
     │   ├──CommonUnits (including GL math folder)
     │   └──GpuManager
     ├──bin (our projects use these debug/release folders as external libs folders)
     │   ├──X64_Debug
     │   └──X64_Release
     └──External Projects (based on our VisMotive data structures and libraries) 
         ├──project1
         ├──project2
         ├──project3
         ├──...
         └──...


### Dependencies
Only GL math is used, which is included in CommonUnits folder. If you want to use this CoreAPIs library as binary libs, then import the following files
- Scenario 1 for developing VisMotive-based modules 
    - Header files : "helpers.h" and "VimCommon.h" 
    - Libraty files : "CommonUnits.lib" and "GpuManager.lib (optional for GPU rendering engine developer)"
    - DLL files : "CommonUnits.dll" and "GpuManager.dll (optional for GPU rendering engine developer)"
- Scenario 2 for developing application that uses VisMotive-based modules and libraries 
    - Header file : "VisMtvApi.h" 
    - Libraty file : "CommonApi.lib"
    - DLL files : "CommonApi.dll", "CommonUnits.dll", "GpuManager.dll", and specific module dlls

### Next plan
Some comments for explaning source codes are written in Korean. I will replace the language to English as default language. Examples and sample codes will be available as individual projects.
