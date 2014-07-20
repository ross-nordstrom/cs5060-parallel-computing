import math
import subprocess

class Util:
   @classmethod
   def getExpectedLines(cls, proc, n):
      expectedLines = list()
      for rank in range(0, proc):
         col = int(rank % n)
         row = int(math.floor(rank / n))
         up =    int( (rank-n)%proc )
         down =  int( (rank+n)%proc )
         left =  int( (row*n + (col-1)%n)%proc )
         right = int( (row*n + (col+1)%n)%proc )
         expectedLines.append("{}/POS ({},{})".format(rank, row, col))
         expectedLines.append("{}/NBRS ({},{},{},{})".format(rank, up, down, left, right))
         expectedLines.append("{}/INBUF ({},{},{},{})".format(rank, up, down, left, right))
      return expectedLines

print "Verify cartesian logic. Expect each process in cartesian topology to send its Rank to its neighbors"

for proc in [4, 9, 16, 25, 36, 49, 64, 81, 100]:
   n = math.sqrt(proc)
   print "Test with {} processors".format(proc)
   expectedLines = Util.getExpectedLines(proc,n)
   cmd = ["mpirun", "-np", str(proc), "-f", "mpd.hosts", "./cart.o"]
   p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE,
                           stdin=subprocess.PIPE)
   out, err = p.communicate()
   lines = out.split("\n")
   print "Test {} proc output...".format(proc)
   errCnt=0
   for line in expectedLines:
      if line not in lines:
         print "Error! Expected to find line: '{}'".format(line)
         errCnt += 1
      else:
         lines.remove(line)
   if errCnt==0:
      print "Success!"
   else:
      print "Error! Missing {} expected lines".format(errCnt)
      print " "
      print "Incorrect lines from program: "
      print "\n".join(lines)


