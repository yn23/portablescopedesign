/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : menu.c
* Author             : MCD Application Team
* Version            : V1.0.1
* Date               : 09/22/2008
* Description        : This file includes the menu navigation driver for the
*                      STM3210E-EVAL demonstration.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef void (* tMenuFunc)(void);
typedef struct sMenuItem * tMenuItem;
typedef struct sMenu * tMenu;

/* Private define ------------------------------------------------------------*/
#define NumberOfIcons        5

#define BlockSize            512 /* Block Size in Bytes */
#define BufferWordsSize      (BlockSize >> 2)

#define Bank1_SRAM_ADDR      0x68000000
#define Bank1_NOR_ADDR       0x64000000
#define LCD_RAM_ADDR         0x6C000002
#define Bank1_NAND_ADDR      0x70000000
#define START_ADDR           0x08019000
#define FLASH_PAGE_SIZE      0x800

#define NAND_OK   0
#define NAND_FAIL 1
#define MAX_PHY_BLOCKS_PER_ZONE  1024
#define MAX_LOG_BLOCKS_PER_ZONE  1000

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u8 MenuItemIndex = 0, nMenuLevel = 0;
u8 ItemNumb[MAX_MENU_LEVELS];

tMenuItem psMenuItem, psCurrentMenuItem;
tMenu psPrevMenu[MAX_MENU_LEVELS];
tMenu psCurrentMenu;

struct sMenuItem
{
  u8* pszTitle;
  tMenuFunc pfMenuFunc;
  tMenuFunc pfUpDownMenuFunc;
  tMenu psSubMenu;
};

struct sMenu
{
  u8* pszTitle;
  tMenuItem psItems;
  u8 nItems;
};

typedef struct __SPARE_AREA
{
  u16 LogicalIndex;
  u16 DataStatus;
  u16 BlockStatus;
} SPARE_AREA;

SD_CardInfo SDCardInfo;
NOR_IDTypeDef NOR_ID;
SD_Error Status = SD_OK;
NAND_ADDRESS wAddress;

extern vu32 DMAComplete;
extern vu32 FirstEntry;

u32 Buffer_Block_Rx[BufferWordsSize * 20];
u8 pBuffer[16384], TxBuffer[256];
vu32 DMASourceAddress = 0;
vu32 fractionaltimeunits = 0;

uc32 IconsAddr[] = {0x6404B084, 0x6404D0C6, 0x6404F108, 0x6405114A, 0x6405318C};
u8 SlidesCheck[6] = {0x42, 0x4D, 0x42, 0x58, 0x02, 0x00};
u8 Icons64Check[6] = {0x42, 0x4D, 0x42, 0x20, 0x00, 0x00};

/*------------------------------ Menu level 4 -------------------------------*/

/*------------------------------ Menu level 3 -------------------------------*/

/*------------------------------ Menu level 2 -------------------------------*/

struct sMenuItem SDCardMenuItems[] = {{" Copy images in SD  ", CopyToSDCard, IdleFunc},
                                      {"  SD CARD->CPU->LCD ", SDCardToLCD, IdleFunc},
                                      {" SD CARD->LCD Speed ", SDCardToLCD_Speed, IdleFunc},
                                      {"       Return       ", ReturnFunc, IdleFunc}};
struct sMenu SDCardMenu = {"      SD Card       ", SDCardMenuItems, countof(SDCardMenuItems)};

struct sMenuItem NANDFlashMenuItems[] = {{"Copy images in NAND ", CopyToNANDFlash, IdleFunc},
                                         {"  NAND ->CPU->LCD   ", NANDFlashToLCD, IdleFunc},
                                         {"   NAND->LCD Speed  ", NANDFlashToLCD_Speed, IdleFunc},
                                         {"NAND Physical Format", NAND_PhysicalErase, IdleFunc},
                                         {"       Return       ", ReturnFunc, IdleFunc}};
struct sMenu NANDFlashMenu = {"      NAND FLASH    ", NANDFlashMenuItems, countof(NANDFlashMenuItems)};

struct sMenuItem ExternalSRAMMenuItems[] = {{"Copy images in SRAM ", CopyToExternalSRAM, IdleFunc},
                                            {" EXT SRAM->CPU->LCD ", ExternalSRAMToLCD, IdleFunc},
                                            {" EXT SRAM->DMA->LCD ", ExternalSRAMToLCD_DMA, IdleFunc},
                                            {"EXT SRAM->LCD Speed ", ExternalSRAMToLCD_Speed, IdleFunc},
                                            {"       Return       ", ReturnFunc, IdleFunc}};
struct sMenu ExternalSRAMMenu = {"    External SRAM   ", ExternalSRAMMenuItems, countof(ExternalSRAMMenuItems)};


struct sMenuItem NORFlashMenuItems[] = {{" NOR FLASH->CPU->LCD", NORFlashToLCD, IdleFunc},
                                        {" NOR FLASH->DMA->LCD", NORFlashToLCD_DMA, IdleFunc},
                                        {"NOR FLASH->LCD Speed", NORFlashToLCD_Speed, IdleFunc},
                                        {"       Return       ", ReturnFunc, IdleFunc}};
struct sMenu NORFlashMenu = {"      NOR FLASH     ", NORFlashMenuItems, countof(NORFlashMenuItems)};

struct sMenuItem InternalFlashMenuItems[] = {{"Copy images in FLASH", CopyToInternalFlash, IdleFunc},
                                             {"INT FLASH->CPU->LCD ", InternalFlashToLCD, IdleFunc},
                                             {"INT FLASH->DMA->LCD ", InternalFlashToLCD_DMA, IdleFunc},
                                             {"INT FLASH->LCD Speed", InternalFlashToLCD_Speed, IdleFunc},
                                             {"       Return       ", ReturnFunc, IdleFunc}};
struct sMenu InternalFlashMenu = {"    Internal FLASH  ", InternalFlashMenuItems, countof(InternalFlashMenuItems)};

/*------------------------------ Menu level 1 -------------------------------*/
struct sMenuItem MainMenuItems[] = {
  {"    Internal FLASH  ", IdleFunc, IdleFunc, &InternalFlashMenu},
  {"      NOR FLASH     ", IdleFunc, IdleFunc, &NORFlashMenu},
  {"    External SRAM   ", IdleFunc, IdleFunc, &ExternalSRAMMenu},
  {"      NAND FLASH    ", IdleFunc, IdleFunc, &NANDFlashMenu},
  {"      SD Card       ", IdleFunc, IdleFunc, &SDCardMenu},
};
struct sMenu MainMenu = {"     Main menu      ", MainMenuItems, countof(MainMenuItems)};

/* Private function prototypes -----------------------------------------------*/
static u8 Buffercmp(u8* pBuffer1, u8* pBuffer2, u16 BufferLength);
static SD_Error LCD_SDDisplay(u32 address);
static SD_Error LCD_WriteSD(u32 address, u32 *pointer);
static u32 LCD_NANDDisplay(u32 address);
static void LCD_WriteDMA(u32 address);
static u16 NAND_Write(u32 Memory_Offset, u8 *Writebuff, u16 Transfer_Length);
static u16 NAND_Read(u32 Memory_Offset, u8 *Readbuff, u16 Transfer_Length);
static NAND_ADDRESS NAND_GetAddress (u32 Address);
static SPARE_AREA ReadSpareArea (u32 address);
static NAND_ADDRESS NAND_ConvertPhyAddress (u32 Address);
static void Media_BufferRead(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead);
static u32 InternalFlashCheckBitmapFiles(void);
static u32 ExternalSRAMCheckBitmapFiles(void);

/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : Menu_Init
* Description    : Initializes the navigation menu.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void Menu_Init(void)
{
  psCurrentMenu = &MainMenu;
  psPrevMenu[nMenuLevel] = psCurrentMenu;
  psMenuItem = MainMenuItems;
}

/*******************************************************************************
* Function Name  : DisplayMenu
* Description    : Displays the current menu.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DisplayMenu(void)
{
  u32 Line = 0, index = 0;
  tMenuItem psMenuItem2;

  /* Set the Back Color */
  LCD_SetBackColor(Blue);
  /* Set the Text Color */
  LCD_SetTextColor(White);

  /* Clear the LCD Screen */
  LCD_Clear(White);

  LCD_DisplayStringLine(Line, psCurrentMenu->pszTitle);
  Line += 24;

  /* Set the Back Color */
  LCD_SetBackColor(White);

  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  while(!(index >= (psCurrentMenu->nItems)))
  {
    psMenuItem2 = &(psCurrentMenu->psItems[index]);
    LCD_DisplayStringLine(Line, psMenuItem2->pszTitle);
    index++;
    Line += 24;
  }
  /* Set the Back Color */
  LCD_SetBackColor(Green);

  /* Set the Text Color */
  LCD_SetTextColor(White);

  /* Get the current menu */
  psMenuItem = &(psCurrentMenu->psItems[MenuItemIndex]);

  LCD_DisplayStringLine(((MenuItemIndex + 1) * 24), psMenuItem->pszTitle);
}

