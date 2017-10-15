import sys
import re


path = "../results/experiments/"
####### TOTAL REQUESTS ########
res_file = path+'totalRequests.txt'
o_file = path+'OUTTotalRequests.txt'

in_file = open(res_file, 'r')
out_file = open(o_file, 'a')
num = 0
for line in in_file.readlines():
    val = re.findall(r':(.*)', line)
    num += int(val[0])

out_file.write('Total Network Requests:' + str(num))

in_file.close()
out_file.close()


####### TOTAL CHECKS ########
res_file = path+'totalChecks.txt'
o_file = path+'OUTTotalChecks.txt'


in_file = open(res_file, 'r')
out_file = open(o_file, 'a')
num = 0
for line in in_file.readlines():
    
    val = re.findall(r':(.*)', line)
    num += int(val[0])

out_file.write('Total Network Checks:' + str(num))

in_file.close()
out_file.close()



####### SUCCESFUL TRUST DELAY ########
res_file = path+'successDelay.txt'
o_file = path+'OUTMeanSuccessDelay.txt'


in_file = open(res_file, 'r')
out_file = open(o_file, 'a')
num = 0.0
cnter = 0

for line in in_file.readlines():
    val = re.findall(r':(.*)', line)

    if (float(val[0])) > 0.0:
        cnter += 1

    num += float(val[0])

if cnter > 0:
    final = num/cnter
    out_file.write('Mean Delay for Successful Trust(sim seconds): ' + str(final)) #if you want to use commas to seperate the files, else use something like \n to write a new line.

in_file.close()
out_file.close()


####### MEAN CHAIN LENGTH ########
res_file = path+'chainLength.txt'
o_file = path+'OUTMeanChainLegth.txt'


in_file = open(res_file, 'r')
out_file = open(o_file, 'a')
num = 0
cnter = 0

for line in in_file.readlines():
    val = re.findall(r':(.*)', line)
    cnter += 1
    num += float(val[0])


final = float(num/cnter) # total chain length, number of nodes that found trust


out_file.write('Mean Chain Length for Trust: ' + str(final))

in_file.close()
out_file.close()



########## NUM OF REQUESTS THAT LEAD TO TRUST ########
# read the total number of trusts
res_file = path+'successfulTrusts.txt'

in_file = open(res_file, 'r')
num = 0

for line in in_file.readlines():
    val = re.findall(r':(.*)', line)
    num += int(val[0])


in_file.close()

# read the total Requests
res_file1 = path+'OUTTotalRequests.txt'
o_file = path+'OUTTotalReqs-LedToTrust.txt'

in_file = open(res_file1, 'r')
out_file = open(o_file, 'a')


for line in in_file.readlines():
    val = re.findall(r':(.*)', line)
    totalReqs = val[0]

out_file.write('From the ' + totalReqs + ' requests, ' + str(num) + ' led to Trust')
in_file.close()
out_file.close()



#### Mean Trust Percentage ####
res_file = path+'trustPercentage.txt'
o_file = path+'OUTMeanTrustPercentage.txt'


in_file = open(res_file, 'r')
out_file = open(o_file, 'a')
num = 0.0
cnter = 0

for line in in_file.readlines():
    val = re.findall(r':(.[^%]*)', line)
    cnter += 1
    num += float(val[0])

final = float(num/cnter) # mean


out_file.write('Mean number of Trust Over Total Requests in the network: ' + str(final))

in_file.close()
out_file.close()

### Mean Checks for each Trust ###
res_file = path+'numChecksForEachTrust.txt'
o_file = path+'OUTMeanNumChecksForEachTrust.txt'

in_file = open(res_file, 'r')
out_file = open(o_file, 'a')
num = 0
cnter = 0

for line in in_file.readlines():
    val = re.findall(r':(.[^%]*)', line)
    cnter += 1
    num += int(val[0])

final = float(num / cnter)  # mean


out_file.write('Mean Number of Checks for each Trust in the network: ' + str(final))

in_file.close()
out_file.close()
