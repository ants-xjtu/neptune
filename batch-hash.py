# call hash-Moon.py multiple time to ensure that 
# each NF can have up to $number of hashed instances

from genericpath import isfile
import os
import shutil
import subprocess
import sys


base_path = '/home/hypermoon/neptune-yh/libs'


if __name__ == '__main__':
    if len(sys.argv) != 4:
        # the reason we need two args here is that we cannot directly copy the hashed version
        # ...or can we?
        print('usage: python batch-hash.py nf_directory number nf_source')
        exit()
    
    if os.path.isfile(sys.argv[1]) == False:
        print(sys.argv[1], 'nf_directory does not exist')
        exit()
    
    if os.path.isfile(sys.argv[-1]) == False:
        print(sys.argv[-1], 'nf_source does not exist')
    
    # the name of NF and its directory might differ
    directory_name = sys.argv[1].split('/')[-2]
    # extract the name of NF from nf_source instead of nf_directory
    # nf_name        = sys.argv[1].split('/')[-1]
    nf_name        = sys.argv[-1].split('/')[-1]

    # do this until $number of subdirectories are created
    number = int(sys.argv[2])
    for i in range(2, number+1):
        this_path = os.path.join(base_path, directory_name+str(i))
        this_nf   = os.path.join(this_path, nf_name)
        
        if os.path.exists(this_path) == False:
            print('creating:', this_path)
            os.mkdir(this_path)
        # if the file exists, do nothing
        if os.path.isfile(this_nf) == False:
            shutil.copy2(sys.argv[-1], this_nf)
        # we find a path with extra files than our newly copied one, ignore this dir
        if len(os.listdir(this_path)) != 1:
            print('ignoring:', this_path)
            continue
        
        # print('Preparing to call', this_nf, nf_name[0]+str(i))
        # use 2-digit hash key to serve short library names. e.g. libc
        subprocess.call(['python', 'hash-Moon.py', this_nf, nf_name[0]+str(i)])
