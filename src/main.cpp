/*#include <Arduino.h>

#include "TFT_eSPI.h"

#define POWERRAIL_EN_PIN  (gpio_num_t)D4 //GPIO_NUM_5

class Utility{
    public:
        TFT_eSPI tft = TFT_eSPI(); 

        Utility(){
            powerOn();
        }
        
        void draw(uint16_t color){            
            tft.fillRect(20,20,200,280,color);
        }

        void init(){
            // ******** Clé : rediriger le SPI global vers HSPI ********
            //SPI.end();                                  // au cas où
            //SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);    // pour que le SPI globalai les memes broches que le HSPI            
            //powerOn();

            tft.init(1);     
            tft.fillScreen(TFT_BLUE);
            draw(TFT_BLACK);
        }

        void powerOn(){
            pinMode(POWERRAIL_EN_PIN, OUTPUT);
            digitalWrite(POWERRAIL_EN_PIN, 1);  
        }
};

Utility utility;


#include "TestMIDIHelper.h"*/


#include "MixerApplication.h"