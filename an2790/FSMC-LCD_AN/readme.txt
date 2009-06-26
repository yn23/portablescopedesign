/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : readme.txt
* Author             : MCD Application Team
* Version            : V1.0.1
* Date               : 09/22/2008
* Description        : Description of the AN2790 "TFT LCD interfacing with the
*                      High-density STM32F10xxx FSMC" .
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

Example description
===================
This AN describes a firmware TFT LCD interfacing with the High Density STM32F10xxx 
FSMC peripheral. The main purpose of this firmware package is to provide resources 
facilitating the development of an application using the LCD on FSMC peripheral.

The firmware interface is composed of library source files developed in order to
support LCD features, an application demo is also provided.


Directory contents
==================
  + include
    - fonts.h              LCD fonts size definition
    - stm32f10x_conf.h     Library Configuration file
    - stm32f10x_it.h       Interrupt handlers header file
    - lcd.h                LCD Header file
    - fsmc_nand.h          FSMC NAND driver Header file
    - fsmc_nor.h           FSMC NOR driver Header file
    - fsmc_sram.h          FSMC SRAM driver Header file
    - main.h               Main Header file
    - menu.h               Menu Navigation Header file
    - sdcard.h             SD Card driver Header file      

  + source
    - stm32f10x_it.c       Interrupt handlers
    - main.c               Main program 
    - fsmc_nand.c          FSMC NAND driver firmware functions
    - fsmc_nor.c           FSMC NOR driver firmware functions
    - fsmc_sram.c          FSMC SRAM driver firmware functions
    - lcd.c                LCD driver firmware functions
    - sdcard.c             SD Card driver firmware functions
    - menu.c               Menu navigation firmware functions         


Hardware environment
====================
This demo runs on STMicroelectronics STM3210E-EVAL evaluation board and can be
easily tailored to any other hardware.
 
How to use it
=============
 + RVMDK
    - Open the FSMC-LCD.Uv2 project
    - Rebuild all files: Project->Rebuild all target files
    - Load project image: Debug->Start/Stop Debug Session
    - Run program: Debug->Run (F5)    

 + EWARMv5
    - Open the FSMC-LCD.eww workspace.
    - Rebuild all files: Project->Rebuild all
    - Load project image: Project->Debug
    - Run program: Debug->Go(F5)

 + RIDE
    - Open the FSMC-LCD.rprj project.
    - Rebuild all files: Project->build project
    - Load project image: Debug->start(ctrl+D)
    - Run program: Debug->Run(ctrl+F9)  

 + HiTOP
    - Open the HiTOP toolchain, a "using projects in HiTOP" window appears.
    - Select open an existing project.
    - Browse to open the FSMC-LCD.htp:
        - under STM32F10E_EVAL directory: to select the project for STM32 High-density devices
    - Rebuild all files: Project->Rebuild all
    - Click on ok in the "download project" window.
    - Run program: Debug->Go(F5).
    Note: 
          - It is mandatory to reset the target before loading the project into  
            target.
          - It is recommended to run the reset script (click on TR button in the
            toolbar menu) after loading the project into target.

NOTE:
 - High-density devices are STM32F101xx and STM32F103xx microcontrollers where
   the Flash memory density ranges between 256 and 512 Kbytes.               

******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE******
