import cv2
import sys
sys.path.append("/mnt/hd1/cxh/liwen/CCTag-develop/python_pakage/pycctag")
import pycctag

markervector = pycctag.detect_from_file("/mnt/hd1/cxh/liwen/CCTag-develop/sample/01.png")
image = cv2.imread("/mnt/hd1/cxh/liwen/CCTag-develop/sample/01.png")
markerlist = list(markervector)
for i in range(len(markerlist)):
    print(str(markerlist[i].id)+"   "+str(markerlist[i].status))
    print(markerlist[i].x, markerlist[i].y)
    pointx = int(markerlist[i].x)
    pointy = int(markerlist[i].y)
    cv2.circle(image, (pointx, pointy), radius=10, color=(0, 255, 0), thickness=-1)
cv2.imwrite("/mnt/hd1/cxh/liwen/CCTag-develop/sample/01_result.png", image)
