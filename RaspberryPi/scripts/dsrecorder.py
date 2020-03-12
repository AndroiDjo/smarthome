import pyaudio
import wave
import os, sys
import getopt
import time
import argparse

__location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))
p = pyaudio.PyAudio()
chunk = 1024  # Record in chunks of 1024 samples
sample_format = pyaudio.paInt16  # 16 bits per sample
channels = 1
fs = 16000  # Record at 44100 samples per second
pathPrefix = "/media/djo/DS/work/clips/"
vocabulary = "vocabulary.txt"
secpersym = 0.13

def fileContainText(filename, text):
    existText = False
    try:
        with open(filename, "r") as f:
            for line in f:
                if line.strip() == text:
                    existText = True
                    break
    except FileNotFoundError:
        existText = False
    return existText

def getNextWavName():
    maxnum = 0
    for file in os.listdir(__location__):
        if file.endswith(".wav") and file.startswith("rec."):
            filenum = int(file[4:-4])
            if filenum > maxnum:
                maxnum = filenum
    return "rec."+str(maxnum + 1)+".wav"

def playwav(frames):
    stream = p.open(format=sample_format,
                    channels=channels,
                    rate=fs,
                    frames_per_buffer=chunk,
                    output=True)
    for fr in frames:
        stream.write(fr)
    stream.stop_stream()
    stream.close()

def record(reclen, delay, text, outputcsv, needconfirm, singlesentence):
    textLen = len(text)

    if textLen == 0:
        raise ValueError('Text cannot be null')

    if singlesentence:
        if fileContainText(vocabulary, text):
            print(vocabulary+" already contains: "+text)
            return

    recLength = reclen
    if recLength == 0:
        recLength = textLen * secpersym
    if recLength < 2:
        recLength = 2

    print (text)
    if needconfirm:
        dummy = input("Ready?")
    if delay > 0:
        print("Sleep "+str(delay)+" sec")
        time.sleep(delay)

    filename = getNextWavName()
    print("Recording "+str(recLength)+" sec...")
    stream = p.open(format=sample_format,
                    channels=channels,
                    rate=fs,
                    frames_per_buffer=chunk,
                    input=True)
    frames = []  # Initialize array to store frames
    # Store data in chunks for 3 seconds
    for i in range(0, int(fs / chunk * recLength)):
        data = stream.read(chunk)
        frames.append(data)

    subframes = frames[5:] #default cut
    # Stop and close the stream 
    stream.stop_stream()
    stream.close()

    print('Finished recording')

    saveWav = False
    if needconfirm:
        dialogue = True
        while dialogue:
            inp = input("s - save, p - playback, r - dont save and repeat record, "+
                        "b - break record and exit script, cn:n - cut wav (first digit - from begin, second - from end), "+
                        "else - skip record: ")
            if inp == "s":
                saveWav = True
                dialogue = False
            elif inp == "r":
                record(reclen=recLength,
                       delay=delay,
                       text=text,
                       outputcsv=outputcsv,
                       needconfirm=needconfirm,
                       singlesentence=singlesentence)
                dialogue = False
            elif inp == "p":
                playwav(subframes)
            elif inp == "b":
                p.terminate()
                sys.exit()
            elif inp.startswith("c"):
                try:
                    frlen = len(frames)
                    cutarr = inp[1:].split(":")
                    if cutarr[0] == "":
                        slfrom = None
                    else:
                        slfrom = int(cutarr[0])

                    if cutarr[1] == "":
                        slto = None
                    else:
                        slto = int(cutarr[1])

                    subframes = frames[slfrom:slto]
                    if len(subframes) > 1:
                        playwav(subframes)
                    else:
                        print("you must cut less than length of wav: "+str(frlen))
                except ValueError:
                    print("wrong number format! example: c10:5")
            else:
                dialogue = False

    if (needconfirm and saveWav) or not needconfirm:
        # Save the recorded data as a WAV file
        wf = wave.open(filename, 'wb')
        wf.setnchannels(channels)
        wf.setsampwidth(p.get_sample_size(sample_format))
        wf.setframerate(fs)
        wf.writeframes(b''.join(subframes))
        wf.close()

        with open(outputcsv, "a") as out:
            out.write(pathPrefix+filename+","+str(os.path.getsize(filename))+","+text+"\n")
        if not fileContainText(vocabulary, text):
            with open(vocabulary, "a") as f:
                f.write(text+"\n")
        print(filename+" written to "+outputcsv)

def main():
    argparser = argparse.ArgumentParser()
    argparser.add_argument('-l','--len',help='Length of record (sec), by default calculates from length of text',type=float,default=0)
    argparser.add_argument('-d','--delay',help='Delay before recording (default 0 sec)',type=int,default=0)
    argparser.add_argument('-t','--text',help='Recorded text')
    argparser.add_argument('-o','--output',help='Output file, csv',default='train.csv')
    argparser.add_argument('-s','--stat',help='Print statistics and exit',action='store_true')
    argparser.add_argument('-m','--mode',help='Record mode: normal - enter text and record it; file - get text from csv file line by line (first column)',default='normal')
    argparser.add_argument('-i','--input',help='Input text file, csv')
    argparser.add_argument('-c','--confirm',help='Confirm before save record',action='store_true')
    argparser.add_argument('-S','--single',help='single sentence mode (skip duplicate sentences)',action='store_true')
    args = argparser.parse_args()

    if args.stat:
        num_lines = sum(1 for line in open("train.csv"))
        print("train.csv - "+str(num_lines))
        num_lines = sum(1 for line in open("dev.csv"))
        print("dev.csv - "+str(num_lines))
        num_lines = sum(1 for line in open("test.csv"))
        print("test.csv - "+str(num_lines))
        num_lines = sum(1 for line in open("vocabulary.txt"))
        print("vocabulary.txt - "+str(num_lines))
        sys.exit()

    if args.mode == 'normal':
        record(reclen=args.len,
               delay=args.delay,
               text=args.text.lower(),
               outputcsv=args.output,
               needconfirm=args.confirm,
               singlesentence=args.single)
    elif args.mode == 'file':
        if args.input is None:
            raise ValueError('You need specify input file for this mode!')
        with open(args.input, "r") as f:
            for line in f:
                record(reclen=args.len,
                       delay=args.delay,
                       text=line.strip(),
                       outputcsv=args.output,
                       needconfirm=True,
                       singlesentence=args.single)

    p.terminate()
    
if __name__ == "__main__":
   main()