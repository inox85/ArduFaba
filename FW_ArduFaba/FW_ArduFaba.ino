#include <SPI.h>
#include <MFRC522.h>
#include "SerialMP3Player.h"
#include "Button2.h"
#include "EEPROM.h"
#include <SoftwareSerial.h>

#define GREEN_BUTTON_PIN 7
#define RED_BUTTON_PIN 8

// Pin MFRC522
#define RST_PIN  9
#define SS_PIN   10

// Pin MP3 Player YX5300 (TX modulo → pin 4, RX modulo → pin 5)
#define MP3_TX 4
#define MP3_RX 5

#define MISC_FOLDER 1

#define POWER_UP_TRACK 1
#define RECOGNIZED_CHARACTER_TRACK 2
#define BATTERY_ALERT_TRACK 3
#define NEW_CHARACTER_TRACK 4
#define DELETE_MEMORY_TRACK 5
#define MEMORY_ERROR_TRACK 6

#define DEBOUNCE_TIME 50
#define DOUBLE_CLICK_TIME 400
#define LONG_PRESS_TIME 1000

const int MAX_TAGS = 100;
const int TAG_SIZE = 4;
const int COUNT_ADDR = 0;
const int DATA_START_ADDR = 1;

MFRC522 mfrc522(SS_PIN, RST_PIN);
SoftwareSerial mp3Serial(MP3_RX, MP3_TX);  // RX, TX del modulo MP3
SerialMP3Player mp3(MP3_RX, MP3_TX);

bool isPlaying = false;
int currentTrack = 0;

Button2 redButton;
Button2 greenButton;

void redButtonHandler(Button2& btn) {
    switch (btn.getType()) {
        case single_click:
            break;
        case double_click:
            playNextTrack();
            break;
        case triple_click:
            Serial.print("triple ");
            break;
        case long_click:
            Serial.print("long ");
            break;
          case empty:
            return;
    }

}


void greenButtonHandler(Button2& btn) {
    switch (btn.getType()) {
        case single_click:
            playPause();
            break;
        case double_click:
            playPreviousTrack();
            break;
        case triple_click:
            Serial.print("triple ");
            break;
        case long_click:
            Serial.print("long");
            break;
          case empty:
            return;
    }

}


void playPause()
{

  if(isPlaying)
  { 
    isPlaying = false;
    Serial.println("Selezione manuale → Pause ");
    mp3.pause(); 
  }
  else
  {
    if(currentTrack == 0)
    {
      currentTrack++;
      mp3.play(currentTrack);
    }
    else
    {
      isPlaying = true;
      Serial.println("Selezione manuale → Play ");
      mp3.play();
    }
  }

}


void playNextTrack()
{
    if(currentTrack < 100)
    {
      currentTrack++;
      Serial.print("Selezione manuale → Play brano ");
      Serial.println(currentTrack);
      mp3.play(currentTrack);
    }
}


void playPreviousTrack()
{
    if(currentTrack >= 1)
    {
      currentTrack--;
      Serial.print("Selezione manuale → Play brano ");
      Serial.println(currentTrack);
      mp3.play(currentTrack);
    }
}

