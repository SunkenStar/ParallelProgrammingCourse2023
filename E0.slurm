#!/bin/bash

#SBATCH -J test_hpc_2   #作业名
#SBATCH -p gpu-quota
#SBATCH -N 1            #1个节点
#SBATCH -n 8            #8个cpu
#SBATCH -o test_hpc_2.out # 将屏幕的输出结果保存到当前文件夹的test_hpc_2.out 
#SBATCH -e test_hpc_2.err # 将屏幕的输出结果保存到当前文件夹的test_hpc_2.err

srun hostname | sort > machinefile.${SLURM_JOB_ID}
NP=`cat machinefile.${SLURM_JOB_ID} | wc -l`
module load intel/19.0.5.281
export I_MPI_HYDRA_TOPOLIB=ipl
mpirun -genv I_MPI_FABRICS shm:dapl -np ${NP} -f ./machinefile.${SLURM_JOB_ID} ./helloworld