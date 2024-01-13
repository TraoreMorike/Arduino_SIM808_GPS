#include <SoftwareSerial.h>

/* --- Macros definitions --- */
#define PWRKEY_PIN	  9
#define	SIM808_RX_PIN	7
#define SIM808_TX_PIN	8

#define HARDWARE_SERIAL_BAUDRATE	9600
#define SIM808_SERIAL_BAUDRATE		9600

#define MAX_INPUT_LENGTH    10
#define DEBUG 0

//#define GPS_ON_REQUEST
#define AUTONOMUS_GPS

/* --- Global variable definitions --- */
#ifdef GPS_ON_REQUEST
  char inputString[MAX_INPUT_LENGTH + 1];
#endif

unsigned long currentTime = 0;

/* --- Objects definitions --- */
SoftwareSerial mySerial(SIM808_RX_PIN, SIM808_TX_PIN);

void setup() {
  
  #ifdef GPS_ON_REQUEST
    // clear the string commands input buffer
    memset(inputString, 0, sizeof(inputString));
  #endif

	// Setting up Serial interface 
	Serial.begin(HARDWARE_SERIAL_BAUDRATE);
	mySerial.begin(SIM808_SERIAL_BAUDRATE);
	
  if (!checkSIM808powerState()) {
    // Power on SIM808
    pinMode(PWRKEY_PIN, OUTPUT);
    digitalWrite(PWRKEY_PIN, HIGH);
    delay(3000);
    digitalWrite(PWRKEY_PIN, LOW);
    delay(1500);
	
    delay(2000);
  }

	// Set up SIM808 GPS 
	// Power up GPS
	send_AT_CMD("AT+CGNSPWR=1", 1000, DEBUG);
	// Configure NMEA sentences | Time, date, position, course and speed data 
	send_AT_CMD("AT+CGNSSEQ=RMC", 1000, DEBUG);

  delay(2000);

}

void loop() {
  
  #ifdef AUTONOMUS_GPS
    unsigned long elapsedTime = millis() - currentTime;

    if (elapsedTime >= 1000) {
      
      // Get GPS coordinates 
      parseGpsInfo(send_AT_CMD("AT+CGNSINF", 1000, DEBUG));
      currentTime = millis();
    }
    
    else {
      return;
    }
  #endif

  #ifdef GPS_ON_REQUEST
    if (Serial.available()) {
      // Read characthers from serial port until \n is received
      int input = Serial.readBytesUntil('\n', inputString, MAX_INPUT_LENGTH);
      // Add '\0' to convert in C String style
      inputString[input] = '\0';

      if (input > 0) {
        //Serial.println("Received: " + String(inputString));
        
       
        if (strcmp(inputString, "GET_GPS") == 0) {
          // Get NMEA 
          parseGpsInfo(send_AT_CMD("AT+CGNSINF", 1000, DEBUG));

          // Clear the string input buffer 
          memset(inputString, 0, sizeof(inputString)); 
        }
      }
    }
    // Clear the string input buffer 
    memset(inputString, 0, sizeof(inputString));
  #endif
}

 
String send_AT_CMD(String command, const int timeout, boolean debug) {
  String response = "";
  
  // Send AT command to SIM808
  mySerial.println(command);
  
  // Save current time 
  long int time = millis();
  
  // Wait for response
  while ((time + timeout) > millis()) {
	
	// Get SIM808 response
	while (mySerial.available()) {
      char c = mySerial.read();
      response += c;
    }
  }
  
  if (debug) {
    Serial.print(response); 
  }
  
  return response;
}

void parseGpsInfo(String gpsInfo) {
  // Example format: +CGNSINF: 1,1,20240106205522.000,53.026797,6.018455,87.900,0.35,138.8,1,,0.9,1.2,0.8,,12,9,,,27,
  
  // Parse only if GPS run and fix status are OK 
  if (gpsInfo.length() > 90) {
    
    // Find the first comma
    int commaIndex1 = gpsInfo.indexOf(',');
    // Second comma
    int commaIndex2 = gpsInfo.indexOf(',', commaIndex1 + 1);
    // Third comma
    int commaIndex3 = gpsInfo.indexOf(',', commaIndex2 + 1);
    // Fourth comma
    int commaIndex4 = gpsInfo.indexOf(',', commaIndex3 + 1);

    String fixStatus = gpsInfo.substring(commaIndex1 + 1,  commaIndex2);

    if (fixStatus.equals("1")) {
    
      // Extract latitude and longitude
      String gpsCoordinates = gpsInfo.substring(commaIndex3 + 1, gpsInfo.indexOf(',', commaIndex4 + 1));

      // Return the GPS coordinates format : "GPS : 53.026797,6.018455"
      //Serial.print("GPS : ");
      Serial.println(gpsCoordinates);
    }
    
    else {
      Serial.println("NO GPS Fix, please wait..");
    }
  }
  else {
      Serial.println("GPS is not running, please wait..");
    }
}

bool checkSIM808powerState(void) {
  
  // Check if SIM808 is powered on
  String state = send_AT_CMD("AT+CGPSSTATUS?", 1000, DEBUG);
  
  // If we have a response, SIM808 is powered on
  if (state.length() > 0) {
    return true;
  }
  else {
    return false;
  }
} 