void setup() 
{
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Inizializzazione MP3 Player...");
  mp3.showDebug(1);
  mp3.begin(9600);
  delay(500);
  mp3.sendCommand(CMD_SEL_DEV, 0, 2); // seleziona SD card
  delay(500);

  mp3.setVol(30); // Volume 50%

  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREEN_BUTTON_PIN, INPUT_PULLUP);

  if(!digitalRead(RED_BUTTON_PIN) && !digitalRead(GREEN_BUTTON_PIN))
  {
    playFileInFolder(MISC_FOLDER, DELETE_MEMORY_TRACK, 8000);
    while(true)
    {
        if(!digitalRead(GREEN_BUTTON_PIN))
        {
          eraseEEPROM();
          break;
        }
        if(!digitalRead(RED_BUTTON_PIN))
        {
          break;
        }
    }
  }

  Serial.println("Inizializzazione Bottoni...");
  // Definiamo i gestori degli eventi
  redButton.begin(RED_BUTTON_PIN);
  redButton.setClickHandler(redButtonHandler);
  //button.setLongClickHandler(handler);       // this will only be called upon release
  redButton.setLongClickDetectedHandler(redButtonHandler);  // this will only be called upon detection
  redButton.setDoubleClickHandler(redButtonHandler);
  redButton.setTripleClickHandler(redButtonHandler);

  greenButton.begin(GREEN_BUTTON_PIN);
  greenButton.setClickHandler(greenButtonHandler);
  //button.setLongClickHandler(handler);       // this will only be called upon release
  greenButton.setLongClickDetectedHandler(greenButtonHandler);  // this will only be called upon detection
  greenButton.setDoubleClickHandler(greenButtonHandler);
  greenButton.setTripleClickHandler(greenButtonHandler);

  
  playFileInFolder(MISC_FOLDER, POWER_UP_TRACK, 5000);

  delay(5000);

  Serial.println("Sistema pronto. Avvicina un tag RFID.");

}

void eraseEEPROM()
{
  Serial.println("Cancellazione EEPROM in corso...");

  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0); // scrive 0 in ogni cella
  }

  Serial.println("EEPROM cancellata.");
}

void loop() 
{

  redButton.loop();
  greenButton.loop();
  
  if(mp3.available())
  {
    // Normale lettura tag RFID per cambiare brano
    if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) 
    {

      byte uid[4];
      memcpy(uid, mfrc522.uid.uidByte, 4);

      Serial.print("UID letto: ");
      
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        Serial.print(" ");
      }
      
      Serial.println();

      int id = findTag(uid);
      
      if (id > 0) 
      {
        Serial.print("Tag riconosciuto, ID: ");
        Serial.println(id);
        playFileInFolder(MISC_FOLDER, RECOGNIZED_CHARACTER_TRACK, 5000);
      }
      else 
      {
      
        id = registerNewTag(uid);
        if (id > 0) 
        {
          Serial.print("Nuovo tag registrato, ID: ");
          Serial.println(id);
          playFileInFolder(MISC_FOLDER, NEW_CHARACTER_TRACK, 5000);
        } 
        else 
        {
          Serial.println("Memoria piena o errore");
          playFileInFolder(MISC_FOLDER, MEMORY_ERROR_TRACK, 5000);
        }
      
      }

      mfrc522.PICC_HaltA();

      if (id > 0) 
      {
        if(id != currentTrack)
        {       
          Serial.print("Tag riconosciuto → Play brano ");
          Serial.println(id);
          mp3.play(id);
          isPlaying = true;
          currentTrack = id;
        }
      }
      else
      {
        Serial.println("Tag non riconosciuto → nessuna azione.");
      }
    }
  }

}


void playFileInFolder(uint8_t folder, uint8_t file, uint16_t wait) 
{ 
  if (mp3.available()) 
  {
    mp3.sendCommand(0x0F, folder, file);
    Serial.print("Inizio messaggio -> ");
    Serial.print("Cartella: ");
    Serial.print(folder);
    Serial.print(" File: ");
    Serial.println(file);
    delay(wait);
    Serial.println("Fine messaggio");
  }
}




int findTag(byte* uid) {
  int count = EEPROM.read(COUNT_ADDR);
  for (int i = 0; i < count; i++) {
    int base = DATA_START_ADDR + i * TAG_SIZE;
    bool match = true;
    for (int j = 0; j < TAG_SIZE; j++) {
      if (EEPROM.read(base + j) != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return i + 1; // mappa da 1 a 100
  }
  return 0; // non trovato
}

int registerNewTag(byte* uid) {
  int count = EEPROM.read(COUNT_ADDR);
  if (count >= MAX_TAGS) return -1; // memoria piena

  int base = DATA_START_ADDR + count * TAG_SIZE;
  for (int j = 0; j < TAG_SIZE; j++) {
    EEPROM.write(base + j, uid[j]);
  }
  EEPROM.write(COUNT_ADDR, count + 1);
  return count + 1; // ID assegnato da 1 a 100
}

