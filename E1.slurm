#!/bin/bash

#SBATCH -J E1-ring   #作业名
#SBATCH -p cpu-low
#SBATCH -N 1            #1个节点
#SBATCH -n 8            #8个cpu
#SBATCH -o E1-ring.out # 将屏幕的输出结果保存到当前文件夹的E1-ring.out 
#SBATCH -e E1-ring.err # 将屏幕的输出结果保存到当前文件夹的E1-ring.err

srun hostname | sort > machinefile.${SLURM_JOB_ID}
NP=`cat machinefile.${SLURM_JOB_ID} | wc -l`
module load intel/19.0.5.281
export I_MPI_HYDRA_TOPOLIB=ipl
mpirun -genv I_MPI_FABRICS shm:dapl -np ${NP} -f ./machinefile.${SLURM_JOB_ID} ./E1-ring