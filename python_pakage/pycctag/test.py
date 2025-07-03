import cv2
import sys
sys.path.append("./CCTag-for-python-usage/python_pakage/pycctag")
import pycctag

markervector = pycctag.detect_from_file("./CCTag-for-python-usage/sample/01.png")
markerlist = list(markervector)
for i in range(len(markerlist)):
    print(str(markerlist[i].id)+"   "+str(markerlist[i].status))
    print("x: " + str(markerlist[i].x)+"   y: "+str(markerlist[i].y))