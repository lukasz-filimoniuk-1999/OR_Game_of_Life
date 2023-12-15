#!/bin/bash

#SBATCH -n 8
#SBATCH -e gol.err

mpiexec ./gol
