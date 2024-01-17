#!/bin/bash

#SBATCH -n 4
#SBATCH -e parallel_openmp.err

mpiexec ./parallel_openmp
