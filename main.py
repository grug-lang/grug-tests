# main.py

import ctypes
import os

lib = ctypes.CDLL("./libfuzz.so")

CALLBACK = ctypes.CFUNCTYPE(None, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t)

def run(data, size):
    s = ctypes.string_at(data, size)
    if s == b"ab":
        print("python: crash!")
        os.abort()

cb = CALLBACK(run)

lib.grug_fuzz.argtypes = [CALLBACK]
lib.grug_fuzz.restype = None

lib.grug_fuzz(cb)
