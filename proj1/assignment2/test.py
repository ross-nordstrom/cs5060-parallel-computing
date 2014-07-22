import math
import subprocess
import numpy      # sudo apt-get install python-numpy
import os

class Util:
   @classmethod
   def mmult(cls, a,b):
      c = numpy.random.random_integers(0,0,(len(a),len(b)))
      for i in range(len(a)):
         for j in range(len(b[0])):
            c[i][j] = 0
            for k in range(len(b[0])):
               c[i][j] += int(a[i][k] * b[k][j])
      return c.astype(int)
   @classmethod
   def rand_matrix(cls, size):
      m = numpy.random.random_integers(0,100, (size,size))
      return m.astype(int)
   @classmethod
   def testmmult(cls,p,c,script, verbose=False):
      # Run the program and check its output
      cmd = ["mpirun","-np",str(p),"-f","mpd.hosts",script]
      p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      out, err = p.communicate()

      # Convert output into object
      if verbose:
         print "OUTPUT WAS:"
         print out

      if 'ERROR' in out or out.isspace() or not out:
         return True
      outVal = [ map(int,row.split()) for row in out.split("\n") if(len(row.split()) > 0) ]
      if verbose:
         print "ACCURACY:"
         print (c == outVal)
      error = False in (c == outVal)
      return error



print "Verify matrix multiplication"
print "Test with sample data..."

errorCnt = 0

# Test sample
a = [
   [1,2,3,4],
   [5,6,7,8],
   [9,10,11,12],
   [13,14,15,16]
]
b = [
   [101,102,103,104],
   [105,106,107,108],
   [109,110,111,112],
   [113,114,115,116]
]
c = Util.mmult(a,b)

print a
print "*"
print b
print "="
print c
print "-----------------------"

for scheme in ['stripe','block']:
   script = "./mmult_"+scheme+".o"
   if Util.testmmult(4,c,script):
      print "Tested sample data\t\033[91m{}\033[0m     \t{}".format('X', scheme)
      errorCnt += 1
   else:
      checkMark = u'\u2713'
      print u"Tested sample data\t\033[92m{}\033[0m     \t{}".format(checkMark, scheme)

cmd = ["mv","data.txt","data.bak"]
p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
out, err = p.communicate()

cmd = ["rm","data*.err", "data*.exp"]
p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
out, err = p.communicate()

print " "
print "Test random inputs..."
inputSets = [[4,4],[4,6],[4,8],[4,24],[9,9],[9,18],[9,27],[16,16],[16,32],[25,25],[25,50]]
for proc,n in inputSets:
   # n = proc*proc
   # print "Test with {} processors".format(proc)
   a = Util.rand_matrix(n)
   b = Util.rand_matrix(n)
   c = Util.mmult(a,b)

   # print a
   # print "*"
   # print b
   # print "="
   # print c
   # print "-----------------------"

   # Setup data.txt input
   fo = open("data.txt", "wb")
   fo.write(str(n)+"\n")
   fo.write(" \n")
   for row in a:
      fo.write( " ".join(map(str,row)) +"\n" )
   fo.write(" \n")
   for row in b:
      fo.write( " ".join(map(str,row)) +"\n" )
   fo.close()

   for scheme in ['stripe','block']:
      script = "./mmult_"+scheme+".o"
      if Util.testmmult(proc,c,script):
         print "Tested with P:{}, N: {}\t\033[91m{}\033[0m  [{}]\t{}".format(proc, n, 'X', errorCnt, scheme)
         cmd = ["cp","data.txt","data"+str(errorCnt)+".err"]
         p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
         out, err = p.communicate()
         errf = open( 'data'+str(errorCnt)+'.exp', 'w' )
         errf.write(str(c)+"\n")
         errf.close()
         errorCnt += 1
      else:
         checkMark = u'\u2713'
         print u"Tested with P:{}, N: {}\t\033[92m{}\033[0m     \t{}".format(proc, n, checkMark, scheme)



cmd = ["mv","data.bak","data.txt"]
p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
out, err = p.communicate()

print "Ran against {} different input sets...".format(1+len(inputSets))
print "\n\n"
if errorCnt == 0:
   print u"Success!\t\t\t\033[92m{}\033[0m".format(proc, checkMark)
else:
   print  "AH! {} errors...\t\t\t\033[91m{}\033[0m".format(errorCnt, 'X')
