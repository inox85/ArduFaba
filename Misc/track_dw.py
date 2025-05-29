from pytube import YouTube
from pydub import AudioSegment
import os

def download_and_normalize_youtube_audio(url, output_folder="output"):
    # Crea la cartella di output se non esiste
    os.makedirs(output_folder, exist_ok=True)

    print("Scaricamento in corso...")
    yt = YouTube(url)
    audio_stream = yt.streams.filter(only_audio=True).first()
    audio_path = audio_stream.download(output_path=output_folder, filename="audio.mp4")

    # Percorso per l'audio MP3 normalizzato
    mp3_output = os.path.join(output_folder, "audio_normalizzato.mp3")

    print("Conversione e normalizzazione in corso...")
    # Carica l'audio
    audio = AudioSegment.from_file(audio_path, format="mp4")
    
    # Normalizzazione (a -1 dBFS)
    normalized_audio = match_target_amplitude(audio, -1.0)

    # Esporta in formato MP3 (richiede ffmpeg installato)
    normalized_audio.export(mp3_output, format="mp3", bitrate="320k")

    print(f"Audio scaricato e normalizzato: {mp3_output}")

def match_target_amplitude(sound, target_dBFS):
    """Normalizza l'audio al livello target (in dBFS)"""
    change_in_dBFS = target_dBFS - sound.dBFS
    return sound.apply_gain(change_in_dBFS)

# ESEMPIO DI UTILIZZO
if __name__ == "__main__":
    link = input("Inserisci il link YouTube: ")
    download_and_normalize_youtube_audio(link)
