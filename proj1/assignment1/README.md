# Usage

## Build
```
./build.sh
```

## Run
```
./run.sh
```

## Test
The test suite parses the program's output, confirming the inter-process communication occurred correctly.

```
python test.py
```

# Implementation
We use MPI's cartesian tools to create a topology and identify neighbors. Then each process sends its rank to each of its neighbors.a
