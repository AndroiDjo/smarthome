import cv2
import time
from threading import Thread

class SmartEye(Thread):

    def __init__(self, stream_url, res_w, res_h):
        Thread.__init__(self)
        self.res_w = res_w
        self.res_h = res_h
        self.cap = cv2.VideoCapture(stream_url)
        self.loop_active = True
        cv2.namedWindow("smarteye", cv2.WND_PROP_FULLSCREEN)
        cv2.setWindowProperty("smarteye",cv2.WND_PROP_FULLSCREEN,cv2.WINDOW_FULLSCREEN)
    
    def run(self):
        print('run')
        while(self.loop_active):
            ret, frame = self.cap.read()
            frame = cv2.resize(frame, (self.res_w, self.res_h)) 
            cv2.imshow("smarteye", frame)
            k = cv2.waitKey(30) & 0xff
            if k == 27: # press 'ESC' to quit
                self.videoStop()
                break

    def videoStop(self):
        print('videoStop')
        self.loop_active = False
        self.cap.release()
        cv2.destroyAllWindows()