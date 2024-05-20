#define TCS3472_ADDR  0x29

void tcs3472_write8b(uint8_t reg, uint8_t data){
  Wire.beginTransmission(TCS3472_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

uint16_t tcs3472_read16b(uint8_t reg){
  uint8_t ret[2] = {0};
  Wire.beginTransmission(TCS3472_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  
  Wire.requestFrom(TCS3472_ADDR, 2);
  ret[0] = Wire.read();
  ret[1] = Wire.read();
  return ret[0] | (ret[1] << 8);
}

void tcs3472_Init(){
  tcs3472_write8b(0x83, 0xFF);// Write WTIME to default
  tcs3472_write8b(0x81, 0xFF);// Write ATIME to default
  tcs3472_write8b(0x80, 0x0B);// set WEN, PEN, AEN and PON bit   
}

void tcs3472_readColor(color_read_t *color_struct){
  color_struct->clear_color = tcs3472_read16b(0x14 | 0xA0);
  color_struct->red_color   = tcs3472_read16b(0x16 | 0xA0);
  color_struct->green_color = tcs3472_read16b(0x18 | 0xA0);
  color_struct->blue_color  = tcs3472_read16b(0x1A | 0xA0);  
}
