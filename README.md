# Micromouse
Micromouse is an event where small robot mice solve a 16x16 maze.


To make the project work on STM32 System Workbench (Eclipse):

1.Clone repository using Git Desktop or from the web   
2.In eclipse go to File->Import->Git->Projects from Git-> Existing local
repository->Add->Browse...& Search ->Finish   
3.Build the projoect (Hammer icon)  
4.Run->Run configurations->Ac6 STM32 Debugging->Search  Project->Micromouse.elf->Run  
5.Project is ready

To enable debugging:

1.In eclipse go to Run->Debug Configurations...
2.In bookmark Debugger change Script to Manual spec
3.Choose:
  Debug device: ST-LinkV2-1
  Debug interface: SWD
4.Apply & Debug
