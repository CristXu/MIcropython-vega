import cv2 as cv
import numpy as np
import struct
a = ['off.jpg','r_on.jpg','g_on.jpg','b_on.jpg']

for path in a:
    data = open("%s.bin"%path.split(".")[0],'wb')
    img = cv.imread(a[2])
    size = img.shape;   
    img = np.reshape(img, (size[0]*size[1]*size[2]))
    for i in  range(len(img)//3):
        b = img[i*3]
        g = img[i*3+1]
        r = img[i*3+2]
        rgb16 = ((((r & 0xf8) >> 3)<< 11) |(((g & 0xfc) >> 2) << 5) | ((b & 0xf8) >> 3)) & 0xffff; 
        inverse_rgb16 = (((rgb16 & 0xff) << 8) | ((rgb16 & 0xff00) >> 8)) & 0xffff; 
        data.write(struct.pack("H",inverse_rgb16)) 
    data.close()


