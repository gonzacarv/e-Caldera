///////////////////////////////////////////////////////////////////
//    Titulo: Firmware de control de Caldera de calefaccion      //
//    Autor: Gonzalo Carvallo                                    //
//    e-mail: gonzacarv@gmail.com    Fecha: Mayo de 2018         //
///////////////////////////////////////////////////////////////////

#include <16f877A.h>
#device ADC=8
#fuses HS, WDT, NOPROTECT, NOLVP, PUT, BROWNOUT // Opciones de configuración
#use delay(clock=20000000) //Clock speed HZ = 20MHZ
#include <flex_lcd.c>
#include <1wire.c>
#include <ds1820.c>
#include "DS1307.c"

///////////////////////////////// Definiciones de Pines /////////////////////////////////
#define TRMP1     PIN_A0  // Temperatura one wire (DS18B20) 1
#define TRMP2     PIN_A1  // Temperatura one wire (DS18B20) 2
#define TRMP3     PIN_A2  // Temperatura aux

#define CALOR1    PIN_B1  // Resistencias 1 (1 unidad RECO 2kW)
#define CALOR2    PIN_B2  // Resistencias 2 (1 unidad RECO 2kW)
#define CALOR3    PIN_B3  // Resistencias 3 (1 unidad RECO 2kW)
#define CALOR4    PIN_B0  // Resistencias 4 (1 unidad RECO 2kW)

#define ARRIBA    PIN_C0  // Pulsador 1 (Arriba)
#define ABAJO     PIN_C1  // Pulsador 2 (Abajo)
#define PROX      PIN_C2  // Pulsador 3 (Siguiente)
#define TECLA4    PIN_B4  // Auxiliar
#define TECLA5    PIN_B5  // Auxiliar

#define RELE3     PIN_D0  // Rele de bomba centrifuga
#define BBA       PIN_D1  // Rele de electrovalvula
#define VALVE     PIN_D2  // Rele auxiliar

#define LLENADO   PIN_A3  // Fin de carrera de llenado por evaporacion AN3
#define FCARRERA  PIN_A4  // Fin de carrera de corte por falta de nivel critico NoAN
#define CRITICO   PIN_A5  // Fin de carrera auxiliar AN4
/////////////////////////////////////////////////////////////////////////////////////////


/// Definiciones de posiciones de memoria
#define HSENCENDIDO    20
#define HSAPAGADO      22
#define POTMAX         24
#define TERMOSTAT      26

////// Variables del DS1307 /////
int sec;
int min;
int hrs;
/////////////////////////////////

/*  EN EL LCD
#define LCD_DB4   PIN_D4
#define LCD_DB5   PIN_C7
#define LCD_DB6   PIN_C6
#define LCD_DB7   PIN_C5

#define LCD_E     PIN_D5
#define LCD_RS    PIN_D7
#define LCD_RW    PIN_D5
*/
/////////////////// Funciones ///////////////////////
void Inicio();
void LeaTemp();
void SaleCalor(int RR);
void Teclado();
void Pantalla();
void Tiempo();
void Calefa();
void Automa();
void Criticos();
void ShutDown();
void Apagado();
void LeaAnalog();
/////////////////////////////////////////////////////

/////////////// VARIABLES GLOBALES ///////////////
short Rebote;
int Menu;
float TempIn;        // Temperatura Sensor 1 (entrada fria)
float TempOut;       // Temperatura Sensor 2 (salida caliente)
int intTempIn;       // Temperatura Sensor 1 (entrada fria)
int intTempOut;      // Temperatura Sensor 2 (salida caliente)
int Termostato;      // Temperatura del termostato
int HsOn;            // Hs de Encendido de Caldera
int HsOff;           // Hs de Apagado de Caldera
int MaxPower;        // Potencia Maxima
int HsConfig;        // Hora temporal para configuracion
int MinConfig;       // Minuto temporal para configuracion
int PotActual;       // Potencia Actual
int EstadoCritico;   // Almacena el estado de funcionamiento de la caldera
int ii;              // Contador complejo
int CritExS1;        // Contador de exceso critico de temperatura en Sensor1
int CritExS2;        // Contador de exceso critico de temperatura en Sensor2
int CritErS1;        // Contador de errores criticos en lectura de temperatura en Sensor1
int CritErS2;        // Contador de errores criticos en lectura de temperatura en Sensor2
short Llenar;        // Variable de llenado con electro sin error
short NCritico;      // Variable de corte por nivel critico

