#!/bin/bash

#SBATCH -n 4
#SBATCH -e parallel_square_openmp.err

mpiexec ./parallel_square_openmp