/*******************************************************************************
* Function Name  : SelFunc
* Description    : This function is executed when "SEL" push-buttton is pressed.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SelFunc(void)
{
  psCurrentMenuItem = psMenuItem;

  if(psMenuItem->psSubMenu != '\0')
  {
    /* Update the current Item by the submenu */
    MenuItemIndex = 0;
    psCurrentMenu = psMenuItem->psSubMenu;
    psMenuItem = &(psCurrentMenu->psItems)[MenuItemIndex];
    DisplayMenu();
    nMenuLevel++;
    psPrevMenu[nMenuLevel] = psCurrentMenu;
  } 
  psCurrentMenuItem->pfMenuFunc();
}

/*******************************************************************************
* Function Name  : UpFunc
* Description    : This function is executed when any of "UP" push-butttons
*                  is pressed.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UpFunc(void)
{
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  psMenuItem = &psCurrentMenu->psItems[MenuItemIndex];
  LCD_DisplayStringLine(((MenuItemIndex + 1) * 24), psMenuItem->pszTitle);

  if(MenuItemIndex > 0)
  {
    MenuItemIndex--;
  }
  else
  {
    MenuItemIndex = psCurrentMenu->nItems - 1;
  }
  /* Set the Back Color */
  LCD_SetBackColor(Green);
  /* Set the Text Color */
  LCD_SetTextColor(White);
  psMenuItem = &psCurrentMenu->psItems[MenuItemIndex];
  LCD_DisplayStringLine(((MenuItemIndex + 1) * 24), psMenuItem->pszTitle);
  ItemNumb[nMenuLevel] = MenuItemIndex;
}

/*******************************************************************************
* Function Name  : DownFunc
* Description    : This function is executed when any of "Down" push-butttons
*                  is pressed.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DownFunc(void)
{
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  psMenuItem = &psCurrentMenu->psItems[MenuItemIndex];
  LCD_DisplayStringLine(((MenuItemIndex + 1) * 24), psMenuItem->pszTitle);
      
  /* Test on the MenuItemIndex value before incrementing it */
  if(MenuItemIndex >= ((psCurrentMenu->nItems)-1))
  {
    MenuItemIndex = 0;
  }
  else
  {
    MenuItemIndex++;
  }
  /* Set the Back Color */
  LCD_SetBackColor(Green);
  /* Set the Text Color */
  LCD_SetTextColor(White);
  /* Get the current menu */
  psMenuItem = &(psCurrentMenu->psItems[MenuItemIndex]);
  LCD_DisplayStringLine(((MenuItemIndex + 1) * 24), psMenuItem->pszTitle);
  ItemNumb[nMenuLevel] = MenuItemIndex;
}

/*******************************************************************************
* Function Name  : ReturnFunc
* Description    : This function is executed when the "RETURN" menu is selected.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ReturnFunc(void)
{
  psMenuItem->pfUpDownMenuFunc();

  if(nMenuLevel == 0)
  {
    nMenuLevel++;
  }

  psCurrentMenu = psPrevMenu[nMenuLevel-1];
  psMenuItem = &psCurrentMenu->psItems[0];
  ItemNumb[nMenuLevel] = 0;
  MenuItemIndex = 0;
  nMenuLevel--;

  if(nMenuLevel != 0)
  {
    DisplayMenu();
  }
  else
  {
    ShowMenuIcons();
  }
}

/*******************************************************************************
* Function Name  : ReadKey
* Description    : Reads key from demoboard.
* Input          : None
* Output         : None
* Return         : Return RIGHT, LEFT, SEL, UP, DOWN or NOKEY
*******************************************************************************/
u8 ReadKey(void)
{
  /* "right" key is pressed */
  if(!GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_13))
  {
    while(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_13) == Bit_RESET)
    {
    } 
    return RIGHT; 
  }	
  /* "left" key is pressed */
  if(!GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_14))
  {
    while(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_14) == Bit_RESET)
    {
    }
    return LEFT; 
  }
  /* "up" key is pressed */
  if(!GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_15))
  {
    while(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_15) == Bit_RESET)
    {
    }
    return UP; 
  }
  /* "down" key is pressed */
  if(!GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_3))
  {
    while(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_3) == Bit_RESET)
    {
    } 
    return DOWN; 
  }
  /* "sel" key is pressed */
  if(!GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_7))
  {
    while(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_7) == Bit_RESET)
    {
    } 
    return SEL; 
  }
  /* "KEY" key is pressed */
  if(!GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8))
  {
    while(GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_8) == Bit_RESET)
    {
    } 
    return KEY; 
  }
  /* No key is pressed */
  else 
  {
    return NOKEY;
  }
}

/*******************************************************************************
* Function Name  : IdleFunc
* Description    : Idle function.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void IdleFunc(void)
{
  /* Nothing to execute: return */
  return;
}

/*******************************************************************************
* Function Name  : DisplayIcons
* Description    : Display menu icons.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DisplayIcons(void)
{
  u32 i = 0, j = 0, l = 0,  iconline = 0, iconcolumn = 0;
  
  iconline = 98;
  iconcolumn = 290 ;
    
  for(i = 0; i < 3; i++)
  {
    for(j = 0; j < 4; j++)
    {
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE); 
      LCD_SetDisplayWindow(iconline, iconcolumn, 64, 64);
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, DISABLE); 
      LCD_NORDisplay(IconsAddr[l]);
      iconcolumn -= 65;
      l++;
      if(l == NumberOfIcons)
      {
        return;
      }
    }
    iconline += 65;
    iconcolumn = 290;
  }
}

/*******************************************************************************
* Function Name  : ShowMenuIcons
* Description    : Show the main menu icon.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ShowMenuIcons(void)
{
  u32 MyKey = 0, i = 0;  
  u16 IconRect[12][2] ={{34, 290}, {34, 225}, {34, 160}, {34, 95},
                       {99, 290}, {99, 225}, {99, 160}, {99, 95},
                       {164, 290}, {164, 225}, {164, 160}, {164, 95}};

  /* Disable the JoyStick interrupts */
  IntExtOnOffConfig(DISABLE);

  while(ReadKey() != NOKEY)
  {
  }

  /* Initializes the Menu state machine */
  Menu_Init();

  MenuItemIndex = 0;
  
  /* Clear*/
  LCD_Clear(White);

  /* Set the Back Color */
  LCD_SetBackColor(Blue);

  /* Set the Text Color */
  LCD_SetTextColor(White);

  LCD_DisplayStringLine(Line0, psMenuItem->pszTitle);
  
  /* Set the Back Color */
  LCD_SetBackColor(White);

  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  /* Displays Icons */    
  DisplayIcons();

  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  /* Disable LCD Window mode */
  LCD_WindowModeDisable(); 


  LCD_DrawRect(IconRect[0][0], IconRect[0][1], 64, 65);

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_13) == RESET)
  {
  }
  /* Endless loop */
  while(1)
  {
    /* Check which key is pressed */
    MyKey = ReadKey();

    /* If "UP" pushbutton is pressed */
    if(MyKey == UP)
    {
      /* Set the Text Color */
      LCD_SetTextColor(White);

      LCD_DrawRect(IconRect[i][0], IconRect[i][1], 64, 65);

      if(i <= 3)
      {
        i += 8;
        if(i >= NumberOfIcons)
        {
          i = (NumberOfIcons - 1);
        }
      }
      else
      {
        i -= 4;
      }
      /* Set the Text Color */
      LCD_SetTextColor(Blue);
      LCD_DrawRect(IconRect[i][0], IconRect[i][1], 64, 65);

      /* Set the Back Color */
      LCD_SetBackColor(Blue);
      /* Set the Text Color */
      LCD_SetTextColor(White);
      /* Test on the MenuItemIndex value before incrementing it */
      if(MenuItemIndex <= 3)
      {
        MenuItemIndex += 8;
        if(MenuItemIndex >= NumberOfIcons)
        {
          MenuItemIndex = (NumberOfIcons - 1);
        }
      }
      else
      {
        MenuItemIndex -= 4;
      }
      /* Get the current menu */
      psMenuItem = &(psCurrentMenu->psItems[MenuItemIndex]);
      LCD_DisplayStringLine(Line0, psMenuItem->pszTitle);
      ItemNumb[nMenuLevel] = MenuItemIndex;
    }
    /* If "DOWN" pushbutton is pressed */
    if(MyKey == DOWN)
    {
      /* Set the Text Color */
      LCD_SetTextColor(White);
      LCD_DrawRect(IconRect[i][0], IconRect[i][1], 64, 65);
      if(i >= 8)
      {
        i -= 8;
      }
      else
      {
        i += 4;
        if(i >= NumberOfIcons)
        {
          i = (NumberOfIcons - 1);
        }
      }
      /* Set the Text Color */
      LCD_SetTextColor(Blue);
      LCD_DrawRect(IconRect[i][0], IconRect[i][1], 64, 65);

      /* Set the Back Color */
      LCD_SetBackColor(Blue);
      /* Set the Text Color */
      LCD_SetTextColor(White);
      /* Test on the MenuItemIndex value before incrementing it */
      if(MenuItemIndex >= 8)
      {
        MenuItemIndex -= 8;
      }
      else
      {
        MenuItemIndex += 4;
        if(MenuItemIndex >= NumberOfIcons)
        {
          MenuItemIndex = (NumberOfIcons - 1);
        }
      }
      /* Get the current menu */
      psMenuItem = &(psCurrentMenu->psItems[MenuItemIndex]);
      LCD_DisplayStringLine(Line0, psMenuItem->pszTitle);
      ItemNumb[nMenuLevel] = MenuItemIndex;
    }
    /* If "LEFT" pushbutton is pressed */
    if(MyKey == LEFT)
    {
      /* Set the Text Color */
      LCD_SetTextColor(White);
      LCD_DrawRect(IconRect[i][0], IconRect[i][1], 64, 65);
      if(i == 0)
      {
        i = (NumberOfIcons - 1);
      }
      else
      {
        i--;
      }
      /* Set the Text Color */
      LCD_SetTextColor(Blue);
      LCD_DrawRect(IconRect[i][0], IconRect[i][1], 64, 65);

      /* Set the Back Color */
      LCD_SetBackColor(Blue);
      /* Set the Text Color */
      LCD_SetTextColor(White);
      if(MenuItemIndex > 0)
      {
        MenuItemIndex--;
      }
      else
      {
        MenuItemIndex = psCurrentMenu->nItems - 1;
      }

      psMenuItem = &psCurrentMenu->psItems[MenuItemIndex];
      LCD_DisplayStringLine(Line0, psMenuItem->pszTitle);
      ItemNumb[nMenuLevel] = MenuItemIndex;
    }
    /* If "RIGHT" pushbutton is pressed */
    if(MyKey == RIGHT)
    {
      /* Set the Text Color */
      LCD_SetTextColor(White);
      LCD_DrawRect(IconRect[i][0], IconRect[i][1], 64, 65);
      if(i == (NumberOfIcons - 1))
      {
        i = 0x00;
      }
      else
      {
        i++;
      }
      /* Set the Text Color */
      LCD_SetTextColor(Blue);
      LCD_DrawRect(IconRect[i][0], IconRect[i][1], 64, 65);

      /* Set the Back Color */
      LCD_SetBackColor(Blue);
      /* Set the Text Color */
      LCD_SetTextColor(White);
      /* Test on the MenuItemIndex value before incrementing it */
      if(MenuItemIndex >= ((psCurrentMenu->nItems) - 1))
      {
        MenuItemIndex = 0;
      }
      else
      {
        MenuItemIndex++;
      }
      /* Get the current menu */
      psMenuItem = &(psCurrentMenu->psItems[MenuItemIndex]);
      LCD_DisplayStringLine(Line0, psMenuItem->pszTitle);
      ItemNumb[nMenuLevel] = MenuItemIndex;
    }
    /* If "SEL" pushbutton is pressed */
    if(MyKey == SEL)
    {
      SelFunc();
      IntExtOnOffConfig(ENABLE);
      return;
    }
  }    
}

