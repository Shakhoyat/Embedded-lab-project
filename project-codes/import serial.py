import serial


ser = serial.Serial('COM6', 9600, timeout=1)

char_to_send = 'A'  

ser.write(char_to_send.encode())

ser.close()