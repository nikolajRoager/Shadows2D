import numpy as np
import matplotlib.pyplot as plt

import sys

if (len(sys.argv)!=2):
    print("Whaaat")
else:
    im = plt.imread(sys.argv[1])
    (h,w,channels)= im.shape
    print(w)
    print(h)
    for y in range(0,h):
        for x in range(0,w):
            if (im[h-y-1][x][0]<0.5):
                print('0', end =" ")
            else:
                print('1', end =" ")
        print()
