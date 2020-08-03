# CoreAPIs for VisMotive framework
[![Build status][s1]][av] [![License: MIT][s3]][li]

[s1]: https://ci.appveyor.com/api/projects/status/vrq0m6iklgf95gui?svg=true
[s3]: https://img.shields.io/badge/License-MIT-orange.svg

[av]: https://ci.appveyor.com/project/korfriend/vismotive-coreapis
[li]: https://opensource.org/licenses/MIT

<br/>
VisMotive framework is designed for open-source scientific processing tools based on point clouds, polygonal models, and volumetric models. We provide scientific visualation tools (back- and front- end) written in C++. It is simple and easy to use. Our goal is to provide simple APIs for front-end developers while allowing back-end developers (who develope and research specific tasks as separate modules) to connect their modules to front-end easily. This project is hosted on <a href="https://github.com/korfriend/">GitHub</a>.

### Requirements:
- Windows 10
- Visual Studio 2017 or higher

### Build Environments
Current build environment assumes the following structure of the developement folders. As external dependencies, our VisMotive-based projects use this core APIs and libraries for most of the volumetric and polygonal processing tasks. To be clear your folder structure should be something quite similar to:

    yourdevfolder/
     |
     ├──CoreAPIs (https://github.com/korfriend/VisMotive-CoreAPIs)
     │   ├──CommonApi
     │   ├──CommonUnits (including GL math folder)
     │   └──GpuManager
     ├──bin (built files are available in https://github.com/korfriend/VisMotive-BuiltBinary)
     │   ├──X64_Debug
     │   └──X64_Release
     └──External Projects (based on our VisMotive data structures and libraries) 
         ├──project1 (e.g., https://github.com/korfriend/LocalIsosurfaceModeler)
         ├──project2
         ├──project3
         ├──...
         └──...


### Dependencies
Only GL math is used, which is included in CommonUnits folder. If you want to use this CoreAPIs library as binary libs, then import the following files
- Scenario 1 (back-end developers) for developing VisMotive-based modules 
    - Header files : "helpers.h" and "VimCommon.h" 
    - Libraty files : "CommonUnits.lib" and "GpuManager.lib (optional for GPU rendering engine developer)"
    - DLL files : "CommonUnits.dll" and "GpuManager.dll (optional for GPU rendering engine developer)"
- Scenario 2 (front-end developers) for developing applicatiosn that uses VisMotive-based modules and libraries 
    - Header file : "VisMtvApi.h" 
    - Libraty file : "CommonApi.lib"
    - DLL files : "CommonApi.dll", "CommonUnits.dll", "GpuManager.dll", and specific module dlls

### What Nex?!
- Some comments for explaning source codes are written in Korean. I will replace the language to English as default language. 
- Examples and sample codes will be available as individual projects.