//////////////////////////////////////////////////

void main(){
int Aux;
Inicio();
LeaAnalog();
while (1) {
/*
while (1){
lcd_putc("\f"); 
LeaAnalog();
restart_wdt(); // Reiniciamos el perro
}*/

++Aux;
if (Aux == 255) { // Cada mas o menos 5 segundos
++ii;
LeaAnalog();
restart_wdt(); // Reiniciamos el perro
LeaTemp();
restart_wdt(); // Reiniciamos el perro
Automa();
if (Llenar == True) output_high(VALVE);
else output_low(VALVE);
if ((ii % 4) == 0) { // reseteo variables de error
CritExS1 = 0;
CritExS2 = 0;
CritErS1 = 0;
CritErS2 = 0;
} // reset error
} // Cada 5 segundos

restart_wdt(); // Reiniciamos el perro
Tiempo();
restart_wdt(); // Reiniciamos el perro
Teclado();
restart_wdt(); // Reiniciamos el perro
Pantalla();
restart_wdt(); // Reiniciamos el perro

Criticos(); // dentro de esta se llama a Calefa();
restart_wdt(); // Reiniciamos el perro
} // while 1
} // main


void Inicio(){
setup_wdt(WDT_2304MS); // Perro mas largo
lcd_init();
DS1307_init();
delay_ms(5);
lcd_putc("\f"); 

setup_adc_ports(NO_ANALOGS);
setup_adc(ADC_OFF); 
setup_psp(PSP_DISABLED); 
setup_spi(FALSE); 
setup_timer_0(RTCC_INTERNAL|RTCC_DIV_1); 
setup_timer_1(T1_DISABLED); 
setup_timer_2(T2_DISABLED,0,1); 
setup_comparator(NC_NC_NC_NC); 
setup_vref(VREF_LOW|-2); 

HsOn = read_eeprom(HSENCENDIDO);
if (HsOn > 47) HsOn = 36; 
HsOff = read_eeprom(HSAPAGADO);
if (HsOff > 47) HsOff = 12; 
MaxPower = read_eeprom(POTMAX);
if ((MaxPower > 4) || (MaxPower == 0)) MaxPower = 4; // Potencia multiplica por 2 
Termostato = read_eeprom(TERMOSTAT);
if ((Termostato > 18) || (Termostato < 3)) Termostato = 13; // Temperatura multiplica por 5

Menu = 1;
ii = 0;
Llenar = 0;
NCritico = 0;
CritExS1 = 0;
CritExS2 = 0;
CritErS1 = 0;
CritErS2 = 0;
EstadoCritico = 0;
SaleCalor(0);
ds1307_get_time(hrs,min,sec);
delay_ms(2);
if ((hrs > 23) || (min > 59) || (sec > 59)) ds1307_set_time(0, 0, 0);
LeaTemp(); // Para que lea la temperatura por primera vez y no nos tire error
}

void LeaAnalog(){
restart_wdt(); // Reiniciamos el perro
int  val;
int  V1;
int  V2;

setup_adc(ADC_CLOCK_INTERNAL);
setup_adc_ports(AN0_AN1_AN2_AN3_AN4);

set_adc_channel(3);
delay_ms(5);
val = read_adc();
V1 = val;
if (val > 170) Llenar = True;
else Llenar = False;

set_adc_channel(4);
delay_ms(5);
val = read_adc();
V2 = val;
if (val > 170) NCritico = True;
else NCritico = False;

setup_adc_ports(NO_ANALOGS);
setup_adc(ADC_OFF); 
/*
lcd_gotoxy(1,1);
printf(lcd_putc,"V1 %03u", V1);
lcd_gotoxy(1,2);
printf(lcd_putc,"V2 %03u", V2);
restart_wdt(); // Reiniciamos el perro
delay_ms(200);
restart_wdt(); // Reiniciamos el perro
*/
} //Fn

void LeaTemp(){
restart_wdt(); // Reiniciamos el perro
Teclado();
TempIn = ds1820_read1(); 
intTempIn = (int) TempIn;
Pantalla();
restart_wdt(); // Reiniciamos el perro
Teclado();
TempOut = ds1820_read2();
intTempOut = (int) TempOut;
Pantalla();
restart_wdt(); // Reiniciamos el perro
}

