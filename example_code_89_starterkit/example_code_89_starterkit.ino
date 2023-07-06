#include <Arduino.h>
#include <Wire.h>

#define ADDR 0x28 //define the default slave address
#define BUFFER_SIZE 7 //define the buffer size to read 7 bytes from response

// Define the write command
uint8_t writeByte = 0xAA; 

// Create a buffer to hold the read bytes
uint8_t readByte[BUFFER_SIZE]; 

/**
 * Function to send a command to a slave device over I2C and read the response
 * @param num Number of bytes to read from the slave device
 */
void getDataFromSensor(int num) {
  Wire.beginTransmission(ADDR); // Begin transmission to the device with address ADDR
  Wire.write(writeByte); // Send the write command to the slave device
  Wire.endTransmission(); // End transmission
  
  delay(100); // Delay for 100ms to give the slave device time to process the command
  
  Wire.requestFrom(ADDR, num); // Request num bytes from the slave device
  
  // Loop over each byte in the response
  for (int i = 0; i < num; i++) {
    // Initialize the byte in the read buffer to 0
    readByte[i] = 0; 
    
    // If there is a byte available to read, read it into the buffer
    if (Wire.available())
      readByte[i] = Wire.read();
    else
      readByte[i] = 0; // If no byte is available to read, set the byte in the buffer to 0
  }
}

/**
 * Function to convert the raw temperature value to Celsius
 * using the given formula: (raw_val/2^16)*50
 * @param rawTemperature - the raw temperature value
 * @return the converted temperature value in Celsius
 */
float convertTemperature(uint32_t rawTemperature) {
  float convertedTemperature = (rawTemperature / pow(2, 16)) * 50;
  return convertedTemperature;
}

/**
 * Function to convert the raw pressure value to bar
 * using the given formula: (x-10%adc_resolution)/80%adc_resolution*fs
 * where fs is the full scale value in bar and
 * adc_resolution is 2^n for a n-bit ADC
 * @param rawPressure - the raw pressure value
 * @return the converted pressure value in bar
 */
float convertPressure(uint32_t rawPressure) {
  float adc_resolution = pow(2, 16);
  float fs = 2.5; // full scale in bar
  float offset = 0; // offset from 6500 raw counts at ambient pressure
  float convertedPressure = ((rawPressure + offset - 0.10 * adc_resolution) / (0.80 * adc_resolution)) * fs;
  return convertedPressure;
}

void printRawBytes() {
  // Print the raw received sensor values
  Serial.println("raw sensor values:");
  for (int i = 0; i < BUFFER_SIZE; i++) {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": 0x");
    Serial.println(readByte[i], HEX);
  }
}

void printStatus() {
  // Interpreting the status byte
  Serial.println("\nStatus byte interpretation:");
  Serial.println(readByte[0] & 0x01 ? "Math saturation: Yes" : "Math saturation: No");
  Serial.println(readByte[0] & 0x02 ? "Connection check fault: Yes" : "Connection check fault: No");
  Serial.println(readByte[0] & 0x04 ? "Memory error: Yes" : "Memory error: No");
  Serial.println((readByte[0] & 0x18) >> 3 == 0 ? "Mode: 0" : (readByte[0] & 0x18) >> 3 == 1 ? "Mode: 1" : (readByte[0] & 0x18) >> 3 == 2 ? "Mode: 2" : "Mode: 3");
  Serial.println(readByte[0] & 0x20 ? "Busy: Yes" : "Busy: No");
  Serial.println(readByte[0] & 0x40 ? "Power: On" : "Power: Off");
  Serial.println();
}

void printPressure() {
  uint32_t pressureData = 0;

  for (int i = 1; i < 3; i++) { // First two bytes after the status byte represent pressure
    pressureData |= ((uint32_t)readByte[i] << ((2-i) * 8));
  }

  Serial.print("Pressure: ");
  float convertedPressure = convertPressure(pressureData);
  Serial.print(convertedPressure);
  Serial.println(" bar");
}

void printTemperature() {
  uint32_t temperatureData = 0;

  for (int i = 4; i < 7; i++) { // Next two bytes represent temperature
    temperatureData |= ((uint32_t)readByte[i] << ((6-i) * 8));
  }
  
  Serial.print("Temperature: ");
  float convertedTemp = convertTemperature(temperatureData); 
  Serial.print(convertedTemp);
  Serial.println(" C");
}

void printAll() {
  Serial.println("\n-------------OUTPUT--------------\n");
  printRawBytes();
  printStatus();
  printPressure();
  printTemperature();
  Serial.println("\n---------------------------------");
}


void setup() {
  Wire.begin(); // Initialize the I2C interface
  Serial.begin(9600); // Start the serial communication with the baud rate of 9600
}


void loop() {
  getDataFromSensor(BUFFER_SIZE); // Call the getDataFromSensor function to send a command and read the response
  printAll(); // Print the received sensor values
  delay(250); // Wait for 250ms before the next loop iteration
}
