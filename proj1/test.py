import subprocess

print "Verify cartesian logic. Expect each process in cartesian topology to send its Rank to its neighbors"

print "Test with 4 processors"
expectedLines = [
   "0/POS (0,0)",
   "0/NBRS (2,2,1,1)",
   "0/INBUF (2,2,1,1)",
   "1/POS (0,1)",
   "1/NBRS (3,3,0,0,",
   "1/INBUF (3,3,0,0,",
   "2/POS (1,0)",
   "2/NBRS (0,0,3,3)",
   "2/INBUF (0,0,3,3)",
   "3/POS (1,1)",
   "3/NBRS (1,1,2,2)",
   "3/INBUF (1,1,2,2)",
]
cmd = ["mpirun", "-np", "4", "-f", "mpd.hosts", "./cart.o"]
p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,
                          stdin=subprocess.PIPE)
out, err = p.communicate()
lines = out.split("\n")
print "Test 4 proc output..."
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
   print "Incorrect lines from program: "
   print "\n".join(lines)