/*******************************************************************************
* Function Name  : LCD_NORDisplay
* Description    : Display a picture from the NOR Flash.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_NORDisplay(u32 address)
{
  /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  
  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
  
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();

  /* Slide n�: index */
  LCD_WriteBMP(address);
}

/*******************************************************************************
* Function Name  : STM32_LCD_DemoIntro
* Description    : Display the STM32 LCD Demo introduction.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void STM32_LCD_DemoIntro(void)
{
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);
  LCD_NORDisplay(SLIDE1);
  Delay(100);
  
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);
  LCD_NORDisplay(SLIDE2);  
  Delay(100);
}

/*******************************************************************************
* Function Name  : InternalFlashToLCD
* Description    : Display images from the internal FLASH on LCD.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void InternalFlashToLCD(void)
{
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);

  /* Checks if the Bitmap files are loaded */
  if(InternalFlashCheckBitmapFiles() != 0)
  {
    /* Set the LCD Back Color */
    LCD_SetBackColor(Blue);

    /* Set the LCD Text Color */
    LCD_SetTextColor(White);    
    LCD_DisplayStringLine(Line0, "      Warning       ");
    LCD_DisplayStringLine(Line1, "You need to copy    ");
    LCD_DisplayStringLine(Line2, "bitmap images in    ");
    LCD_DisplayStringLine(Line3, "internal FLASH.     ");

    LCD_DisplayStringLine(Line7, " Push JoyStick to   ");
    LCD_DisplayStringLine(Line8, "continue.           ");  
    while(ReadKey() == NOKEY)
    {
    }

    while(ReadKey() != NOKEY)
    {
    }

    LCD_Clear(White);
    DisplayMenu();
    IntExtOnOffConfig(ENABLE);
    return;
  }
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Push Key Button to ");
  LCD_DisplayStringLine(Line5, " start or stop.     ");


  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "    Internal FLASH  ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Push key Push Button to exit */
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    /* Timing measurement base time config */
    TimingMeasurement_Config();
    
    /* Start timing counter: measure display time */
    SysTick_CounterCmd(SysTick_Counter_Enable);

    LCD_WriteBMP(START_ADDR);

    /* Stop timing counter */
    SysTick_CounterCmd(SysTick_Counter_Disable);

    DisplayTimingCompute();

    GPIO_SetBits(GPIOF, GPIO_Pin_6);


    LCD_WriteBMP((START_ADDR + 0x25842));
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);

  /* Set the Back Color */
  LCD_SetBackColor(White);

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "    Internal FLASH  ");
  sprintf((char*)TxBuffer,"Time = %d e-6 s  ", fractionaltimeunits);
  
  LCD_DisplayStringLine(Line2, TxBuffer);

  /* Set the Text Color */
  LCD_SetTextColor(Green);
  sprintf((char*)TxBuffer,"  %d fps (Frames per Second)", 1000000/fractionaltimeunits); 
  LCD_DisplayStringLine(Line3, TxBuffer);
  LCD_DisplayStringLine(Line4, "Second)");

  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
}
 
/*******************************************************************************
* Function Name  : InternalFlashToLCD_DMA
* Description    : Display images from the internal FLASH on LCD using DMA.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void InternalFlashToLCD_DMA(void)
{

  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }
  
  /* Clear the LCD */
  LCD_Clear(White);

  /* Checks if the Bitmap files are loaded */
  if(InternalFlashCheckBitmapFiles() != 0)
  {
    /* Set the LCD Back Color */
    LCD_SetBackColor(Blue);

    /* Set the LCD Text Color */
    LCD_SetTextColor(White);    
    LCD_DisplayStringLine(Line0, "      Warning       ");
    LCD_DisplayStringLine(Line1, "You need to copy    ");
    LCD_DisplayStringLine(Line2, "bitmap images in    ");
    LCD_DisplayStringLine(Line3, "internal FLASH.     ");

    LCD_DisplayStringLine(Line7, " Push JoyStick to   ");
    LCD_DisplayStringLine(Line8, "continue.           ");  
    while(ReadKey() == NOKEY)
    {
    }

    while(ReadKey() != NOKEY)
    {
    }

    LCD_Clear(White);
    DisplayMenu();
    IntExtOnOffConfig(ENABLE);
    return;
  }

  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Push Key Button to ");
  LCD_DisplayStringLine(Line5, " start or stop.     "); 

    /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, " Internal FLASH DMA ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Push key Push Button to exit */
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    /* Timing measurement base time config */
    TimingMeasurement_Config();

    /* Start timing counter: measure display time */
    SysTick_CounterCmd(SysTick_Counter_Enable);

    LCD_WriteDMA(START_ADDR);

    /* Stop timing counter */
    SysTick_CounterCmd(SysTick_Counter_Disable);

    DisplayTimingCompute();

    GPIO_SetBits(GPIOF, GPIO_Pin_6);

    LCD_WriteDMA((START_ADDR + 0x25842));
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  LCD_DisplayStringLine(Line1, " Internal FLASH DMA ");
  sprintf((char*)TxBuffer,"Time = %d e-6 s  ", fractionaltimeunits);
  
  LCD_DisplayStringLine(Line2, TxBuffer);

  /* Set the Text Color */
  LCD_SetTextColor(Green);
  sprintf((char*)TxBuffer,"  %d fps (Frames per Second)", 1000000/fractionaltimeunits);  
  LCD_DisplayStringLine(Line3, TxBuffer);
  LCD_DisplayStringLine(Line4, "Second)");  

  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);  
} 

