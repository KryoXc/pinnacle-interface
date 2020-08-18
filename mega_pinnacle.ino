//SoftwareSerial interface(2,3); // Probably need to be different pins. TX, RX

const byte ADDRESS = 0x80; // only 5 bits, 00000xxx
const int INTERVAL = 1000;
const int BUFFERSIZE = 64;

byte command_buffer[64];
int arc_density = -1;
unsigned long prev_time = 0;
boolean incoming_data = false;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(19200);
  Serial1.begin(19200, SERIAL_8O1); //Serial1 is 19(Rx), 18(Tx)
  Serial.println("<Arduino is ready>");
}


void loop() {

  // always be reading at the start of the loop for new data
  if (Serial1.available()) {
    testSerialEvent();
  }

  // grab time snapshot.
  unsigned long current_time = millis();
  if (current_time - prev_time >= INTERVAL) {
    prev_time = current_time;
    RequestSetpoints();
  }
}


int testSerialEvent() {
  byte val = Serial1.read();
  Serial.println("coming from Pinnacle: ");
  Serial.println(val);
  
  if (val == 0x06) {
    // ACK received
    Serial.println("ACK received. Expect more data.");
    incoming_data = true;
    
    //receive data. This method is blocking.
    // what if no data?
    int i = 0;
    while (Serial1.available()) {
      command_buffer[i] = Serial1.read();
    }
    // Validate data here
    Serial.print(i);
    Serial.println(" bytes received.");
    for(int k=0;k<i;k++) {
      Serial.print(command_buffer[k]);
    }
    Serial.print("\n");
    
    sendACK();
    
  } else if (val == 0x15) {
    // checksum failed or something. TODO: Test this later
    Serial.println("ERROR: received NAK from Pinnacle.");
  }

  return 0;
}


//***** Commands *****
int RequestArcDensity() {

  byte command[] = {ADDRESS, 0xA9, 0x00};
  byte checksum = makeChecksum(command, sizeof(command));
  Serial.print("checksum for RequestArcDensity: ");
  Serial.println(checksum, HEX);
  command[2] = checksum; // command[sizeof(command) - 1] = checksum

  Serial1.write(command, sizeof(command));
  return 0;
  
}


int RequestSetpoints() {

  byte command[] = {ADDRESS, 0xA9, 0x00}; 
  byte checksum = makeChecksum(command, sizeof(command));
  Serial.print("checksum for RequestSetpoints: ");
  Serial.println(checksum, HEX);
  command[2] = checksum;

  Serial1.write(command, sizeof(command));
  return 0;
}


void sendACK() {
  // 0x06 is the code for ACK
  Serial1.write(0x06);  
}


byte makeChecksum(byte data[], int len) {
  byte checksum = data[0];
  for (int i=1; i < len - 1; i++) {
    checksum ^= data[i]; 
  }

  return checksum;
}
