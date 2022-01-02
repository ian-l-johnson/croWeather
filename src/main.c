/*****************************************************************************
croWeather

   crow theme weather display

(c) ILJC 2020-2021
 ******************************************************************************/
#include <stdlib.h>     //exit()
#include <signal.h>     //signal()
#include <time.h>

#include "crow.h"


#define WEATHER_UPDATE_MIN  15   // in Minutes
#define WEATHER_TEMP_FILE   "/tmp/curWeather.json"
#define IMAGES_DIR          "./images/"
#define CROWFLY_IMG         "crowFly.bmp"
#define CROWSIT_IMG         "crowSit.bmp"
#define MIN_TO_SEC 60

//=======================================================

#define USE_EPAPER  1   // display on epaper, or not

//=======================================================


weatherCode_s WWO_code[] = {
   { 113, "Sunny",          sun },
   { 116, "PartlyCloudy",    partSun },
   { 119, "Cloudy",          cloud },
   { 122, "VeryCloudy",      cloud },
   { 143, "Fog",             cloud },
   { 176, "LightShowers",     rain },
   { 179, "LightSleetShowers",rain },
   { 182, "LightSleet",       rain },
   { 185, "LightSleet",       rain },
   { 200, "ThunderyShowers",  rain },
   { 227, "LightSnow",        snow },
   { 230, "HeavySnow",        snow },
   { 248, "Fog",              cloud },
   { 260, "Fog",              cloud },
   { 263, "LightShowers",     rain },
   { 266, "LightRain",        rain },
   { 281, "LightSleet",       rain },
   { 284, "LightSleet",       rain },
   { 293, "LightRain",        rain },
   { 296, "LightRain",        rain },
   { 299, "HeavyShowers",     rain },
   { 302, "HeavyRain",        rain },
   { 305, "HeavyShowers",     rain },
   { 308, "HeavyRain",        rain },
   { 311, "LightSleet",       rain },
   { 314, "LightSleet",       rain },
   { 317, "LightSleet",       rain },
   { 320, "LightSnow",        snow },
   { 323, "LightSnowShowers", snow },
   { 326, "LightSnowShowers", snow },
   { 329, "HeavySnow",        snow },
   { 332, "HeavySnow",        snow },
   { 335, "HeavySnowShowers", snow },
   { 338, "HeavySnow",        snow },
   { 350, "LightSleet",       rain },
   { 353, "LightShowers",     rain },
   { 356, "HeavyShowers",     rain },
   { 359, "HeavyRain",        rain },
   { 362, "LightSleetShowers",rain },
   { 365, "LightSleetShowers",rain },
   { 368, "LightSnowShowers", snow },
   { 371, "HeavySnowShowers", snow },
   { 374, "LightSleetShowers",rain },
   { 377, "LightSleet",       rain },
   { 386, "ThunderyShowers",  rain },
   { 389, "ThunderyHeavyRain",rain },
   { 392, "ThunderySnowShowers",rain },
   { 395, "HeavySnowShowers", snow },
   { 0,   "lastentry",        0 }
};


typedef struct {
   uint8_t   humidityPct;
   uint16_t  pressure;
   int8_t    temp_F;
   char      winddir[10];
   uint16_t  weatherCode;
   uint8_t   windspeedMiles;
} weather_s ;

char weatherImageName[100];


//----------------------------------------------------------------------------
char weatherDesc[100];
char *getWeatherDesc(uint16_t weatherCode)
{
   int i=0;

   weatherDesc[0]='?';
   weatherDesc[1]=0;

   while(i<100)
   {
      if(WWO_code[i].code == 0)
         break;

      if(WWO_code[i].code == weatherCode)
      {
         strcpy(weatherDesc, WWO_code[i].wdesc);
         break;
      }
      i++;
   }

   return(weatherDesc);
}

