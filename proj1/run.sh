echo "Program output is:"
echo "  '<RANK>/POS (row, col)'             -- The position of each process"
echo "  '<RANK>/NBRS  (up,down,left,right)' -- The ranks expected at each relative pos"
echo "  '<RANK>/INBUF (up,down,left,right)' -- The ranks received from each relative pos"
echo " "
echo " "
mpirun -np 16 -f mpd.hosts ./cart.o
mpirun -np 9 -f mpd.hosts ./cart.o
