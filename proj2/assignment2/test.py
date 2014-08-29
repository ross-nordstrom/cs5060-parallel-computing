import math
import subprocess
import string
import random
import numpy      # sudo apt-get install python-numpy
import os

class Util:
   @classmethod
   def randomStr(cls,sz):
      return ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(sz))

   @classmethod
   def test(cls,x):
      # Run the program and check its output
      cmd = ["./pc_openmp.o"]
      p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      out, err = p.communicate()

      # Convert output into object
      error = True
      if out and out.split()[0] == x:
         error = False
      return error


print "Verify producer consumer"

cmd = ["mv","string.txt","string.bak"]
p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
out, err = p.communicate()

cmd = ["rm","string*.err", "string*.exp"]
p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
out, err = p.communicate()

print " "
print "Test random inputs..."
errorCnt = 0
timesPerIter = 10
inputSets = [[4,1,1],[4,1,2],[10,1,4],[10,2,1],[10,2,2],[20,4,2],[20,10,1],[100,10,4],[100,10,10],[100,25,5],[100,50,1],[100,25,5], [1000,1,1]]
for strLen,qLen,consumers in inputSets:

   # Setup string.txt input
   x = Util.randomStr(strLen)
   fo = open("string.txt", "wb")
   fo.write(x+"\n")
   fo.close()

   # Run with each data set 5 times
   for _ in range(timesPerIter):
      if Util.test(x):
         print "Tested with StrLen:{} \t\033[91m{}\033[0m  [{}]".format(strLen, 'X', errorCnt)
         cmd = ["cp","string.txt","string"+str(errorCnt)+".err"]
         p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
         out, err = p.communicate()
         errf = open( 'string'+str(errorCnt)+'.exp', 'w' )
         errf.write(x+"\n")
         errf.close()
         errorCnt += 1
      else:
         checkMark = u'\u2713'
         print u"Tested with StrLen:{} \t\033[92m{}\033[0m".format(strLen, checkMark)

cmd = ["mv","string.bak","string.txt"]
p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
out, err = p.communicate()

print "Ran against {} different input sets, {} times each...".format(1+len(inputSets), str(timesPerIter))
print "\n\n"
if errorCnt == 0:
   print u"Success!\t\t\t\033[92m{}\033[0m".format(checkMark)
else:
   print  "AH! {} errors...\t\t\t\033[91m{}\033[0m".format(errorCnt, 'X')
