/*hardware:
ESP32 Dev kit C
NFC module V3
RTC module
SD card module 
Piezzo buzzer
*/

#include <Wire.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <Adafruit_PN532.h>

#include <HTTPClient.h>

#include <csv.cpp>
#include <secret.h>

#define PN532_IRQ   (2)
#define PN532_RESET (3)

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* ssid = wifiSsid;
const char* password = wifiPwd;
const int maxDataCount = 16; //dinamikussá tenni! 

int coffeePrice=100;

Data dataArray[maxDataCount];
File logFile;

String printByteArray(byte* array, int size) {
  String byteStr="";
  for (int i = 0; i < size; i++) {
    byteStr+=String(array[i], HEX);
    if (i < size - 1) {
    }
  }
  return byteStr;
}

void succesPurchase(uint8_t* nfcData, String name, int credit) {
  
  logFile = SD.open(logPath, FILE_APPEND);
  if (logFile) {
    logFile.print(timeClient.getEpochTime());
    logFile.print(",");
    logFile.print(name);
    logFile.print(",");
    logFile.print(printByteArray(nfcData,4));
    logFile.print(",");
    logFile.print(credit);
    logFile.print(",");
    logFile.println("success");
    logFile.close();
  } else {
    Serial.println("logFile could not open");
  }
}

void noCredit(uint8_t* nfcData, String name, int credit) {
  
  logFile = SD.open(logPath, FILE_APPEND);
  if (logFile) {
    logFile.print(timeClient.getEpochTime());
    logFile.print(",");
    logFile.print(name);
    logFile.print(",");
    logFile.print(printByteArray(nfcData,4));
    logFile.print(",");
    logFile.print(credit);
    logFile.print(",");
    logFile.println("not enough credit");
    logFile.close();
  } else {
    Serial.println("logFile could not open");
  }
}

int findPersonAndCheckCredit(Data* dataArray, int dataSize, byte* nfcData, int amount) {
  for (int i = 0; i < dataSize; i++) {
    if (memcmp(dataArray[i].rfid0, nfcData, sizeof(dataArray[i].rfid0)) == 0 ||
        memcmp(dataArray[i].rfid1, nfcData, sizeof(dataArray[i].rfid1)) == 0 ||
        memcmp(dataArray[i].rfid2, nfcData, sizeof(dataArray[i].rfid2)) == 0) {
        
      if (dataArray[i].credit >= amount) { // Credit check and deduction
        dataArray[i].credit -= amount;
        succesPurchase(dataArray[i].rfid0,dataArray[i].name, dataArray[i].credit);
        return i;

      } else {
        noCredit(dataArray[i].rfid0,dataArray[i].name, dataArray[i].credit);
        return -1; //has not enough credit
      }
    }
  }
  return -1;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.print("Initializing SD card...");
  
  timeClient.begin();
  
  nfc.begin();

  // see if the card is present and can be initialized:
  if (!SD.begin()) {
    Serial.println("Card failed, or not present");
    
    // don't do anything more:
    while (1);
  }
 
  // reading CSV file and store mem
  if (readCSVFile(dbPath, dataArray, maxDataCount)) {
    Serial.println("csv file has been scanned. ");
    for (int i = 0; i < maxDataCount; i++) {
      Serial.print("ID: "); Serial.println(dataArray[i].id);
      Serial.print("Name: "); Serial.println(dataArray[i].name);
      Serial.print("Credit: "); Serial.println(dataArray[i].credit);
      Serial.print("RFID0: "); Serial.println(printByteArray(dataArray[i].rfid0, 4));
      Serial.print("RFID1: "); Serial.println(printByteArray(dataArray[i].rfid1, 4));
      Serial.print("RFID2: "); Serial.println(printByteArray(dataArray[i].rfid2, 4));
      Serial.println("----------------------");
    }

  } else {
    Serial.println("Error reading the CSV file!");
  }

  timeClient.update();
  logFile = SD.open(logPath, FILE_APPEND);
  logFile.print("system started at: ");
  logFile.println(timeClient.getEpochTime());
  logFile.close();

  postCSV();
}

void loop(void) {
  
  timeClient.update();
  
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength; 
  
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
    if (success) {
    Serial.println("RFID card detected!");

    // Ha van ilyen ID az adattömbben
    int personIndex = findPersonAndCheckCredit(dataArray, maxDataCount, uid, coffeePrice);

    if (personIndex != -1) {
      Serial.println("Az NFC kartya rendelkezik jogosultsaggal.");
      
      if (personIndex >= 0) {
        Serial.println("Kave keszitese...");
        Serial.println("Kave kesz.");
      } else {
        Serial.println("Nincs elegendo kredit. Toltsd fel a kreditet!");
      }

    } else {
      Serial.println("The NFC card is not authorized.");
    }

    delay(1000); // ide baszni kell egy nonblocking delayt 
  }

}