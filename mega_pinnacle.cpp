/**
 * mega_pinnacle.ino
 * 
 * Author: Diego Morales
 * 1/5/2020
 * 
 */

#include <Wire.h>

const byte ADDRESS = 0x08; // only 5 bits, 00000xxx
const int INTERVAL = 500;
const int BUFFERSIZE = 64;
const int DAC_RESOLUTION = 4096;

byte command_buffer[BUFFERSIZE];
byte output_buffer[BUFFERSIZE];
byte dac_buffer[3];

unsigned long prev_time = 0;
boolean vcp = false;
int expected = 0;
int bytes_received = 0;


void setup() {

  Serial.begin(19200);
  Serial1.begin(19200, SERIAL_8O1); //Serial1 is 19(Rx), 18(Tx)
  pinMode(A10, OUTPUT);
  analogWrite(9, 153);
  Wire.begin();
  Serial.println("<Arduino is ready>");
}


void loop() {

  if (Serial1.available()) {
    testSerialEvent();
  }
  
  // grab time snapshot. `vcp` is a flag that toggles the current request function between VCP and arcs.
  unsigned long current_time = millis();
  if (current_time - prev_time >= INTERVAL) {
    prev_time = current_time;
    if(vcp) {
      RequestVCP();
    } else {
      RequestArcDensity();
    }
    vcp = !vcp;
  }
}


int testSerialEvent() {
  byte val = Serial1.read();
  
  if (bytes_received < expected) {
    command_buffer[bytes_received] = val;
    bytes_received++;
  } else {
    Serial.println("response: ");
    for (int j = 0; j < expected; j++) {
      Serial.print(command_buffer[j], HEX);
      Serial.print(" ");
      Serial.print(command_buffer[j], BIN);
      Serial.print(" ");
      Serial.println(command_buffer[j]); 
    }
    sendACK();
    memcpy(output_buffer, command_buffer, 64); //copy full size of buffer
    memset(command_buffer, 0, 64);
    
    if(output_buffer[2] == 0xA8) {
      outputVCP();
    } else {
      outputArcDensity();
    }
    // Reset request state
    bytes_received = 0;
    expected = 0;
  }
  return 0;
}


//***** Commands *****
int RequestArcDensity() {

  expected = 7;
  byte command[] = {ADDRESS, 0xBC, 0x00};
  byte checksum = makeChecksum(command, sizeof(command));
  command[2] = checksum; // command[sizeof(command) - 1] = checksum

  Serial.println("\n===========\nRequesting Arc Density...");
  
  Serial1.write(command, sizeof(command));
  return 0;
  
}


int RequestRegulationMode() {

  expected = 6;
  byte command[] = {ADDRESS, 0xA4, 0x00};
  byte checksum = makeChecksum(command, sizeof(command));
  Serial.print("checksum for RequestRegulationMode: ");
  Serial.println(checksum, HEX);
  command[2] = checksum; // command[sizeof(command) - 1] = checksum
  
  Serial1.write(command, sizeof(command));
  return 0;
  
}


int RequestSetpoints() {

  expected = 9;
  byte command[] = {ADDRESS, 0xA9, 0x00}; 
  byte checksum = makeChecksum(command, sizeof(command));
  command[2] = checksum;
  
  Serial.println("\n==========\nRequesting Setpoints...");
  Serial1.write(command, sizeof(command));
  return 0;
}


int RequestVCP() {
  
  expected = 9;
  byte command[] = {ADDRESS, 0xA8, 0x00};
  byte checksum = makeChecksum(command, sizeof(command));
  command[2] = checksum;

  Serial.println("Requesting Power, Current, and Voltage...");
  
  Serial1.write(command, sizeof(command));
}


// ***** Utilities *****

void sendACK() {
 
  Serial1.write(0x06); // 0x06 is the code for ACK
}


byte makeChecksum(byte data[], int len) {
  
  byte checksum = data[0];
  for (int i = 1; i < len; i++) {
    checksum ^= data[i]; 
  }

  return checksum;
}


// ***** Monitor Output *****

void outputVCP() {

  float power = output_buffer[3] | output_buffer[4] << 8;
  power = power / 100.0;
  
  float current = output_buffer[5] | output_buffer[6] << 8;
  current = current / 100.0;

  float voltage = output_buffer[7] | output_buffer[8] << 8;

  Serial.print("power: ");
  Serial.println(power);  
  Serial.print("current: ");
  Serial.println(current);
  Serial.print("voltage: ");
  Serial.println(voltage);

  // TODO Set max power so that the read power will never cross it.
  // power is is Watts?
  float max_power = 500;

  // scale power to fit resolution, send to DAC
  unsigned int bpower = ceil((power / max_power) * DAC_RESOLUTION);  

  dac_buffer[0] = 0x40; // Sets the control byte to write mode
  dac_buffer[1] = bpower >> 4;
  dac_buffer[2] = bpower << 4;

  Wire.beginTransmission(0x62);
  Wire.write(dac_buffer, 3);
  Wire.endTransmission();
}


void outputArcDensity() {
  
  float soft_arcs = output_buffer[3] | output_buffer[4] << 8;
  
  float hard_arcs = output_buffer[5] | output_buffer[6] << 8;

  Serial.print("Soft arc/s: ");
  Serial.println(soft_arcs);
  Serial.print("Hard arc/s: ");
  Serial.println(hard_arcs);

  // TODO set max arcs/sec so that the read arcs/sec will never cross it.
  float max_arcs = 100;

  unsigned int barcs = ceil((hard_arcs / max_arcs) * DAC_RESOLUTION);

  dac_buffer[0] = 0x40; // Set the control byte to write mode
  dac_buffer[1] = barcs >> 4;
  dac_buffer[2] = barcs << 4;

  Wire.beginTransmission(0x63);
  Wire.write(dac_buffer, 3);
  Wire.endTransmission();
}