void SaleCalor(int RR){
switch(RR)
{
   case 0:
      output_low(CALOR1);
      output_low(CALOR2);
      output_low(CALOR3);
      output_low(CALOR4);
      break;

   case 1:
      output_high(CALOR1);
      output_low(CALOR2);
      output_low(CALOR3);
      output_low(CALOR4);
      break;

   case 2:
      output_low(CALOR1);
      output_high(CALOR2);
      output_high(CALOR3);
      output_low(CALOR4);
      break;

   case 3:
      output_low(CALOR1);
      output_high(CALOR2);
      output_high(CALOR3);
      output_high(CALOR4);
      break;

   case 4:
      output_high(CALOR1);
      output_high(CALOR2);
      output_high(CALOR3);
      output_high(CALOR4);
      break;
/*
   case 5:
      output_low(CALOR1);
      output_high(CALOR1B);
      output_low(CALOR2);
      output_high(CALOR4);
      break;

   case 6:
      output_low(CALOR1);
      output_low(CALOR1B);
      output_high(CALOR2);
      output_high(CALOR4);
      break;

   case 7:
      output_low(CALOR1);
      output_high(CALOR1B);
      output_high(CALOR2);
      output_high(CALOR4);
      break;

   case 8:
      output_high(CALOR1);
      output_high(CALOR1B);
      output_high(CALOR2);
      output_high(CALOR4);
      break;
*/
   defalut:
      output_low(CALOR1);
      output_low(CALOR2);
      output_low(CALOR3);
      output_low(CALOR4);
      break;
}
 PotActual = RR;
 restart_wdt(); // Reiniciamos el perro
}

void Tiempo(){
ds1307_get_time(hrs,min,sec);
delay_ms(2);
}

void Teclado(){

restart_wdt();
if (input(ARRIBA)==0 && input(ABAJO)==0 && input(PROX)==0) Rebote = false; // Soltamos las teclas
restart_wdt(); // Reiniciamos el perro
if (Rebote == false){ // Tomamos el comando ya que el antirebote nos dice que esta ok

  if (input(ARRIBA)==1 && input(ABAJO)==1){ // Reseteamos el estado critico
  Rebote = True;
  EstadoCritico = 0;
  }

  if (input(PROX)==1){
  Rebote = True;
  Menu = Menu + 1;
  if (Menu == 9) Menu = 1; 
  }

switch (Menu){
   case 3:
      if (input(ARRIBA)==1){
      Rebote = True;
      Termostato = Termostato + 1;
      if (Termostato == 18) Termostato = 17;
      write_eeprom(TERMOSTAT,Termostato);
      delay_ms(5);
      }
      
      if (input(ABAJO)==1){
      Rebote = True;
      Termostato = Termostato - 1;
      if (Termostato == 7) Termostato = 8; // El 7 es APAGADO (Termostato x 5 da la temperatura en °C)
      write_eeprom(TERMOSTAT,Termostato);
      delay_ms(5);
      }
   break;

   case 4:
      if (input(ARRIBA)==1){
      Rebote = True;
      HsOn = HsOn + 1;
      if (HsOn == 49) HsOn = 0;
      write_eeprom(HSENCENDIDO,HsOn);
      delay_ms(5);
      }
      
      if (input(ABAJO)==1){
      Rebote = True;
      HsOn = HsOn - 1;
      if (HsOn == 255) HsOn = 48;
      write_eeprom(HSENCENDIDO,HsOn);
      delay_ms(5);
      }
   break;

   case 5:
      if (input(ARRIBA)==1){
      Rebote = True;
      HsOff = HsOff + 1;
      if (HsOff == 49) HsOff = 0;
      write_eeprom(HSAPAGADO,HsOff);
      delay_ms(5);
      }
      
      if (input(ABAJO)==1){
      Rebote = True;
      HsOff = HsOff - 1;
      if (HsOff == 255) HsOff = 48;
      write_eeprom(HSAPAGADO,HsOff);
      delay_ms(5);
      }
   break;

   case 2:
      if (input(ARRIBA)==1){
      Rebote = True;
      MaxPower = MaxPower + 1;
      if (MaxPower == 5) MaxPower = 4;
      write_eeprom(POTMAX,MaxPower);
      delay_ms(5);
      }
      
      if (input(ABAJO)==1){
      Rebote = True;
      MaxPower = MaxPower - 1;
      if (MaxPower == 0) MaxPower = 1;
      write_eeprom(POTMAX,MaxPower);
      delay_ms(5);
      }
   break;

   case 1:
      if (input(ARRIBA)==1) output_high(BBA);
      else output_low(BBA);
      
      if (input(ABAJO)==1) output_high(VALVE);
      else output_low(VALVE);
   break;

  case 6:
      ds1307_get_time(HsConfig,MinConfig,sec);
      delay_ms(2);
      Menu = 7;
  break;

  case 7:
      if (input(ARRIBA)==1){
      Rebote = True;
      HsConfig = HsConfig + 1;
      if (HsConfig > 23) HsConfig = 0;
      ds1307_set_time(HsConfig, MinConfig, 0);
      }
      
      if (input(ABAJO)==1){
      Rebote = True;
      HsConfig = HsConfig - 1;
      if (HsConfig == 255) HsConfig = 23;
      ds1307_set_time(HsConfig, MinConfig, 0);
      }
   break;
   
  case 8:
      if (input(ARRIBA)==1){
      Rebote = True;
      MinConfig = MinConfig + 1;
      if (MinConfig > 59) MinConfig = 0;
      ds1307_set_time(HsConfig, MinConfig, 0);
      }
      
      if (input(ABAJO)==1){
      Rebote = True;
      MinConfig = MinConfig - 1;
      if (MinConfig == 255) MinConfig = 59;
      ds1307_set_time(HsConfig, MinConfig, 0);
      }
   break;
}

}//rebote falso
} //fn


