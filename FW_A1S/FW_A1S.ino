#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <SPI.h>
#include <SD.h>

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;

void setup() {
  Serial.begin(115200);

  // Monta SD
  if (!SD.begin(5)) {
    Serial.println("SD init failed!");
    while (true);
  }

  // Output I2S → DAC interno o esterno (ES8388 o simile)
  out = new AudioOutputI2S();
  out->SetPinout(27, 26, 25);  // BCLK, LRC, DOUT — adatta ai pin della tua scheda
  out->SetGain(0.5);

  // Carica file MP3 da SD
  file = new AudioFileSourceSD("/test.mp3");
  mp3 = new AudioGeneratorMP3();
  mp3->begin(file, out);
}

void loop() {
  if (mp3->isRunning()) {
    mp3->loop();
  } else {
    Serial.printf("done\n");
    delay(1000);
  }
}

