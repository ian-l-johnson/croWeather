/*
 * crow.h
 *
 *  Created on: Jan 24, 2020
 *      Author: ian
 */
#ifndef _CROW_H_
#define _CROW_H_

#include <stdio.h>
#include <stdlib.h>
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "GUI_BMPfile.h"

// Display resolution
#define EPD_4IN2_WIDTH       400
#define EPD_4IN2_HEIGHT      300


typedef enum {
   cloud,
   nightClear,
   partSun,
   rain,
   snow,
   sun,
   windy,
} wPics_e;

typedef struct {
   uint16_t code;
   char     wdesc[60];
   wPics_e  imageType;
} weatherCode_s;


void EPD_4IN2_Init(void);
void EPD_4IN2_Clear(void);
void EPD_4IN2_Display(UBYTE *Image);
void EPD_4IN2_Sleep(void);


#endif /* _CROW_H_ */