void Pantalla(){
   lcd_gotoxy(16,1);
   if (Llenar) printf(lcd_putc,"#");
   else printf(lcd_putc," ");
   lcd_gotoxy(1,1);
   if (EstadoCritico == 0) {
         if (bit_test(ii,0)) printf(lcd_putc,"TO:%4.1f%cC %02d:%02d", TempOut,223,hrs,min);
         else printf(lcd_putc,"TI:%4.1f%cC %02d:%02d", TempIn,223,hrs,min);
   }
   if (EstadoCritico == 1) printf(lcd_putc,"EXCESO DE TEMPER"); 
   if (EstadoCritico == 2) printf(lcd_putc,"BAJO NIVEL AGUA "); 
   if (EstadoCritico == 3) printf(lcd_putc,"FALLA DE SENSOR ");
   restart_wdt(); // Reiniciamos el perro

switch (Menu){
   case 3:
   lcd_gotoxy(1,2);
   if (Termostato == 7) printf(lcd_putc,"Ter: OFF P: %d kW",(PotActual * 2)); 
   else printf(lcd_putc,"Ter:%2d%cC P: %d kW", Termostato*5,223,(PotActual * 2)); 
   break;

   case 4:
   lcd_gotoxy(1,2); 
   if (HsOn == 48 ) printf(lcd_putc,"Hs Encend: OFF  ");
   else printf(lcd_putc,"Hs Encend: %02d:%02d", HsOn/2,(HsOn%2)*30); 
   break;

   case 5:
   lcd_gotoxy(1,2); 
   if (HsOff == 48 ) printf(lcd_putc,"Hs Apagad: OFF  "); 
   printf(lcd_putc,"Hs Apagad: %02d:%02d", HsOff/2,(HsOff%2)*30);
   break;

   case 2:
   lcd_gotoxy(1,2); 
   printf(lcd_putc,"Limite Pot: %d kW", (MaxPower * 2)); 
   break;

   case 1:
   lcd_gotoxy(1,2); 
   printf(lcd_putc,"Ar: Bba Ab: Valv"); 
   break;

   case 6:
   lcd_gotoxy(1,2); 
   printf(lcd_putc,"Ajuste de Hora  "); 
   break;

   case 7:
   lcd_gotoxy(1,2); 
   printf(lcd_putc,"Ajuste de Hora  "); 
   break;

   case 8:
   lcd_gotoxy(1,2); 
   printf(lcd_putc,"Ajuste de Minuto"); 
   break;
} //SW
} //FN

