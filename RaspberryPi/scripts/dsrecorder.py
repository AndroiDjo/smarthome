import pyaudio
import wave
import os, sys
import getopt
import time
import argparse

def main():
    __location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))
    maxnum = 0
    chunk = 1024  # Record in chunks of 1024 samples
    sample_format = pyaudio.paInt16  # 16 bits per sample
    channels = 1
    fs = 16000  # Record at 44100 samples per second

    argparser = argparse.ArgumentParser()
    argparser.add_argument('-l','--len',help='Length of record (default 2 sec)',type=float,default=2)
    argparser.add_argument('-d','--delay',help='Delay before recording (default 0 sec)',type=int,default=0)
    argparser.add_argument('-t','--text',help='Recorded text',required=True)
    argparser.add_argument('-o','--output',help='Output file',default='train.csv')
    argparser.add_argument('-s','--stat',help='Print statistics and exit',action='store_true')
    args = argparser.parse_args()

    if args.stat:
        num_lines = sum(1 for line in open("train.csv"))
        print("train.csv - "+str(num_lines))
        num_lines = sum(1 for line in open("dev.csv"))
        print("dev.csv - "+str(num_lines))
        num_lines = sum(1 for line in open("test.csv"))
        print("test.csv - "+str(num_lines))
        sys.exit()

    recordLength = args.len
    recordDelay = args.delay
    recordText = args.text.lower()
    outputFile = args.output

    for file in os.listdir(__location__):
        if file.endswith(".wav") and file.startswith("rec."):
            filenum = int(file[4:-4])
            if filenum > maxnum:
                maxnum = filenum
    
    maxnum += 1

    if recordDelay > 0:
        print("Sleep "+str(recordDelay)+" sec")
        time.sleep(recordDelay)

    filename = "rec."+str(maxnum)+".wav"
    p = pyaudio.PyAudio()  # Create an interface to PortAudio
    print('Recording')
    print(recordText)

    stream = p.open(format=sample_format,
                    channels=channels,
                    rate=fs,
                    frames_per_buffer=chunk,
                    input=True)

    frames = []  # Initialize array to store frames

    # Store data in chunks for 3 seconds
    for i in range(0, int(fs / chunk * recordLength)):
        data = stream.read(chunk)
        frames.append(data)

    # Stop and close the stream 
    stream.stop_stream()
    stream.close()
    # Terminate the PortAudio interface
    p.terminate()

    print('Finished recording')

    # Save the recorded data as a WAV file
    wf = wave.open(filename, 'wb')
    wf.setnchannels(channels)
    wf.setsampwidth(p.get_sample_size(sample_format))
    wf.setframerate(fs)
    wf.writeframes(b''.join(frames))
    wf.close()

    with open(outputFile, "a") as out:
        out.write("/media/djo/DS/work/clips/"+filename+","+str(os.path.getsize(filename))+","+recordText+"\n")

    existText = False
    with open("vocabulary.txt", "r") as f:
        for line in f:
            if line.strip() == recordText:
                existText = True
                break

    if not existText:
        with open("vocabulary.txt", "a") as f:
            f.write(recordText+"\n")

    print(filename+" written to "+outputFile)
    
if __name__ == "__main__":
   main()