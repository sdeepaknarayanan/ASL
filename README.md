# Team 29 - Volume Estimation
## Team Members:
- Samuel Kiegeland
- S Deepak Narayanan
- Vraj Patel
- Raghu Raman Ravi

## Project Dependencies:
In terms of external dependencies, we require the presence of the following libraries and tools at minimum to be able to build and run the code:
- GCC 11
- Armadillo
- Lapack
- BLAS
- GLPK

## Build instructions:
Once you are in the suitable branch, 
- **GCC Build**: Run `make`
- **Clang Build**: Run `make clang`
- **Debug Build**: Run `make debug`

**Note**: For the branch `main`, if you want to compile with specific input sizes in mind, you need to modify the `#define M m` and `#define N n` lines in the file `estimateVol.cpp` to the desired dimensions.  

## Running the executable:
The executable `polyvol` is generated at the end of the build process. It can then be utilized as follows:
```bash
./polyvol [input-file-name]
```

The format for the input file is as follows:
- The first line contains the space-separated integers $m$ and $n$.
- $m$ lines follow, each containing $n+1$ space-separated numbers (`float` or `int`). The number in the $j$<sup>th</sup> column of the $i$<sup>th</sup> line represents $A_{ij}$ for $1 \le j \le n$ and $B_j$ for $j=n+1$.

## Generation of testcases:
To generate the testcases, you can run `gen.py` as follows:
```bash
python3 gen.py
```

The description and precise definition of the polytopes generated can be found in the report. 

## Testing:
Once you have run the testcase generation script at least once, to populate the `./tests/` directory with the appropriate input files, you can now test the correctness of the built executable. To do so, you can to run `test.py` as follows:
```bash
python3 test.py
```

This above will build and test the output of the default target of the make file. You can choose another build target to test as follows:
```bash
python3 test.py [build-target-name]
```
