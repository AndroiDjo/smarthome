import time
from threading import Thread
from tkinter import *
from tkinter import ttk
from tkinter import font

class SmartClock(Thread):

    def __init__(self, bg, fg, fsize):
        Thread.__init__(self)
        self.bg = bg
        self.fg = fg
        self.fsize = fsize

    def run(self):
        self.root = Tk()
        self.root.attributes("-fullscreen", True)
        self.root.configure(background=self.bg)
        self.root.after(1000, self.show_time)
        fnt = font.Font(family='Helvetica', size=self.fsize, weight='bold')
        self.timetxt = StringVar()
        self.timetxt.set(time.strftime("%H:%M:%S"))
        lbl = ttk.Label(self.root, textvariable=self.timetxt, font=fnt, foreground=self.fg, background=self.bg)
        lbl.place(relx=0.5, rely=0.5, anchor=CENTER)
        self.root.mainloop()

    def show_time(self):
        self.timetxt.set(time.strftime("%H:%M:%S"))
        self.root.after(1000, self.show_time)

    def timeStop(self):
        self.root.destroy()