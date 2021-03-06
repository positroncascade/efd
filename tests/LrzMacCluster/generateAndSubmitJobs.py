#!/usr/bin/env python
# encoding: utf-8

import os
import re
import subprocess
import shutil
import sys
import time

# Add and parse arguments {{{
# -----------------------------------------------------------------------------
from argparse import ArgumentParser

parser = ArgumentParser()

parser.add_argument('-f', nargs='?', default=".*")
parser.add_argument('-s', nargs='?', default="")
parser.add_argument('-p', nargs='?', default="")
parser.add_argument('-t', nargs='?', default="1")
parser.add_argument('-m', nargs='?', default="1")
parser.add_argument('--with-server', dest='with_server', action='store_true')
parser.set_defaults(with_server=False)
parser.add_argument('-d', nargs=1)

args, unknown_args = parser.parse_known_args()
# -----------------------------------------------------------------------------
# }}}

template = (""
"#!/bin/bash\n"
"#\n"
"#SBATCH -J {0}\n"
"#SBATCH -o /home/hpc/pr63so/ga39puw2/jobs/logs/{0}.%j.%N.out\n"
"#SBATCH -D  /home/hpc/pr63so/ga39puw2/jobs/output\n"
"#SBATCH --mail-type=end\n"
"#SBATCH --mail-user=SlavaMikerov@gmail.com\n"
"#SBATCH --time=24:00:00\n"
"#SBATCH --get-user-env\n"
"#SBATCH --partition=bdz\n"
"#SBATCH --ntasks={1}\n"
"\n"
"DIR=$HOME/Project\n"
"bin=$DIR/.install/Gcc/Release/bin\n"
"conf=$DIR/tests/LrzMacCluster\n"
"\n"
"cwd=\"$(pwd)\"\n"
"output=\"$(pwd)/{3}\"\n"
"mkdir -p $output\n"
"mkdir -p $output/Precice\n"
"mkdir -p $output/Fluid\n"
"mkdir -p $output/Petsc\n"
"cp -f $conf/Precice/{2}.xml $output/Precice/{2}.xml\n"
"cp -f $conf/Fluid/{0}.xml $output/Fluid/{0}.xml\n"
"cp -f $conf/Petsc/Basic.conf $output/Petsc/Basic.conf\n"
"cd $output\n"
"\n"
"mpiexec -n {1} $bin/Fluid \\\n"
"  -o . \\\n"
"  -e Petsc/Basic.conf \\\n"
"  -s Fluid/{0}.xml")

template2 = (""
"#!/bin/bash\n"
"#\n"
"#SBATCH -J {0}\n"
"#SBATCH -o /home/hpc/pr63so/ga39puw2/jobs/logs/{0}.%j.%N.out\n"
"#SBATCH -D  /home/hpc/pr63so/ga39puw2/jobs/output\n"
"#SBATCH --mail-type=end\n"
"#SBATCH --mail-user=SlavaMikerov@gmail.com\n"
"#SBATCH --time=24:00:00\n"
"#SBATCH --get-user-env\n"
"#SBATCH --partition=bdz\n"
"#SBATCH --ntasks={1}\n"
"\n"
"DIR=$HOME/Project\n"
"bin=$DIR/.install/Gcc/Release/bin\n"
"conf=$DIR/tests/LrzMacCluster\n"
"\n"
"cwd=\"$(pwd)\"\n"
"output=\"$(pwd)/{4}\"\n"
"mkdir -p $output\n"
"mkdir -p $output/Precice\n"
"mkdir -p $output/Fluid\n"
"mkdir -p $output/Petsc\n"
"cp -f $conf/Precice/{2}.xml $output/Precice/{2}.xml\n"
"cp -f $conf/Precice/GeometryModeDirectForcingAction.py $output/Precice/GeometryModeDirectForcingAction.py\n"
"cp -f $conf/Fluid/{0}.xml $output/Fluid/{0}.xml\n"
"cp -f $conf/Petsc/Basic.conf $output/Petsc/Basic.conf\n"
"cd $output/Precice\n"
"mpiexec -n 1 binprecice server Fluid {2}.xml &\n"
"cd $output\n"
"\n"
"mpiexec -n {3} $bin/Fluid \\\n"
"  -o . \\\n"
"  -e Petsc/Basic.conf \\\n"
"  -s Fluid/{0}.xml")

for root, dirs, files in os.walk(args.d[0]):
  for file_name in files:
    if not re.match(args.f, file_name):
      continue
    basic_file_name = os.path.splitext(file_name)[0]
    sh_file_name = basic_file_name + ".sh"
    if os.path.exists(sh_file_name):
      os.remove(sh_file_name)
    fo = open(sh_file_name, "wb+")
    if (args.with_server):
      fo.write(template2.format(
        basic_file_name,
        args.t,
        args.p,
        args.m,
        basic_file_name + args.s));
    else:
      fo.write(template.format(basic_file_name,
        args.t,
        args.p,
        basic_file_name + args.s));
    fo.close()
    subprocess.call("sbatch {0} {1}".format(" ".join(unknown_args), sh_file_name), shell=True)
    # time.sleep(10)
