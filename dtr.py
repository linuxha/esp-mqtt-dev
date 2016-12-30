import sys, serial


from time import sleep


# Do we have any arguments?
if (len(sys.argv) == 1):
    print "Usage: dtr.py <commport>\r\ne.g. dtr.py COM5"
    exit()


socket = serial.Serial(sys.argv[1],
                       baudrate=115200,
                       bytesize=serial.EIGHTBITS,
                       parity=serial.PARITY_NONE,
                       stopbits=serial.STOPBITS_ONE,
                       timeout=1,
                       xonxoff=0,
                       rtscts=0
                       )

# Set DTR high initially - output is inverted
#socket.setDTR(False)
socket.setDTR(True)
sleep(1)
socket.setDTR(False)



socket.close()

print "\r\nReset issued"

#        socket.write('AT'+chr(13));
#        sleep(1)
#        print "Reading"
#        print socket.readlines()