/*******************************************************************************
* Function Name  : InternalFlashToLCD_Speed
* Description    : Display images from the internal FLASH on LCD using CPU
*                  with a controlled speed through the RV1 trimmer.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void InternalFlashToLCD_Speed(void)
{
  u32 keystate = 0;
  
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);

  /* Checks if the Bitmap files are loaded */
  if(InternalFlashCheckBitmapFiles() != 0)
  {
    /* Set the LCD Back Color */
    LCD_SetBackColor(Blue);

    /* Set the LCD Text Color */
    LCD_SetTextColor(White);    
    LCD_DisplayStringLine(Line0, "      Warning       ");
    LCD_DisplayStringLine(Line1, "You need to copy    ");
    LCD_DisplayStringLine(Line2, "bitmap images in    ");
    LCD_DisplayStringLine(Line3, "internal FLASH.     ");

    LCD_DisplayStringLine(Line7, " Push JoyStick to   ");
    LCD_DisplayStringLine(Line8, "continue.           ");  
    while(ReadKey() == NOKEY)
    {
    }

    while(ReadKey() != NOKEY)
    {
    }

    LCD_Clear(White);
    DisplayMenu();
    IntExtOnOffConfig(ENABLE);
    return;
  }
  
  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Turn the Trimmer to");
  LCD_DisplayStringLine(Line5, " control the delay  ");
  LCD_DisplayStringLine(Line6, " between two frames.");
  LCD_DisplayStringLine(Line7, " Push Key Button to ");
  LCD_DisplayStringLine(Line8, " start or stop.     ");


  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "    Internal FLASH  ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);
  
  /* Configure the systick */    
  SysTick_Config();

  /* Push key Push Button to exit */
  while(keystate != KEY)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    LCD_WriteBMP(START_ADDR);

    GPIO_SetBits(GPIOF, GPIO_Pin_6);
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);
    
    if(keystate == KEY)
    {
      break;
    }
    
    LCD_WriteBMP((START_ADDR + 0x25842));
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);

  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
} 

/*******************************************************************************
* Function Name  : NORFlashToLCD
* Description    : Display images from the NOR FLASH on LCD .
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NORFlashToLCD(void)
{
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Push Key Button to ");
  LCD_DisplayStringLine(Line5, " start or stop.     ");
  
  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "      NOR FLASH     ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }
  
   /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
 
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Push key Push Button to exit */
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    /* Timing measurement base time config */
    TimingMeasurement_Config();

    /* Start timing counter: measure display time */
    SysTick_CounterCmd(SysTick_Counter_Enable);

    LCD_WriteBMP(SLIDE1);

    /* Stop timing counter */
    SysTick_CounterCmd(SysTick_Counter_Disable);

    DisplayTimingCompute();

    GPIO_SetBits(GPIOF, GPIO_Pin_6);

    LCD_WriteBMP(SLIDE2);
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  LCD_DisplayStringLine(Line1, "      NOR FLASH     ");
  sprintf((char*)TxBuffer,"Time = %d e-6 s  ", fractionaltimeunits);
  
  LCD_DisplayStringLine(Line2, TxBuffer);

  /* Set the Text Color */
  LCD_SetTextColor(Green);
  sprintf((char*)TxBuffer,"  %d fps (Frames per Second)", 1000000/fractionaltimeunits);  
  LCD_DisplayStringLine(Line3, TxBuffer);
  LCD_DisplayStringLine(Line4, "Second)");
   
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);  
}

/*******************************************************************************
* Function Name  : NORFlashToLCD_DMA
* Description    : Display images from the NOR FLASH on LCD using DMA.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NORFlashToLCD_DMA(void)
{

  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Push Key Button to ");
  LCD_DisplayStringLine(Line5, " start or stop.     ");  
  
  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "    NOR FLASH DMA   ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

   /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
 
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Push key Push Button to exit */
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    /* Timing measurement base time config */
    TimingMeasurement_Config();

    /* Start timing counter: measure display time */
    SysTick_CounterCmd(SysTick_Counter_Enable);

    LCD_WriteDMA(SLIDE1);

    /* Stop timing counter */
    SysTick_CounterCmd(SysTick_Counter_Disable);

    DisplayTimingCompute();

    GPIO_SetBits(GPIOF, GPIO_Pin_6);

    LCD_WriteDMA(SLIDE2);
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  LCD_DisplayStringLine(Line1, "    NOR FLASH DMA   ");
  sprintf((char*)TxBuffer,"Time = %d e-6 s  ", fractionaltimeunits);
  
  LCD_DisplayStringLine(Line2, TxBuffer);

  /* Set the Text Color */
  LCD_SetTextColor(Green);
  sprintf((char*)TxBuffer,"  %d fps (Frames per Second)", 1000000/fractionaltimeunits);  
  LCD_DisplayStringLine(Line3, TxBuffer);
  LCD_DisplayStringLine(Line4, "Second)");
 
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);  
}

/*******************************************************************************
* Function Name  : NORFlashToLCD_Speed
* Description    : Display images from the NOR FLASH on LCD using CPU
*                  with a controlled speed through the RV1 trimmer.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NORFlashToLCD_Speed(void)
{
  u32 keystate = 0;
  
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Turn the Trimmer to");
  LCD_DisplayStringLine(Line5, " control the delay  ");
  LCD_DisplayStringLine(Line6, " between two frames.");
  LCD_DisplayStringLine(Line7, " Push Key Button to ");
  LCD_DisplayStringLine(Line8, " start or stop.     ");


  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "      NOR FLASH     ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }


  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }
  
   /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
 
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Configure the systick */    
  SysTick_Config();

  /* Push key Push Button to exit */
  while(keystate != KEY)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);
    LCD_WriteBMP(SLIDE1);
    GPIO_SetBits(GPIOF, GPIO_Pin_6);
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);

    if(keystate == KEY)
    {
      break;
    }
    
    LCD_WriteBMP(SLIDE2);
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
} 

/*******************************************************************************
* Function Name  : NANDFlashToLCD
* Description    : Display images from the NAND FLASH on LCD.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NANDFlashToLCD(void)
{
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Push Key Button to ");
  LCD_DisplayStringLine(Line5, " start or stop.     ");  

    /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "      NAND FLASH    ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  FSMC_NAND_Init();   

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Push key Push Button to exit */
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    /* Timing measurement base time config */
    TimingMeasurement_Config();

    /* Start timing counter: measure display time */
    SysTick_CounterCmd(SysTick_Counter_Enable);

    LCD_NANDDisplay(0);

    /* Stop timing counter */
    SysTick_CounterCmd(SysTick_Counter_Disable);

    DisplayTimingCompute();

    GPIO_SetBits(GPIOF, GPIO_Pin_6);

    LCD_NANDDisplay(320);
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  LCD_DisplayStringLine(Line1, "      NAND FLASH    ");
  sprintf((char*)TxBuffer,"Time = %d e-6 s  ", fractionaltimeunits);
  
  LCD_DisplayStringLine(Line2, TxBuffer);

  /* Set the Text Color */
  LCD_SetTextColor(Green);
  sprintf((char*)TxBuffer,"  %d fps (Frames per Second)", 1000000/fractionaltimeunits);  
  LCD_DisplayStringLine(Line3, TxBuffer);
  LCD_DisplayStringLine(Line4, "Second)");
    
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);  
}

/*******************************************************************************
* Function Name  : NANDFlashToLCD_Speed
* Description    : Display images from the NAND FLASH on LCD using CPU
*                  with a controlled speed through the RV1 trimmer.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NANDFlashToLCD_Speed(void)
{
  u32 keystate = 0;
  
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Turn the Trimmer to");
  LCD_DisplayStringLine(Line5, " control the delay  ");
  LCD_DisplayStringLine(Line6, " between two frames.");
  LCD_DisplayStringLine(Line7, " Push Key Button to ");
  LCD_DisplayStringLine(Line8, " start or stop.     ");


  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "      NAND FLASH    ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  FSMC_NAND_Init();   

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Configure the systick */    
  SysTick_Config();

  /* Push key Push Button to exit */
  while(keystate != KEY)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);
    LCD_NANDDisplay(0);
    GPIO_SetBits(GPIOF, GPIO_Pin_6);
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);

    if(keystate == KEY)
    {
      break;
    }
    
    LCD_NANDDisplay(320);
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);

  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
} 

