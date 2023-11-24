#!/bin/bash

#SBATCH -J E3-MPI_pthread  #作业名
#SBATCH -p cpu-quota
#SBATCH -N 1            #4个节点
#SBATCH -n 32            #32个cpu
#SBATCH -o E3-MPI_pthread.out # 将屏幕的输出结果保存到当前文件夹的E3-MPI_pthread.out 
#SBATCH -e E3-MPI_pthread.err # 将屏幕的输出结果保存到当前文件夹的E3-MPI_pthread.err

srun hostname | sort > machinefile.${SLURM_JOB_ID}
NP=`cat machinefile.${SLURM_JOB_ID} | wc -l`
module load intel/19.0.5.281
export I_MPI_HYDRA_TOPOLIB=ipl
mpirun -genv I_MPI_FABRICS shm:dapl -np ${NP} -f ./machinefile.${SLURM_JOB_ID} ./E3-MPI_pthread A.dat B.dat C.dat
