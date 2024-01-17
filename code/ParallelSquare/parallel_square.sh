#!/bin/bash

#SBATCH -n 4
#SBATCH -e parallel_square.err

mpiexec ./parallel_square