/*******************************************************************************
* Function Name  : ExternalSRAMToLCD
* Description    : Display images from the External SRAM on LCD.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ExternalSRAMToLCD(void)
{
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }
  
  /* Clear the LCD */
  LCD_Clear(White);

  /* Checks if the Bitmap files are loaded */
  if(ExternalSRAMCheckBitmapFiles() != 0)
  {
    /* Set the LCD Back Color */
    LCD_SetBackColor(Blue);

    /* Set the LCD Text Color */
    LCD_SetTextColor(White);    
    LCD_DisplayStringLine(Line0, "      Warning       ");
    LCD_DisplayStringLine(Line1, "You need to copy    ");
    LCD_DisplayStringLine(Line2, "bitmap images in    ");
    LCD_DisplayStringLine(Line3, "external SRAM.      ");

    LCD_DisplayStringLine(Line7, " Push JoyStick to   ");
    LCD_DisplayStringLine(Line8, "continue.           ");  
    while(ReadKey() == NOKEY)
    {
    }

    while(ReadKey() != NOKEY)
    {
    }

    LCD_Clear(White);
    DisplayMenu();
    IntExtOnOffConfig(ENABLE);
    return;
  }
 
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Push Key Button to ");
  LCD_DisplayStringLine(Line5, " start or stop.     ");  

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "    External SRAM   ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  /* Configure FSMC Bank1 NOR/SRAM3 */
  FSMC_SRAM_Init();

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Push key Push Button to exit */
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    /* Timing measurement base time config */
    TimingMeasurement_Config();

    /* Start timing counter: measure display time */
    SysTick_CounterCmd(SysTick_Counter_Enable);

    LCD_WriteBMP((u32)Bank1_SRAM_ADDR);

    /* Stop timing counter */
    SysTick_CounterCmd(SysTick_Counter_Disable);

    DisplayTimingCompute();

    GPIO_SetBits(GPIOF, GPIO_Pin_6);

    LCD_WriteBMP((u32) (Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)));    
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  LCD_DisplayStringLine(Line1, "    External SRAM   ");
  sprintf((char*)TxBuffer,"Time = %d e-6 s  ", fractionaltimeunits);
  
  LCD_DisplayStringLine(Line2, TxBuffer);

  /* Set the Text Color */
  LCD_SetTextColor(Green);
  sprintf((char*)TxBuffer,"  %d fps (Frames per Second)", 1000000/fractionaltimeunits); 
  LCD_DisplayStringLine(Line3, TxBuffer);
  LCD_DisplayStringLine(Line4, "Second)");
    
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);  
}

/*******************************************************************************
* Function Name  : ExternalSRAMToLCD_DMA
* Description    : Display images from the External SRAM on LCD using DMA.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ExternalSRAMToLCD_DMA(void)
{
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }
  
  /* Clear the LCD */
  LCD_Clear(White);

  /* Checks if the Bitmap files are loaded */
  if(ExternalSRAMCheckBitmapFiles() != 0)
  {
    /* Set the LCD Back Color */
    LCD_SetBackColor(Blue);

    /* Set the LCD Text Color */
    LCD_SetTextColor(White);    
    LCD_DisplayStringLine(Line0, "      Warning       ");
    LCD_DisplayStringLine(Line1, "You need to copy    ");
    LCD_DisplayStringLine(Line2, "bitmap images in    ");
    LCD_DisplayStringLine(Line3, "external SRAM.      ");

    LCD_DisplayStringLine(Line7, " Push JoyStick to   ");
    LCD_DisplayStringLine(Line8, "continue.           ");  
    while(ReadKey() == NOKEY)
    {
    }

    while(ReadKey() != NOKEY)
    {
    }

    LCD_Clear(White);
    DisplayMenu();
    IntExtOnOffConfig(ENABLE);
    return;
  }

  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Push Key Button to ");
  LCD_DisplayStringLine(Line5, " start or stop.     ");  
  
  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "  External SRAM DMA ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  /* Configure FSMC Bank1 NOR/SRAM3 */
  FSMC_SRAM_Init();

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Push key Push Button to exit */
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    /* Timing measurement base time config */
    TimingMeasurement_Config();

    /* Start timing counter: measure display time */
    SysTick_CounterCmd(SysTick_Counter_Enable);

    LCD_WriteDMA(Bank1_SRAM_ADDR);

    /* Stop timing counter */
    SysTick_CounterCmd(SysTick_Counter_Disable);

    DisplayTimingCompute();

    GPIO_SetBits(GPIOF, GPIO_Pin_6);

    LCD_WriteDMA((Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)));	
  }

  while(ReadKey() != NOKEY)
  {
  }  

  /* Clear the LCD */
  LCD_Clear(White);
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  LCD_DisplayStringLine(Line1, "  External SRAM DMA ");
  sprintf((char*)TxBuffer,"Time = %d e-6 s  ", fractionaltimeunits);
  
  LCD_DisplayStringLine(Line2, TxBuffer);

  /* Set the Text Color */
  LCD_SetTextColor(Green);
  sprintf((char*)TxBuffer,"  %d fps (Frames per Second)", 1000000/fractionaltimeunits);  
  LCD_DisplayStringLine(Line3, TxBuffer);
  LCD_DisplayStringLine(Line4, "Second)");

  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);  
}

/*******************************************************************************
* Function Name  : ExternalSRAMToLCD_Speed
* Description    : Display images from the External SRAM on LCD using CPU
*                  with a controlled speed through the RV1 trimmer.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ExternalSRAMToLCD_Speed(void)
{
  u32 keystate = 0;
  
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }
  
  /* Clear the LCD */
  LCD_Clear(White);

  /* Checks if the Bitmap files are loaded */
  if(ExternalSRAMCheckBitmapFiles() != 0)
  {
    /* Set the LCD Back Color */
    LCD_SetBackColor(Blue);

    /* Set the LCD Text Color */
    LCD_SetTextColor(White);    
    LCD_DisplayStringLine(Line0, "      Warning       ");
    LCD_DisplayStringLine(Line1, "You need to copy    ");
    LCD_DisplayStringLine(Line2, "bitmap images in    ");
    LCD_DisplayStringLine(Line3, "external SRAM.      ");

    LCD_DisplayStringLine(Line7, " Push JoyStick to   ");
    LCD_DisplayStringLine(Line8, "continue.           ");  
    while(ReadKey() == NOKEY)
    {
    }

    while(ReadKey() != NOKEY)
    {
    }

    LCD_Clear(White);
    DisplayMenu();
    IntExtOnOffConfig(ENABLE);
    return;
  }
 
  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Turn the Trimmer to");
  LCD_DisplayStringLine(Line5, " control the delay  ");
  LCD_DisplayStringLine(Line6, " between two frames.");
  LCD_DisplayStringLine(Line7, " Push Key Button to ");
  LCD_DisplayStringLine(Line8, " start or stop.     ");


  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "    External SRAM   ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  /* Configure FSMC Bank1 NOR/SRAM3 */
  FSMC_SRAM_Init();

  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);
  
  /* Configure the systick */    
  SysTick_Config();

  /* Push key Push Button to exit */
  while(keystate != KEY)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);
    LCD_WriteBMP((u32)Bank1_SRAM_ADDR);
    GPIO_SetBits(GPIOF, GPIO_Pin_6);
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);

    if(keystate == KEY)
    {
      break;
    }
    
    LCD_WriteBMP((u32) (Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)));
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);    
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);

  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
}

/*******************************************************************************
* Function Name  : SDCardToLCD
* Description    : Display images from the SD Card on LCD.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SDCardToLCD(void)
{
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Push Key Button to ");
  LCD_DisplayStringLine(Line5, " start or stop.     ");  
  
  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "       SD Card      ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  /*-------------------------- SD Init ----------------------------- */
  Status = SD_Init();

  if (Status == SD_OK)
  {
    /*----------------- Read CSD/CID MSD registers ------------------*/
    Status = SD_GetCardInfo(&SDCardInfo);
  }

  if (Status == SD_OK)
  {
    /*----------------- Select Card --------------------------------*/
    Status = SD_SelectDeselect((u32) (SDCardInfo.RCA << 16));
  }

  if (Status == SD_OK)
  {
    Status = SD_EnableWideBusOperation(SDIO_BusWide_4b);
  }

  /* Set Device Transfer Mode */
  if (Status == SD_OK)
  {
    Status = SD_SetDeviceMode(SD_DMA_MODE);
  }
  
  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Push key Push Button to exit */
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);

    /* Timing measurement base time config */
    TimingMeasurement_Config();

    /* Start timing counter: measure display time */
    SysTick_CounterCmd(SysTick_Counter_Enable);

    LCD_SDDisplay(0x00);

    /* Stop timing counter */
    SysTick_CounterCmd(SysTick_Counter_Disable);

    DisplayTimingCompute();

    GPIO_SetBits(GPIOF, GPIO_Pin_6);

    LCD_SDDisplay(0x25800);
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  /* Set the Back Color */
  LCD_SetBackColor(White);
  /* Set the Text Color */
  LCD_SetTextColor(Blue);

  LCD_DisplayStringLine(Line1, "       SD Card      ");
  sprintf((char*)TxBuffer,"Time = %d e-6 s  ", fractionaltimeunits);
  
  LCD_DisplayStringLine(Line2, TxBuffer);

  /* Set the Text Color */
  LCD_SetTextColor(Green);
  sprintf((char*)TxBuffer,"  %d fps (Frames per Second)", 1000000/fractionaltimeunits);  
  LCD_DisplayStringLine(Line3, TxBuffer);
  LCD_DisplayStringLine(Line4, "Second)");
  
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }


  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);  
}