void Calefa(){
int diferencia;
int PromTemp;
PromTemp = ((intTempOut + intTempIn) / 2);
if (Menu == 3){ // Funcionando
restart_wdt(); // Reiniciamos el perro
if (Termostato == 7) Apagado();
else{
output_high(BBA);
if (PromTemp >= (Termostato * 5)) SaleCalor(0);
else { //Termostato en aumento
diferencia = ((Termostato * 5) - PromTemp);
if (diferencia == 1) SaleCalor(1);
if (diferencia == 2) if (MaxPower >= 2) SaleCalor(2); else SaleCalor(MaxPower);
if (diferencia == 3) if (MaxPower >= 3) SaleCalor(3); else SaleCalor(MaxPower);
if (diferencia >= 4) if (MaxPower >= 4) SaleCalor(4); else SaleCalor(MaxPower);
/*
if (diferencia < 2 ) SaleCalor(1);
if ((diferencia >= 2 ) && (diferencia < 4)) if (MaxPower >= 2) SaleCalor(2); else SaleCalor(MaxPower);
if ((diferencia >= 4) && (diferencia < 5)) if (MaxPower >= 3) SaleCalor(3); else SaleCalor(MaxPower);
if ((diferencia >= 5) && (diferencia < 6)) if (MaxPower >= 4) SaleCalor(4); else SaleCalor(MaxPower);
if ((diferencia >= 6) && (diferencia < 7)) if (MaxPower >= 5) SaleCalor(5); else SaleCalor(MaxPower);
if ((diferencia >= 7) && (diferencia < 8)) if (MaxPower >= 6) SaleCalor(6); else SaleCalor(MaxPower);
if ((diferencia >= 8) && (diferencia < 9)) if (MaxPower >= 7) SaleCalor(7); else SaleCalor(MaxPower);
if ( diferencia >= 9) if (MaxPower >= 8) SaleCalor(8); else SaleCalor(MaxPower);
*/
} // Termostato en aumento
}
} else ShutDown(); // Menu es 2 o sea operacion normal
} // Fn

void Automa(){
if (((HsOn / 2) == hrs) && (((HsOn % 2) * 30) == min)){
Menu = 3;
// EX DISEŃO if (Termostato == 7) Termostato = read_eeprom(TERMOSTAT);
// EX DISEŃO if (Termostato == 7) Termostato = 13; //por si estaba apagada
}//On conincide

if (((HsOff / 2) == hrs) && (((HsOff % 2) * 30) == min)){
Menu = 2;
// EX DISEŃO Termostato = 7;
}//Off conincide
}//fn

void Criticos(){
if ((intTempOut > 86) && (intTempOut <= 100)) ++CritExS1; 
if ((intTempIn  > 86) && (intTempIn  <= 100)) ++CritExS2; 
if ((CritExS1 > 2) || (CritExS2 > 2)) { ShutDown(); EstadoCritico = 1; }

/*if (input(CRITICO) == 1) { //Evitamos falsas lecturas
   delay_ms(500);
   if (input(CRITICO) == 1) {
      delay_ms(500);
      if (input(CRITICO) == 1) {
         ShutDown();
         EstadoCritico = 2;
      }
   }
} // Nivel critico */

if (NCritico == True){
   ShutDown();
   EstadoCritico = 2;
}

if ((intTempOut > 100) || (intTempOut < 4)) ++CritErS1;
if ((intTempIn  > 100) || (intTempIn  < 4)) ++CritErS2; 
if ((CritErS1 > 2) || (CritErS2 > 2)) { ShutDown(); EstadoCritico = 3; }


if (EstadoCritico == 0){
if (Menu == 3){
Calefa();
output_high(BBA);
//if (input(LLENADO) == 1) Llenar = True;
//else Llenar = False;
} else ShutDown(); // Menu 1
} //Estado Critico 0
} // fn

void ShutDown(){
if ((intTempOut < 40) && (intTempIn < 40)) output_low(BBA);
else output_high(BBA);
output_low(VALVE);
SaleCalor(0);
}

void Apagado(){
if ((intTempOut < 40) && (intTempIn < 40)) output_low(BBA);
else output_high(BBA);
output_low(VALVE);
SaleCalor(0);
}


