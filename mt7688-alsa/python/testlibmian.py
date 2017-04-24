from ctypes import *
import os
import time

libsc = cdll.LoadLibrary(os.getcwd() + '/libmain.so')
#print libsc.get_button()
print libsc.thread_rec()
time.sleep(1);
libsc.clear_display()