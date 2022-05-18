float ds1820_read1() 
{ 
 int8 busy=0, temp1, temp2; 
 signed int16 temp3; 
 float result; 

 onewire1_reset(); 
 onewire1_write(0xCC); 
 onewire1_write(0x44); 

 while (busy == 0) 
  busy = onewire1_read(); 

 onewire1_reset(); 
 onewire1_write(0xCC); 
 onewire1_write(0xBE); 
 temp1 = onewire1_read(); 
 temp2 = onewire1_read(); 
 temp3 = make16(temp2, temp1); 
  
// result = (float) temp3 / 2.0;   //Calculation for DS18S20 with 0.5 deg C resolution 
 result = (float) temp3 / 16.0;  //Calculation for DS18B20 with 0.1 deg C resolution 
  
 delay_ms(200); 
 return(result); 
}

float ds1820_read2() 
{ 
 int8 busy=0, temp1, temp2; 
 signed int16 temp3; 
 float result; 

 onewire2_reset(); 
 onewire2_write(0xCC); 
 onewire2_write(0x44); 

 while (busy == 0) 
  busy = onewire2_read(); 

 onewire2_reset(); 
 onewire2_write(0xCC); 
 onewire2_write(0xBE); 
 temp1 = onewire2_read(); 
 temp2 = onewire2_read(); 
 temp3 = make16(temp2, temp1); 
  
// result = (float) temp3 / 2.0;   //Calculation for DS18S20 with 0.5 deg C resolution 
 result = (float) temp3 / 16.0;  //Calculation for DS18B20 with 0.1 deg C resolution 
  
 delay_ms(200); 
 return(result); 
}
