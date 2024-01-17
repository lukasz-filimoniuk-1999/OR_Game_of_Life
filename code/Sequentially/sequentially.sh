#!/bin/bash

#SBATCH -n 4
#SBATCH -e parallel.err

mpiexec ./parallel
