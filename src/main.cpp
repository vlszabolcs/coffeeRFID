
#include <Wire.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <Adafruit_PN532.h>
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

void printByteArray(byte* array, int size) {
  for (int i = 0; i < size; i++) {
    Serial.print(array[i], HEX);
    if (i < size - 1) {
      Serial.print(" ");
    }
  }
  Serial.println();
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
  if (!SD.begin()) {
    Serial.println("SD failure!");
    while (1);
  }


  // reading CSV file and store mem
  if (readCSVFile(dbPath, dataArray, maxDataCount)) {
    Serial.println("csv file has been scanned. ");
    for (int i = 0; i < maxDataCount; i++) {
      Serial.print("ID: "); Serial.println(dataArray[i].id);
      Serial.print("Name: "); Serial.println(dataArray[i].name);
      Serial.print("Credit: "); Serial.println(dataArray[i].credit);
      Serial.print("RFID0: "); printByteArray(dataArray[i].rfid0, 4);
      Serial.print("RFID1: "); printByteArray(dataArray[i].rfid1, 4);
      Serial.print("RFID2: "); printByteArray(dataArray[i].rfid2, 4);
      Serial.println("----------------------");
    }

  } else {
    Serial.println("Error reading the CSV file!");
  }

  logFile = SD.open("coffee_log.txt", FILE_WRITE);
  if (logFile) {
    logFile.println("Indítás...");
    logFile.close();
  } else {
    Serial.println("Nem sikerült megnyitni a log fájlt.");
  }
}


void log(String logEntry) {
  File logFile = SD.open(logPath, FILE_WRITE);
  if (logFile) {
    logFile.println(logEntry);
    logFile.close();
  } else {
    Serial.println("Nem sikerült megnyitni a log fájlt.");
  }
}


int findPersonAndCheckCredit(Data* dataArray, int dataSize, byte* nfcData, int amount) {
  for (int i = 0; i < dataSize; i++) {
    if (memcmp(dataArray[i].rfid0, nfcData, sizeof(dataArray[i].rfid0)) == 0 ||
        memcmp(dataArray[i].rfid1, nfcData, sizeof(dataArray[i].rfid1)) == 0 ||
        memcmp(dataArray[i].rfid2, nfcData, sizeof(dataArray[i].rfid2)) == 0) {
        
      if (dataArray[i].credit >= amount) { // Credit check and deduction
        dataArray[i].credit -= amount;
        String logPurchase = String(dataArray[i].name)+","+
                     String(dataArray[i].rfid0, HEX)+","+
                     String(dataArray[i].credit) ;
        Serial.println(logPurchase);
        log(logPurchase);
        return i; // Sikeresen levonás, visszaadjuk a személy indexét

      } else {
        return -1; //has not enough credit
      }
    }
  }
  return -1; // Az ID nem található az adattömbben
}

void loop(void) {
  
  timeClient.update();
  
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength; 
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
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