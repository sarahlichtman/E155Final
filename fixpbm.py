### Python file to fix the .pbm files so they can more easily read into memory for the FPGA.
# The pbm files are initially in the form (hexadecimal)

# 5036 0a36 3430 2034 3830 0aff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ....

# The first line is mostly useless except for the last 5 bytes, so get rid of that. Want file to be in the form

# 001110101010100100000....
# 0101001010110101010101010101.....
# ....

# So that each is WIDTH binary digits long where WIDTH is the width of the image in pixels (inputted to the program from the command line)
# This way the fixed pbm file will be the EXACT bit mapped image and can be read into memory on the FPGA and read directly.

import sys
FILE = sys.argv[1]
WIDTH = int(sys.argv[2])

with open(FILE,'r') as f:
	s = f.read()
s = (s.replace(' ','',1)).replace('\n','',2)
print len(s)

binary_list = []
for i in range(len(s)):
	if i > 7:
		byte_string = bin(ord(s[i]))[2:]
		len_diff = 8 - len(byte_string)
		byte_string = '0'*len_diff + byte_string
		if len(byte_string) != 8:
			print byte_string
		binary_list.append(byte_string)
new_s = ''.join(binary_list)
print len(new_s)

binary_list = []
for i in range(len(new_s)):
	if i % WIDTH == 0 and i != 0:
		binary_list.append('\n')
	binary_list.append(new_s[i])
final_s = ''.join(binary_list)
print len(final_s)

with open("fixed"+FILE,"w") as f:
	f.write(final_s)

