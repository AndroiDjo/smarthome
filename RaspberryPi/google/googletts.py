from pydub import AudioSegment
from pydub.playback import play
from google.cloud import texttospeech
import io, os

__location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))
os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = os.path.join(__location__, 'svcacc.json')

client = texttospeech.TextToSpeechClient()
voice = texttospeech.types.VoiceSelectionParams(
    language_code='ru-RU',
    name='ru-RU-Wavenet-C')
audio_config = texttospeech.types.AudioConfig(
    audio_encoding=texttospeech.enums.AudioEncoding.MP3)

def speech(text):
    synthesis_input = texttospeech.types.SynthesisInput(text=text)
    response = client.synthesize_speech(synthesis_input, voice, audio_config)
    voice_resp = AudioSegment.from_file(io.BytesIO(response.audio_content), format="mp3")
    play(voice_resp)