/*******************************************************************************
* Function Name  : SDCardToLCD_Speed
* Description    : Display images from the SD Card on LCD using CPU
*                  with a controlled speed through the RV1 trimmer.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SDCardToLCD_Speed(void)
{
  u32 keystate = 0;
  
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, " Turn the Trimmer to");
  LCD_DisplayStringLine(Line5, " control the delay  ");
  LCD_DisplayStringLine(Line6, " between two frames.");
  LCD_DisplayStringLine(Line7, " Push Key Button to ");
  LCD_DisplayStringLine(Line8, " start or stop.     ");


  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line1, "   LCD on FSMC Demo ");  
  LCD_DisplayStringLine(Line2, "       SD Card      ");

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }

  /*-------------------------- SD Init ----------------------------- */
  Status = SD_Init();

  if (Status == SD_OK)
  {
    /*----------------- Read CSD/CID MSD registers ------------------*/
    Status = SD_GetCardInfo(&SDCardInfo);
  }

  if (Status == SD_OK)
  {
    /*----------------- Select Card --------------------------------*/
    Status = SD_SelectDeselect((u32) (SDCardInfo.RCA << 16));
  }

  if (Status == SD_OK)
  {
    Status = SD_EnableWideBusOperation(SDIO_BusWide_4b);
  }

  /* Set Device Transfer Mode */
  if (Status == SD_OK)
  {
    Status = SD_SetDeviceMode(SD_DMA_MODE);
  }
  
  /* Set the LCD display window */
  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Configure the systick */    
  SysTick_Config();

  /* Push key Push Button to exit */
  while(keystate != KEY)
  {
    GPIO_ResetBits(GPIOF, GPIO_Pin_6);
    LCD_SDDisplay(0x00);
    GPIO_SetBits(GPIOF, GPIO_Pin_6);
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);

    if(keystate == KEY)
    {
      break;
    }
    
    LCD_SDDisplay(0x25800);
    keystate = DelayJoyStick(ADC_GetConversionValue(ADC1)/40);
  }

  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }


  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
}

/*******************************************************************************
* Function Name  : LCD_WriteSD
* Description    : Write a full size picture on the SD Card.
* Input          : - address: SD card memory address to write to.
*                  - pointer: pointer to the buffer to be written in SD card.
* Output         : None
* Return         : None
*******************************************************************************/
SD_Error LCD_WriteSD(u32 address, u32 *pointer)
{
  u32 i = 0, sdaddress = address;

  for(i = 0; i < 15; i++)
  {
    if (Status == SD_OK)
    {
      /* Write block of 512 bytes on awddress 0 */
      Status = SD_WriteMultiBlocks(sdaddress, pointer, BlockSize, 20);
      sdaddress += BlockSize * 20;
      pointer += BlockSize * 5;
    }
    else
    {
      break;
    }
  }
  return Status;
}

/*******************************************************************************
* Function Name  : LCD_SDDisplay
* Description    : Write a full size picture on the LCD from SD Card.
* Input          : address: SD card memory address to read from.
* Output         : None
* Return         : None
*******************************************************************************/
SD_Error LCD_SDDisplay(u32 address)
{
  u32 i = 0, j = 0, sdaddress = address;
  u16* ptr = (u16 *)(Buffer_Block_Rx);
  

  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  /* Set GRAM write direction and BGR = 1 */
  /* I/D=00 (Horizontal : decrement, Vertical : decrement) */
  /* AM=1 (address is updated in vertical writing direction) */
  LCD_WriteReg(R3, 0x1008);
 
  LCD_WriteRAM_Prepare();
 
  for(i = 0; i < 15; i++)
  {
    if (Status == SD_OK)
    {
      /* Read block of 512*20 bytes from address 0 */
      Status = SD_ReadMultiBlocks(sdaddress, Buffer_Block_Rx, BlockSize, 20);
      for(j = 0; j < 5120; j++)
      {
        LCD_WriteRAM(ptr[j]);
      }
    }
    else
    {
      break;
    }
    ptr = (u16 *)Buffer_Block_Rx;
    sdaddress += BlockSize*20;
  }  
  LCD_WriteReg(R3, 0x1018);
  return Status;
}

/*******************************************************************************
* Function Name  : LCD_NANDDisplay
* Description    : Write a full size picture on the LCD from NAND Flash.
* Input          : address: address to the picture on the NAND Flash.
* Output         : None
* Return         : None
*******************************************************************************/
u32 LCD_NANDDisplay(u32 address)
{
  u32 i = 0, j = 0;
  u16 *ptr;

  ptr = (u16 *) pBuffer;

  LCD_SetDisplayWindow(239, 0x13F, 240, 320);

  LCD_WriteReg(R3, 0x1008);
 
  LCD_WriteRAM_Prepare();
 
  for(i = 0; i < 9; i++)
  {	
    NAND_Read((address * 512), pBuffer, 32);
    address += 32;
    
    for(j = 0; j < 8192; j++)
    {
      LCD_WriteRAM(ptr[j]);
    }
  }

  NAND_Read((address * 512), pBuffer, 32);

  for(j = 0; j < 3072; j++)
  {
    LCD_WriteRAM(ptr[j]);
  }


  LCD_WriteReg(R3, 0x1018);
  return Status;
}

/*******************************************************************************
* Function Name  : LCD_WriteDMA
* Description    : Write a full size picture on the LCD from the selected memory 
*                  using DMA.
* Input          : address: address to the picture on the selected memory.
* Output         : None
* Return         : None
*******************************************************************************/
void LCD_WriteDMA(u32 address)
{
  DMA_InitTypeDef  DMA_InitStructure;

  /* Set GRAM write direction and BGR = 1 */
  /* I/D=00 (Horizontal : decrement, Vertical : decrement) */
  /* AM=1 (address is updated in vertical writing direction) */
  LCD_WriteReg(R3, 0x1008);
 
  LCD_WriteRAM_Prepare();
  
  DMAComplete = 0;
  FirstEntry = 1;
  DMASourceAddress = address + 0x42;
  
  /* DMA1 channel3 configuration */
  DMA_DeInit(DMA1_Channel3);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)DMASourceAddress;  
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)LCD_RAM_ADDR;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 0xFFFF;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;
  
  DMA_Init(DMA1_Channel3, &DMA_InitStructure);

  /* Enable DMA Channel3 Transfer Complete interrupt */
  DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);

  /* Enable DMA1 channel3 */
  DMA_Cmd(DMA1_Channel3, ENABLE);

  /* Wait for DMA transfer Complete */
  while(DMAComplete == 0)
  {
  }
}

/*******************************************************************************
* Function Name  : CheckBitmapFiles
* Description    : Checks if the bitmap files (slides + icons) are already loaded
*                  in the NOR FLASH.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u32 CheckBitmapFiles(void)
{
  u32 index = 0;
  u8 Tab[6];

 
  /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  
  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
  
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();


  Tab[0] = 0x00;
  Tab[1] = 0x00;
  Tab[2] = 0x00;
  Tab[3] = 0x00;
  Tab[4] = 0x00;
  Tab[5] = 0x00;


  /* Read bitmap size */
  Media_BufferRead(Tab, SLIDE1, 6);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  if(Buffercmp(Tab, SlidesCheck, 5)!= 0) return 1;
  Tab[0] = 0x00;
  Tab[1] = 0x00;
  Tab[2] = 0x00;
  Tab[3] = 0x00;
  Tab[4] = 0x00;
  Tab[5] = 0x00;

  /* Read bitmap size */
  Media_BufferRead(Tab, SLIDE2, 6);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  if(Buffercmp(Tab, SlidesCheck, 5)!= 0) return 1;
  Tab[0] = 0x00;
  Tab[1] = 0x00;
  Tab[2] = 0x00;
  Tab[3] = 0x00;
  Tab[4] = 0x00;
  Tab[5] = 0x00;

  for(index = 0; index < 5; index++)
  {
    /* Read bitmap size */
    Media_BufferRead(Tab, IconsAddr[index], 6);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
    if(Buffercmp(Tab, Icons64Check, 5)!= 0) return 1;
    Tab[0] = 0x00;
    Tab[1] = 0x00;
    Tab[2] = 0x00;
    Tab[3] = 0x00;
    Tab[4] = 0x00;
    Tab[5] = 0x00;
  }

  return 0;
}


