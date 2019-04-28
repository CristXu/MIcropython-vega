import cv2 as cv
import numpy as np
a = ['big.jpg','medium.jpg','small.jpg']

data_c = open("icon.c",'w')
data_h = open("icon.h",'w')
cStr = '#include "icon.h" \n'
hStr = '#include "stdint.h"\n'
for path in a:
    img = cv.imread(path)
    cStr += 'const uint16_t %s[] = {\n\t'%path.split(".")[0]
    hStr += 'extern const uint16_t %s[];\n'%path.split(".")[0]
    size = img.shape;   
    img = np.reshape(img, (size[0]*size[1]*size[2]))
    for i in  range(len(img)//3):
        b = img[i*3]
        g = img[i*3+1]
        r = img[i*3+2]
        rgb16 = ((((r & 0xf8) >> 3)<< 11) |(((g & 0xfc) >> 2) << 5) | ((b & 0xf8) >> 3)) & 0xffff; 
        inverse_rgb16 = (((rgb16 & 0xff) << 8) | ((rgb16 & 0xff00) >> 8)) & 0xffff; 
        cStr += "%d ,"%inverse_rgb16; 
        if (i+1)%16 is 0:
            cStr += '\n\t'
    cStr += '}; \n'   
    cStr += 'const uint16_t %s_w = %d;\n'%(path.split(".")[0],size[1]);
    cStr += 'const uint16_t %s_h = %d;\n'%(path.split(".")[0],size[0]);
    hStr += 'extern const uint16_t %s_w;\n'%path.split(".")[0]
    hStr += 'extern const uint16_t %s_h;\n'%path.split(".")[0]
data_c.write(cStr)
data_h.write(hStr)
data_c.close();
data_h.close();
