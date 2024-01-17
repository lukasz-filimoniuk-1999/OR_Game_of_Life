#!/bin/bash

#SBATCH -n 4
#SBATCH -e gol.err

mpiexec ./parallel