/*******************************************************************************
* Function Name  : Buffercmp
* Description    : Compares two buffers.
* Input          : - pBuffer1, pBuffer2: buffers to be compared.
*                : - BufferLength: buffer's length
* Output         : None
* Return         : 0: pBuffer1 identical to pBuffer2
*                  1: pBuffer1 differs from pBuffer2
*******************************************************************************/
static u8 Buffercmp(u8* pBuffer1, u8* pBuffer2, u16 BufferLength)
{
  while(BufferLength--)
  {
    if(*pBuffer1 != *pBuffer2)
    {
      return 1;
    }

    pBuffer1++;
    pBuffer2++;
  }

  return 0;
}

/*******************************************************************************
* Function Name  : TimingMeasurement_Config
* Description    : Configure the SysTick Base time.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TimingMeasurement_Config(void)
{
  RCC_ClocksTypeDef RCC_ClockFreq;

  /* Get HCLK frequency */
  RCC_GetClocksFreq(&RCC_ClockFreq);

  SysTick_CounterCmd(SysTick_Counter_Disable);

  /* Configure HCLK clock as SysTick clock source */
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

  /*  */
  SysTick_SetReload(0xFFFFFF);

  /* Disable the SysTick Interrupt */
  SysTick_ITConfig(DISABLE);

  SysTick_CounterCmd(SysTick_Counter_Clear);
}

/*******************************************************************************
* Function Name  : DisplayTimingCompute
* Description    : Calculate the number of �s in the systick counter.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DisplayTimingCompute(void)
{
  u32 counter = 0;

  counter = SysTick_GetCounter();

  if (counter != 0)
  {
    /* Convert timer ticks to ns */
    counter =  (0xFFFFFF - counter) * 111;
  }
  
  /* Compute timing in microsecond (�s) */
  fractionaltimeunits = counter / 1000;

}

/*******************************************************************************
* Function Name  : CopyToInternalFlash
* Description    : Copy images to internal FLASH.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CopyToInternalFlash(void)
{
  u32 EraseCounter = 0x00, NORSourceAddress = 0x00, Address = 0x00;
  FLASH_Status FLASHStatus;

  FLASHStatus = FLASH_COMPLETE;

  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, "Push Key Button to  ");
  LCD_DisplayStringLine(Line5, "start the copy      ");  
  LCD_DisplayStringLine(Line6, "operation from NOR  ");
  LCD_DisplayStringLine(Line7, "to internal FLASH.  ");
  

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }
  
  /* Clear the LCD */  
  LCD_Clear(White);

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line4, "Copy in Progress... "); 
  LCD_DisplayStringLine(Line5, "Please wait...      ");
   
 /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  
  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
  
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();


  /* Unlock the Flash Program Erase controller */
  FLASH_Unlock();

  /* Clear All pending flags */
  FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	

  /* Erase the FLASH pages */
  for(EraseCounter = 0; (EraseCounter < 151) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
  {
    FLASHStatus = FLASH_ErasePage(START_ADDR + (FLASH_PAGE_SIZE * EraseCounter));
  }
  
  Address = START_ADDR;
  NORSourceAddress = SLIDE1;

  while((Address < (START_ADDR + 0x25842)) && (FLASHStatus == FLASH_COMPLETE))
  {
    FLASHStatus =  FLASH_ProgramHalfWord(Address, *(vu16 *)NORSourceAddress);
    NORSourceAddress += 2;
    Address += 2;
  }

  Address = (START_ADDR + 0x25842);
  NORSourceAddress = SLIDE2;

  while((Address < (START_ADDR + 0x4B084)) && (FLASHStatus == FLASH_COMPLETE))
  {
    FLASHStatus =  FLASH_ProgramHalfWord(Address, *(vu16 *)NORSourceAddress);
    NORSourceAddress += 2;
    Address += 2;
  }

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line4, "    Copy finished   ");
  
  LCD_ClearLine(Line5);
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }


  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
}

/*******************************************************************************
* Function Name  : CopyToExternalSRAM
* Description    : Copy images to the external SRAM.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CopyToExternalSRAM(void)
{
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, "Push Key Button to  ");
  LCD_DisplayStringLine(Line5, "start the copy      ");  
  LCD_DisplayStringLine(Line6, "operation from NOR  ");
  LCD_DisplayStringLine(Line7, "to External SRAM.   ");
  

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }
  
  /* Clear the LCD */  
  LCD_Clear(White);

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line4, "Copy in Progress... "); 
  LCD_DisplayStringLine(Line5, "Please wait...      ");
   
 /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  
  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
  
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();

  /* Configure FSMC Bank1 NOR/SRAM3 */
  FSMC_SRAM_Init();

  FSMC_SRAM_WriteBuffer((u16*)SLIDE1, 0x0000, SLIDE_SIZE);
  FSMC_SRAM_WriteBuffer((u16*)SLIDE2, 2 * SLIDE_SIZE, SLIDE_SIZE);

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line4, "    Copy finished   ");
  
  LCD_ClearLine(Line5);
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }


  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
}

/*******************************************************************************
* Function Name  : CopyToSDCard
* Description    : Copy images to the SD Card.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CopyToSDCard(void)
{
  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, "Push Key Button to  ");
  LCD_DisplayStringLine(Line5, "start the copy      ");  
  LCD_DisplayStringLine(Line6, "operation from NOR  ");
  LCD_DisplayStringLine(Line7, "to SD Card.         ");
  

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }
  
  /* Clear the LCD */  
  LCD_Clear(White);

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line4, "Copy in Progress... ");  
  LCD_DisplayStringLine(Line5, "Please wait...      ");

  /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  
  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
  
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();

  /*-------------------------- SD Init ----------------------------- */
  Status = SD_Init();

  if (Status == SD_OK)
  {
    /*----------------- Read CSD/CID MSD registers ------------------*/
    Status = SD_GetCardInfo(&SDCardInfo);
  }

  if (Status == SD_OK)
  {
    /*----------------- Select Card --------------------------------*/
    Status = SD_SelectDeselect((u32) (SDCardInfo.RCA << 16));
  }

  if (Status == SD_OK)
  {
    Status = SD_EnableWideBusOperation(SDIO_BusWide_4b);
  }

  /* Set Device Transfer Mode */
  if (Status == SD_OK)
  {
    Status = SD_SetDeviceMode(SD_DMA_MODE);
  }
  
  SDIO->CLKCR &= 0xFFFFFF00;
  SDIO->CLKCR |= 0x5;
  
  LCD_WriteSD(0x0, (u32 *)(SLIDE1 + 0x42));
  LCD_WriteSD(0x25800, (u32 *)(SLIDE2 + 0x42));

  SDIO->CLKCR &= 0xFFFFFF00;
  SDIO->CLKCR |= 0x1;
  
  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line4, "    Copy finished   ");

  LCD_ClearLine(Line5);
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }


  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);   
}

/*******************************************************************************
* Function Name  : CopyToNANDFlash
* Description    : Copy images to the NAND Flash.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CopyToNANDFlash(void)
{
  u32 block = 0, i = 0;

  IntExtOnOffConfig(DISABLE);
 
  while(ReadKey() != NOKEY)
  {
  }

  /* Clear the LCD */
  LCD_Clear(White);
  
  LCD_SetTextColor(Red);
  LCD_DisplayStringLine(Line4, "Push Key Button to  ");
  LCD_DisplayStringLine(Line5, "start the copy      ");  
  LCD_DisplayStringLine(Line6, "operation from NOR  ");
  LCD_DisplayStringLine(Line7, "to NAND FLASH.      ");
  

  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) != RESET)
  {
  }
  while(GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_8) == RESET)
  {
  }
  
  /* Clear the LCD */  
  LCD_Clear(White);

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line4, "Copy in Progress... ");  
  LCD_DisplayStringLine(Line5, "Please wait...      ");
  
  /* Write/read to/from FSMC SRAM memory  *************************************/
  /* Enable the FSMC Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  
  /* Configure FSMC Bank1 NOR/SRAM2 */
  FSMC_NOR_Init();
  
  /* Read NOR memory ID */
  FSMC_NOR_ReadID(&NOR_ID);

  FSMC_NOR_ReturnToReadMode();

  FSMC_NAND_Init();
				  
  for(i = 0; i < 10; i++)
  {
    NAND_Write((block * 512), (u8 *)(SLIDE1 + 0x42 + (block * 512)), 32);
    block += 32;    
  }

  for(i = 0, block = 320; i < 10; i++)
  {
    NAND_Write((block * 512), (u8 *)(SLIDE2 + 0x42 + ((block - 320) * 512)), 32);
    block += 32;    
  }

  /* Set the Text Color */
  LCD_SetTextColor(Blue);
  LCD_DisplayStringLine(Line4, "    Copy finished   ");

  LCD_ClearLine(Line5);
  LCD_DisplayStringLine(Line6, "Press any Key to    ");
  LCD_DisplayStringLine(Line7, "continue.           ");

  while(ReadKey() == NOKEY)
  {
  }

  while(ReadKey() != NOKEY)
  {
  }


  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);

}

