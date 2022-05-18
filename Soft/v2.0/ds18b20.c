////////////////////////////////////////////////////////////////////////////
////   Library for a Dallas 1820 Temperature chip                       ////
////                                                                    ////
////   init_temp();          Call before the other functions are used   ////
////                                                                    ////
////                                                                    ////
////   d = read_full_temp(); Read the temp in degrees C (0 to 125)      ////
////                                                                    ////
////////////////////////////////////////////////////////////////////////////


#ifndef DS1820_PIN
   #define DS1820_PIN  PIN_A2
#endif

#define TOUCH_PIN  DS1820_PIN
#include "touch.c"
void init_temp() {

}

float read_full_temp()
{
    byte i, buffer[9];
    float minstep;

    if (touch_present()) {         // get present (reset)                    (2)
        touch_write_byte(0xCC);    // Skip ROM                               (3)
        touch_write_byte (0x44);   // Start Conversion
        delay_ms(200);             // delay 200 ms                           (4)
        touch_present();           // get present (reset)                    (5)
        touch_write_byte(0xCC);    // Skip ROM                               (6)
        touch_write_byte (0xBE);   // Read Scratch Pad

        for(i=0; i<9;i++)          // read 9 bytes                           (7)
            buffer[i] = touch_read_byte();
    }


//minstep = (float)(buffer[0]/2)-0.25+((float)( buffer[7]-buffer[6])/buffer[7]);
     if(buffer[1]){
      minstep = (float)(-1)*(~make16(buffer[1],buffer[0])+1)/2;
     }else{
      minstep = (float)(make16(buffer[1],buffer[0])/2);
     }
      minstep = minstep - 0.25 + ((float)( buffer[7] - buffer[6])/buffer[7]);
      return (minstep);
}
