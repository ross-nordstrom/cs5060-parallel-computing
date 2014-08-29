echo "Sample data > 4 processes > stripe partitioning:"
mpirun -np 2 -f mpd.hosts ./mmult_stripe.o
echo "Sample data > 4 processes > block partitioning:"
mpirun -np 4 -f mpd.hosts ./mmult_block.o
