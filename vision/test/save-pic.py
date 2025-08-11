import cv2
import numpy as np

cap = cv2.VideoCapture(2)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 800)  # Largura em pixels
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 600)  # Altura em pixels

_,frame = cap.read()

cv2.imwrite('frame7.png', frame)