/*******************************************************************************
* Function Name  : NAND_PhysicalErase
* Description    : Erases the NAND Flash Content.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NAND_PhysicalErase(void)
{
  NAND_ADDRESS phAddress;
  SPARE_AREA  SpareArea;
  u32 BlockIndex = 0;

  /* Disble the JoyStick interrupts */
  IntExtOnOffConfig(DISABLE);


  while(ReadKey() != NOKEY)
  {
  }

  LCD_Clear(White);

  /* Set the Back Color */
  LCD_SetBackColor(Blue);
  /* Set the Text Color */
  LCD_SetTextColor(White); 

  LCD_DisplayStringLine(Line4, " Erase NAND Content ");
  LCD_DisplayStringLine(Line5, "Please wait...      ");
 
  /* FSMC Initialization */
  FSMC_NAND_Init();
 
  for (BlockIndex = 0 ; BlockIndex < NAND_ZONE_SIZE * NAND_MAX_ZONE; BlockIndex++)
  {
    phAddress = NAND_ConvertPhyAddress(BlockIndex * NAND_BLOCK_SIZE );
    SpareArea = ReadSpareArea(BlockIndex * NAND_BLOCK_SIZE);
   
    if((SpareArea.DataStatus != 0)||(SpareArea.BlockStatus != 0)){
        FSMC_NAND_EraseBlock (phAddress);
    }  
  }
  
  
  /* Display the "To stop Press SEL" message */
  LCD_DisplayStringLine(Line4, "     NAND Erased    ");
  LCD_DisplayStringLine(Line5, "  To exit Press SEL ");

  /* Loop until SEL key pressed */
  while(ReadKey() != SEL)
  {
  }
  
  LCD_Clear(White);
  DisplayMenu();
  IntExtOnOffConfig(ENABLE);
}

/*******************************************************************************
* Function Name  : NAND_Write
* Description    : write one sector by once
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static u16 NAND_Write(u32 Memory_Offset, u8 *Writebuff, u16 Transfer_Length)
{
  SPARE_AREA sp;

  do 
  {
    sp = ReadSpareArea(Memory_Offset / 512);

    if (sp.BlockStatus != 0)
    {
      /* Check block status and calculate start and end addreses */
      wAddress = NAND_GetAddress(Memory_Offset / 512);
      FSMC_NAND_EraseBlock (wAddress);
      FSMC_NAND_WriteSmallPage(Writebuff, wAddress, Transfer_Length);
	}
    Memory_Offset += 32*512;
  }while(sp.BlockStatus ==0);

  return NAND_OK;
}

/*******************************************************************************
* Function Name  : NAND_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static u16 NAND_Read(u32 Memory_Offset, u8 *Readbuff, u16 Transfer_Length)
{ 
  SPARE_AREA sp;

  do 
  {
    sp = ReadSpareArea(Memory_Offset / 512);

    if (sp.BlockStatus !=0 )
    {
      /* Check block status and calculate start and end addreses */
      wAddress = NAND_GetAddress(Memory_Offset / 512);
	  FSMC_NAND_ReadSmallPage (Readbuff , wAddress, Transfer_Length);
	}
    Memory_Offset += 32*512;
  }while(sp.BlockStatus ==0);

  return NAND_OK;
}

/*******************************************************************************
* Function Name  : NAND_GetAddress
* Description    : Translate logical address into a phy one
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static NAND_ADDRESS NAND_GetAddress (u32 Address)
{
  NAND_ADDRESS Address_t;

  Address_t.Page  = Address & (NAND_BLOCK_SIZE - 1);
  Address_t.Block = Address / NAND_BLOCK_SIZE;
  Address_t.Zone = 0;

  while (Address_t.Block >= MAX_LOG_BLOCKS_PER_ZONE)
  {
    Address_t.Block -= MAX_LOG_BLOCKS_PER_ZONE;
    Address_t.Zone++;
  }
  return Address_t;
}


/*******************************************************************************
* Function Name  : ReadSpareArea
* Description    : Check used block
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static SPARE_AREA ReadSpareArea (u32 address)
{
  SPARE_AREA t;
  u8 Buffer[16];
  NAND_ADDRESS address_s;
  address_s = NAND_ConvertPhyAddress(address);
  FSMC_NAND_ReadSpareArea(Buffer , address_s, 1) ;

  t = *(SPARE_AREA *)Buffer;

  return t;
}

/*******************************************************************************
* Function Name  : NAND_ConvertPhyAddress
* Description    : None
* Input          : physical Address
* Output         : None
* Return         : Status
*******************************************************************************/
static NAND_ADDRESS NAND_ConvertPhyAddress (u32 Address)
{
  NAND_ADDRESS Address_t;

  Address_t.Page  = Address & (NAND_BLOCK_SIZE - 1);
  Address_t.Block = Address / NAND_BLOCK_SIZE;
  Address_t.Zone = 0;

  while (Address_t.Block >= MAX_PHY_BLOCKS_PER_ZONE)
  {
    Address_t.Block -= MAX_PHY_BLOCKS_PER_ZONE;
    Address_t.Zone++;
  }
  return Address_t;
}

/*******************************************************************************
* Function Name  : Media_BufferRead
* Description    : Read a buffer from the memory media 
* Input          : - pBuffer: Destination buffer address
*                : - ReadAddr: start reading position
*                : - NumByteToRead: size of the buffer to read
* Output         : None.
* Return         : None.
*******************************************************************************/
static void Media_BufferRead(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead)
{ 
  /* Read the data */
  while(NumByteToRead--)
  {
    *pBuffer++ = *(vu8 *)ReadAddr++;
  }
}

/*******************************************************************************
* Function Name  : InternalFlashCheckBitmapFiles
* Description    : Checks if the bitmap files (slides) are already loaded
*                  in the internal FLASH.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static u32 InternalFlashCheckBitmapFiles(void)
{
  u8 Tab[6];

  Tab[0] = *(vu8 *)(START_ADDR);
  Tab[1] = *(vu8 *)(START_ADDR + 1);
  Tab[2] = *(vu8 *)(START_ADDR + 2);
  Tab[3] = *(vu8 *)(START_ADDR + 3);
  Tab[4] = *(vu8 *)(START_ADDR + 4);
  Tab[5] = *(vu8 *)(START_ADDR + 5);

  if(Buffercmp(Tab, SlidesCheck, 5)!= 0) return 1;

  Tab[0] = *(vu8 *)(START_ADDR);
  Tab[1] = *(vu8 *)(START_ADDR + 0x25842 + 1);
  Tab[2] = *(vu8 *)(START_ADDR + 0x25842 + 2);
  Tab[3] = *(vu8 *)(START_ADDR + 0x25842 + 3);
  Tab[4] = *(vu8 *)(START_ADDR + 0x25842 + 4);
  Tab[5] = *(vu8 *)(START_ADDR + 0x25842 + 5);
  if(Buffercmp(Tab, SlidesCheck, 5)!= 0) return 1;

  return 0;
}

/*******************************************************************************
* Function Name  : ExternalSRAMCheckBitmapFiles
* Description    : Checks if the bitmap files (slides) are already loaded
*                  in the external SRAM.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static u32 ExternalSRAMCheckBitmapFiles(void)
{
  u8 Tab[6];

  /* Configure FSMC Bank1 NOR/SRAM3 */
  FSMC_SRAM_Init();

  Tab[0] = *(vu8 *)(Bank1_SRAM_ADDR);
  Tab[1] = *(vu8 *)(Bank1_SRAM_ADDR + 1);
  Tab[2] = *(vu8 *)(Bank1_SRAM_ADDR + 2);
  Tab[3] = *(vu8 *)(Bank1_SRAM_ADDR + 3);
  Tab[4] = *(vu8 *)(Bank1_SRAM_ADDR + 4);
  Tab[5] = *(vu8 *)(Bank1_SRAM_ADDR + 5);

  if(Buffercmp(Tab, SlidesCheck, 5)!= 0) return 1;

  Tab[0] = *(vu8 *)((Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)));
  Tab[1] = *(vu8 *)((Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)) + 1);
  Tab[2] = *(vu8 *)((Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)) + 2);
  Tab[3] = *(vu8 *)((Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)) + 3);
  Tab[4] = *(vu8 *)((Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)) + 4);
  Tab[5] = *(vu8 *)((Bank1_SRAM_ADDR + (2 * SLIDE_SIZE)) + 5);
  if(Buffercmp(Tab, SlidesCheck, 5)!= 0) return 1;

  return 0;
}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
