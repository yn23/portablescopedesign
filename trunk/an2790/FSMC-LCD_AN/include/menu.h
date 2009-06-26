/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : menu.h
* Author             : MCD Application Team
* Version            : V1.0.1
* Date               : 09/22/2008
* Description        : This file contains all the functions prototypes for the
*                      menu navigation firmware driver.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MENU_H
#define __MENU_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_lib.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define  MAX_MENU_LEVELS 4
#define  NOKEY  0
#define  SEL    1
#define  RIGHT  2
#define  LEFT   3
#define  UP     4
#define  DOWN   5
#define  KEY    6

#define SLIDE_SIZE    76866 
#define SLIDE1		  0x64000000
#define SLIDE2		  0x64025842

/* Module private variables --------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define countof(a) (sizeof(a) / sizeof(*(a)))

/* Private functions ---------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Menu_Init(void);
void DisplayMenu(void);
void SelFunc(void);
void UpFunc(void);
void DownFunc(void);
void ReturnFunc(void);
u8 ReadKey(void);
void IdleFunc(void);
void DisplayIcons(void);
void LCD_NORDisplay(u32 address);
void ShowMenuIcons(void);
void STM32_LCD_DemoIntro(void);
u32 CheckBitmapFiles(void);
void InternalFlashToLCD(void);
void InternalFlashToLCD_DMA(void);
void InternalFlashToLCD_Speed(void);
void NORFlashToLCD(void);
void NORFlashToLCD_DMA(void);
void NORFlashToLCD_Speed(void);
void NANDFlashToLCD(void);
void NANDFlashToLCD_Speed(void);
void ExternalSRAMToLCD(void);
void ExternalSRAMToLCD_DMA(void);
void ExternalSRAMToLCD_Speed(void);
void SDCardToLCD(void);
void SDCardToLCD_Speed(void);
void TimingMeasurement_Config(void);
void DisplayTimingCompute(void);
void CopyToInternalFlash(void);
void CopyToExternalSRAM(void);
void CopyToNANDFlash(void);
void CopyToSDCard(void);
void NAND_PhysicalErase(void);

#endif /* __MENU_H */

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
