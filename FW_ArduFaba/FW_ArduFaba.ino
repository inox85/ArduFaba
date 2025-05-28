#include <SPI.h>
#include <MFRC522.h>
#include "SerialMP3Player.h"
#include "Button2.h"
#include "EEPROM.h"

// Pin MFRC522
#define RST_PIN  9
#define SS_PIN   10

// Pin MP3 Player YX5300 (TX modulo → pin 4, RX modulo → pin 5)
#define MP3_TX 4
#define MP3_RX 5

const int MAX_TAGS = 100;
const int TAG_SIZE = 4;
const int COUNT_ADDR = 0;
const int DATA_START_ADDR = 1;

MFRC522 mfrc522(SS_PIN, RST_PIN);
SerialMP3Player mp3(MP3_RX, MP3_TX);

// UID noti
byte tag1_UID[4] = {0xE3, 0xDA, 0x68, 0x3}; // Brano 1
byte tag2_UID[4] = {0x12, 0x34, 0x56, 0x78}; // Brano 2

bool isPlaying = false;
int currentTrack = 0;



void setup() {
  Serial.begin(9600);


  // Serial.println("Cancellazione EEPROM...");

  // for (int i = 0; i < EEPROM.length(); i++) {
  //   EEPROM.write(i, 0xFF);  // oppure 0x00 se preferisci
  // }

  // Serial.println("EEPROM cancellata.");


  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Inizializzazione MP3 Player...");
  mp3.showDebug(1);
  mp3.begin(9600);
  delay(500);
  mp3.sendCommand(CMD_SEL_DEV, 0, 2); // seleziona SD card
  delay(500);

  mp3.setVol(30); // Volume 50%

  Serial.println("Sistema pronto. Avvicina un tag RFID.");

  playFileInFolder(1, 1);
}

void loop() {
  // Controllo risposte MP3 (fine traccia)
  if (mp3.available()) {
    String answer = mp3.decodeMP3Answer();
    Serial.println("MP3 risponde: " + answer);

    if (answer.indexOf("track finished") >= 0) {
      isPlaying = false;

      // Dopo fine brano → verifica se il tag è ancora lì
      if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        Serial.print("UID presente: ");
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          Serial.print(mfrc522.uid.uidByte[i], HEX);
          Serial.print(" ");
        }
        Serial.println();

        int detectedTrack = getTrackFromUID(mfrc522.uid.uidByte);
        if (detectedTrack == currentTrack) {
          Serial.println("Stesso tag ancora presente → Riparte il brano.");
          mp3.setVol(30);
          mp3.play(currentTrack);
          delay(5000);
          isPlaying = true;
        }
      }
    }

  }

  // Normale lettura tag RFID per cambiare brano
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) 
  {
    byte uid[4];
    memcpy(uid, mfrc522.uid.uidByte, 4);

    Serial.print("UID letto: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    int id = trovaOScriviUIDinEEPROM(uid);

    mfrc522.PICC_HaltA();

    int trackToPlay = id;

    if (trackToPlay > 0) {
      Serial.print("Tag riconosciuto → Play brano ");
      Serial.println(trackToPlay);
      mp3.play(trackToPlay);
      isPlaying = true;
      currentTrack = trackToPlay;
    } else {
      Serial.println("Tag non riconosciuto → nessuna azione.");
    }

  }

}

uint8_t trovaOScriviUIDinEEPROM(const byte* uid) {
  int primoSlotLibero = -1;

  for (uint8_t blocco = 0; blocco < 100; blocco++) {
    int addr = blocco * 4;
    bool match = true;
    bool slotVuoto = true;

    // Verifica se i 4 byte del blocco coincidono con l'UID
    for (uint8_t i = 0; i < 4; i++) {
      byte val = EEPROM.read(addr + i);

      if (val != uid[i]) {
        match = false;
      }

      if (val != 0xFF) {
        slotVuoto = false;
      }
    }

    if (match) {
      return blocco + 1; // UID già presente
    }

    // Segna il primo slot vuoto disponibile
    if (slotVuoto && primoSlotLibero == -1) {
      primoSlotLibero = blocco;
    }
  }

  // Se UID non trovato ma c'è uno slot libero, scrivilo lì
  if (primoSlotLibero != -1) {
    int addr = primoSlotLibero * 4;
    for (uint8_t i = 0; i < 4; i++) {
      EEPROM.write(addr + i, uid[i]);
    }
    return primoSlotLibero + 1;
  }

  // EEPROM piena
  return 0;
}


// Funzione: restituisce il numero brano in base all'UID letto
int getTrackFromUID(byte *uid) {
  if (compareUID(uid, tag1_UID)) return 1;
  if (compareUID(uid, tag2_UID)) return 2;
  return 0;
}

// Confronta due UID (4 byte)
bool compareUID(byte *uid1, byte *uid2) {
  for (byte i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

void playFileInFolder(uint8_t folder, uint8_t file) {
  
  if (mp3.available()) 
  {
    mp3.sendCommand(0x0F, folder, file);
  }

}

