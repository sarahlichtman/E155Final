### Python file to fix the .ppm files so they can more easily read into memory for the FPGA.
# The ppm files are initially in the form (hexadecimal)

# 5036 0a36 3430 2034 3830 0a32 3535 0aff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff
# ffff ffff ffff ffff ffff ffff ffff ffff

# The first line is mostly useless except for the last byte (the last two characters) So get rid of all that but keep the last two.
# Want file to be


# ffffff ffffff ffffff ffffff ffffff
# ffffff ffffff ffffff ffffff ffffff

# So that each line in memory is a full rgb triplet (each of the three colors gets one byte)

## Note to Stephen - Instead of displaying what combination of notes is being played using a single image (which would require 256
##	images), instead have 8 images prepared (one for each note) and have the ones that are being played lit up.

import sys
FILE = sys.argv[1]

with open(FILE,"r") as f:
	s = f.read()

s = ((s.replace(' ','')).replace('\n',''))[30:]
new_s_list = [];

for i in range(len(s)):
	if i % 16 == 0 and i != 0:
		new_s_list.append(' ')
	if i % 36 == 0 and i != 0:
		new_s_list.append('\n')
	new_s_list.append(s[i]);
new_s = ''.join(new_s_list)

with open("fixed"+FILE,"w") as f:
	f.write(new_s)