//----------------------------------------------------------------------------
char *getWeatherImageName(uint16_t weatherCode)
{
   int i=0;

   strcpy(weatherImageName, "cloud.bmp");  // default to cloudy

   while(i<100)
   {
      if(WWO_code[i].code == 0)  // reached the end
      {
         printf("ERROR: code not found - using cloudy (code=%d)\n",weatherCode);
         break;
      }

      if(WWO_code[i].code == weatherCode)
      {
         switch(WWO_code[i].imageType)
         {
            case(cloud):   strcpy(weatherImageName, "cloud.bmp");   break;
            case(partSun): strcpy(weatherImageName, "partSun.bmp"); break;
            case(rain):    strcpy(weatherImageName, "rain.bmp");    break;
            case(snow):    strcpy(weatherImageName, "snow.bmp");    break;
            case(sun):     strcpy(weatherImageName, "sun.bmp");     break;

            //case(nightClear): // not used in weather code list
            //case(windy):      // not used in weather code list
            default: // not found: error
               printf("ERROR: code not found - using cloudy (code=%d)\n",weatherCode);
               return(weatherImageName);
               break;
         };
         break;
      }
      i++;
   }

   return(weatherImageName);
}

//----------------------------------------------------------------------------
void Handler(int signo)
{
   //System Exit
   printf("\r\nHandler:exit\r\n");
   DEV_Module_Exit();
   exit(0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int main(void)
{
   char     tmpStr[1000];
   time_t   rawtime;
   struct tm *timeinfo;
   int      hours;
   int      ampm=0;
   weather_s weather;
   FILE     *jfp;
   char     jbuf[2000];
   int      jread;
   uint16_t tokCount=0;
   char*    pch, *spcch;
   char     lastTok[80];
   int      powerUp=1;


   // Exception handling:ctrl + c
//   signal(SIGINT, Handler);

   printf("\n");
   printf("+++++++++++++++++++++++++++++++\n");
   printf("++      croWeather v1.1      ++\n");
   printf("+++++++++++++++++++++++++++++++\n");

   printf("\nUpdating every %d minutes\n", WEATHER_UPDATE_MIN);

#if USE_EPAPER

   if (DEV_Module_Init() != 0)
      return -1;

   printf("e-Paper Init and Clear...\r\n");
   EPD_4IN2_Init();
   EPD_4IN2_Clear();
   DEV_Delay_ms(500);

   //Create a new image cache
   UBYTE *BlackImage;
   /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
   UWORD Imagesize = ((EPD_4IN2_WIDTH % 8 == 0) ? (EPD_4IN2_WIDTH / 8) : (EPD_4IN2_WIDTH / 8 + 1)) * EPD_4IN2_HEIGHT;
   if ((BlackImage = (UBYTE*) malloc(Imagesize)) == NULL)
   {
      printf("Failed to apply for black memory...\r\n");
      return -1;
   }
   Paint_NewImage(BlackImage, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, 180, WHITE);

#endif  // USE_EPAPER


   while(1)
   {

      // get current time
      time(&rawtime);
      timeinfo = localtime(&rawtime);
      hours = timeinfo->tm_hour;
      // adjust for 12h clock:
      ampm=0;  // default AM
      if(hours > 12)
      {
         hours = hours - 12;
         ampm=1;  // PM
      }
      if(hours == 0)
         hours = 12;
      if(hours == 12)
         ampm=1;


      // update weather every X minutes
      if(((timeinfo->tm_min % WEATHER_UPDATE_MIN) == 0) || powerUp)
      {
         powerUp=0;

         // remove old weather data
         system("rm "WEATHER_TEMP_FILE);

         // request new weather data
         system("./getWeather.sh");

         // try to read new weather data
         jfp = fopen(WEATHER_TEMP_FILE, "r");
         if(jfp)
         {
            jread = fread(jbuf,1,1000,jfp);
            if(jread > 70)
            {

               tokCount=0;
               pch = strtok (jbuf,"{}[]:\",");
               while (pch != NULL)
               {
               spcch=strchr(pch,' ');
               if(spcch==NULL)
               {
                     if(strcmp(pch,"request")==0)
                        break;
                     if(strcmp(lastTok,"humidity")==0)       weather.humidityPct = atoi(pch);
                     if(strcmp(lastTok,"pressure")==0)       weather.pressure = atoi(pch);
                     if(strcmp(lastTok,"temp_F")==0)         weather.temp_F = atoi(pch);
                     if(strcmp(lastTok,"weatherCode")==0)    weather.weatherCode = atoi(pch);
                     if(strcmp(lastTok,"winddir16Point")==0) strcpy(weather.winddir, pch);
                     if(strcmp(lastTok,"windspeedMiles")==0) weather.windspeedMiles = atoi(pch);

                     strcpy(lastTok,pch);
                     tokCount++;
               }

               pch = strtok (NULL, "{}[]:\",");
               }
   
               printf("----- Current Conditions ----------------\n");
               printf("code %d\n", weather.weatherCode);
               printf("[%s]\n", getWeatherDesc(weather.weatherCode));
               printf("%d F  \n", weather.temp_F);
               //printf("%d kPa\n", weather.pressure/10);
               printf("Wind from %s ", weather.winddir);
               printf("at %d mph\n", weather.windspeedMiles);
               printf("%d%% humidity\n", weather.humidityPct);
               printf("-----------------------------------------\n");

   #if USE_EPAPER
               // Paint_SelectImage(BlackImage);
               Paint_Clear(WHITE);
               GUI_ReadBmp(IMAGES_DIR "crowLineArt.bmp", 130, 1);


               #define X_POS    20
               #define Y_POS   130
               #define Y_GAP    30
               uint8_t wLine = 0;


               //Paint_DrawString_EN(X_POS, Y_POS+(Y_GAP*wLine++), weather.weatherDesc, &Font24, WHITE, BLACK);
               sprintf(tmpStr,"%s%s",IMAGES_DIR, getWeatherImageName(weather.weatherCode));
               GUI_ReadBmp(tmpStr, 20, 20);

               sprintf(tmpStr,"%d F",weather.temp_F);
               Paint_DrawString_EN(X_POS, Y_POS+(Y_GAP*wLine++), tmpStr, &Font24, WHITE, BLACK);
               wLine++;

               sprintf(tmpStr, "%d:%02d %s", hours, timeinfo->tm_min, ampm ? "pm" : "am");
               Paint_DrawString_EN(X_POS, Y_POS+(Y_GAP*wLine++), tmpStr, &Font20, WHITE, BLACK);

               // sprintf(tmpStr,"%d kPa",weather.pressure/10);
               // Paint_DrawString_EN(X_POS, Y_POS+(Y_GAP*wLine++), tmpStr, &Font20, WHITE, BLACK);

               sprintf(tmpStr, "%d%% humidity", weather.humidityPct);
               Paint_DrawString_EN(X_POS, Y_POS+(Y_GAP*wLine++), tmpStr, &Font20, WHITE, BLACK);

               sprintf(tmpStr,"Wind %s @ %dmph",weather.winddir, weather.windspeedMiles);
               Paint_DrawString_EN(X_POS, Y_POS+(Y_GAP*wLine++), tmpStr, &Font20, WHITE, BLACK);


               EPD_4IN2_Display(BlackImage);
   #endif  // USE_EPAPER
            }

         }
         else
         {
            printf("  --- UNABLE TO UPDATE WEATHER ---  \n");
            // TODO 23 mark as NOT updated....
         }
      }

      sleep(60);  // seconds

   }  // while(1)


   /* should never run...
   printf("Clear...\r\n");
   EPD_4IN2_Clear();

   printf("Goto Sleep...\r\n");
   EPD_4IN2_Sleep();
   free(BlackImage);
   BlackImage = NULL;

   // close 5V
   printf("close 5V, Module enters 0 power consumption ...\r\n");
   DEV_Module_Exit();
   */

   return 0;
}
