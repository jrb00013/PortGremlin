import random

def random_vid_pid():
    vid = hex(random.randint(0x1000, 0xFFFF))
    pid = hex(random.randint(0x1000, 0xFFFF))
    print(f"Random VID: {vid}, PID: {pid}")
