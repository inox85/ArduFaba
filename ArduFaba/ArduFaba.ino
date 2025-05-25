#include <SPI.h>
#include <MFRC522.h>
#include "SerialMP3Player.h"

// Pin MFRC522
#define RST_PIN  9
#define SS_PIN   10

// Pin MP3 Player YX5300 (TX modulo → pin 4, RX modulo → pin 5)
#define MP3_TX 4
#define MP3_RX 5

MFRC522 mfrc522(SS_PIN, RST_PIN);
SerialMP3Player mp3(MP3_RX, MP3_TX);

// UID noti
byte tag1_UID[4] = {0xE3, 0xDA, 0x68, 0x3}; // Brano 1
byte tag2_UID[4] = {0x12, 0x34, 0x56, 0x78}; // Brano 2

bool isPlaying = false;
int currentTrack = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Inizializzazione MP3 Player...");
  mp3.showDebug(1);
  mp3.begin(9600);
  delay(500);
  mp3.sendCommand(CMD_SEL_DEV, 0, 2); // seleziona SD card
  delay(500);

  mp3.setVol(15); // Volume 50%

  Serial.println("Sistema pronto. Avvicina un tag RFID.");
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
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("UID letto: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    int trackToPlay = getTrackFromUID(mfrc522.uid.uidByte);

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
