# script to copy and hash single onvm NF for multiple times
# TODO: seperate main function to avoid code duplication

import os
import shutil
import subprocess

allNF = ['scan', 'firewall', 'encrypt', 'decrypt']
# use a short and simple hash key
hashKey = ['s', 'f', 'e', 'd']
sourceDir = '/home/hypermoon/Qcloud/pure_rtc/build'
destDir = '/home/hypermoon/neptune-yh/onvm'
maxCore = 16


if __name__ == '__main__':
    # check if we have compiled all the NF needed
    for nf in allNF:
        currPath = os.path.join(sourceDir, nf)
        if os.path.exists(currPath) == False:
            print("Error: cannot find file", currPath)
            exit()
            pass
    # check if we have created the directory
    for idx, nf in enumerate(allNF):
        coreCtr = 1
        while coreCtr <= maxCore:
            hashed = 1    
            if coreCtr == 1:
                currPath = os.path.join(destDir, nf)
            else:
                currPath = os.path.join(destDir, nf+str(coreCtr))
            if os.path.exists(currPath) == False:
                # print("Attempt to mkdir", currPath)
                os.mkdir(currPath)
                hashed = 0
                pass
            # copy each NF into maxCore instances
            # print("copy file from", os.path.join(sourceDir, nf), "to", currPath)
            shutil.copy2(os.path.join(sourceDir, nf), currPath, follow_symlinks=True)
            
            # hashing
            # print("calling hash-moon with argv[2]:", currPath+'/'+nf, "argv[3]:", hashKey[idx]+str(coreCtr))
            if hashed == 0:
                subprocess.call(['python', 'hash-Moon.py', currPath+'/'+nf, hashKey[idx]+str(coreCtr)])
            coreCtr += 1
    