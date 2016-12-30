# Returns error codes:
# 1 = couldn't open input file
# 2 = couldn't open output file

import sys

# Do we have any arguments?
if (len(sys.argv) == 1):
    print "Usage: web_page_convert.py <input_file> <output_file"
    exit()


if (len(sys.argv) > 1):
    try:
        in_file = open(sys.argv[1], "rb")
    except IOError:
        print "Couldn't open ", sys.argv[1]
        exit(1)
else:
    try:
        in_file = open("webpages.espfs", "rb")
    except IOError:
        print "Couldn't open webpages.espfs"
        exit(1)

    print "Input from webpages.espf in current directory"

if (len(sys.argv) > 2):
    try:
        out_file = open(sys.argv[2], "w")
    except IOError:
        print "Couldn't open output file ", sys.argv[2]
        exit(2)
else:
    try:
        out_file = open("webpage.h", "w")
    except IOError:
        print "Couldn't open output file webpage.h"
        exit(2)
    print "Output to webpage.h in current directory"


out_file.write("#ifndef USER_WEBPAGE_H_\n")
out_file.write("#define USER_WEBPAGE_H_\n")
out_file.write("static const uint8_t ICACHE_RODATA_ATTR web_page[] = {")

items_per_line = 30
byte_count = 0
beginning = 1

while True:
    # if this isn't the start then put in a comma separator
    # - used to avoid a trailing comma at the end of the file
    if beginning != 1:
        out_file.write(", ")
    else:
        beginning = 0

    # read and output the next byte
    next_byte = in_file.read(1)

    # at EOF?
    if (next_byte == ''):
        break
    out_file.write(str(ord(next_byte)))
    byte_count += 1
    if byte_count > items_per_line:
        out_file.write("\n")
        byte_count = 0
#end of while loop
        
out_file.write("};\n")
out_file.write("#endif /* USER_WEBPAGE_H_ */")

in_file.close()
out_file.close()

print "File System Conversion finished"
exit(0)

