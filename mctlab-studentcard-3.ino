#include <Wire.h>
#include <Adafruit_NFCShield_I2C.h>
#include <Ethernet.h>
#include <SPI.h>
#include <RestClient.h>
#include <ArduinoJson.h>

#define IRQ   (2)
#define RESET (3)
int ledR = 7;
int ledG = 6;
int doorContact = 5;

RestClient client = RestClient("medical-it.be");
Adafruit_NFCShield_I2C nfc(IRQ, RESET);

//global vars
String response;
String apiUrl = "/api/mctlab/cards/";

const char* studentName;
const char* cardNumber;
const char* studentBanned;

void setup(void) {
  //setup serial communications
  Serial.begin(115200);
  
  //manual ip config
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  byte ip[] = { 192, 168, 0, 111 };
  Ethernet.begin(mac,ip);
  
  //led & door contact config
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(doorContact, OUTPUT);

  //begin nfc
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Set the max number of retry attempts to read from a card
  nfc.setPassiveActivationRetries(0xFF);
  // configure board to read RFID tags
  nfc.SAMConfig();
    
  Serial.println("Waiting for a student card");
}

void loop(void) {
  //RFID
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  String scannedUid = "";
  
  // Wait for an ISO14443A type cards (Mifare, etc.).
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    for (uint8_t i=0; i < uidLength; i++) 
    {
      scannedUid += uid[i], HEX;
    }
    
    //convert string to char
    String fullUrl = apiUrl + scannedUid;
    char charBuf[fullUrl.length()];
    fullUrl.toCharArray(charBuf,fullUrl.length()+1);
    
    //get response
    response = "";
    int statusCode = client.get(charBuf, &response);
    
    //get response without hassle
    int firstBracket = response.indexOf('{');
    int lastBracket = response.lastIndexOf('}');
    response = response.substring(firstBracket, lastBracket+1);
    
    //parse json response
    StaticJsonBuffer<200> jsonBuffer;
    
    char jsonBuf[response.length()];
    response.toCharArray(jsonBuf,response.length()+1);
    
    JsonObject& root = jsonBuffer.parseObject(jsonBuf);

    if (!root.success()) {
      Serial.println("parseObject() failed");
    }
    else {
        studentName = root["studentName"];
        cardNumber = root["cardNumber"];
        studentBanned = root["studentBanned"];
        
        String strStudentName(studentName);
        String strCardNumber(cardNumber);
        String strStudentBanned(studentBanned);
        
        Serial.println(studentName);
        Serial.println(cardNumber);
        Serial.println(studentBanned);

        if(strStudentBanned == "0") {
          Serial.println("Welcome " + strStudentName +"!");
          Serial.println();
          
          digitalWrite(ledG, HIGH);
          digitalWrite(doorContact, HIGH);
          delay(3000);
          digitalWrite(ledG, LOW);
          digitalWrite(doorContact, LOW);
        }
        else {
          Serial.println("Sorry " + strStudentName + " , geen toegang.");
          
          for (int i=0; i < 8; i++){
            digitalWrite(ledR, HIGH);
            delay(100);
            digitalWrite(ledR, LOW);
            delay(100);
          } 
        }
    }
  }
  else
  {
    Serial.println("Timed out waiting for a card");
  }
}
