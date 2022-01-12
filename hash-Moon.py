# rename a whole dependency tree of a library
import shutil
import sys
from elftools.elf.elffile import ELFFile
import subprocess

visited = []
process = []

# when debugging, prepend a path here to use a library compiled by yourself
sysPath = ['/home/hypermoon/Qcloud/glib/_build/glib/', \
        '/home/hypermoon/dpdk-20.11/build/lib/', \
        '/home/hypermoon/Qcloud/numactl/.libs/', \
        '/usr/lib/x86_64-linux-gnu/',\
        '/lib/x86_64-linux-gnu/',\
        '/usr/local/lib/',\
        '/usr/local/lib/x86_64-linux-gnu/', \
        '/home/hypermoon/Qcloud/pure_rtc/libonvm/']
# specify those libraries that are shared, i.e., not hashed as a dependency
unchanged = ['libc.so.6', \
        'ld-linux-x86-64.so.2', \
        'libpthread.so.0', \
        'libm.so.6', \
        'libgcc_s.so.1', \
        'libstdc++.so.6']
# unchanged = []


def hash(key, filename):
    if filename in unchanged:
        # print(filename, "remain unchanged during hashing")
        return filename

    exeName = key
    # TODO: use a more messy hash function. Hash key 'L2Fwd' and 'L2Fwd2' would have
    #       no difference on 'libdl.so.2'
    # hash up the argument within its length, so I choose simple Casear shifting
    # for example, if the filename of executable is /path/to/click
    # and currently we are processing libc.so.6
    # print("hashing file:", filename)
    ctr = 0
    newname = ""
    for char in filename:
        # we just mess up the part before .
        if char == '.':
            newname += filename[ctr:]
            break
        a = ord(filename[ctr])
        b = ord(exeName[(ctr % len(exeName))])
        # map the name into a-z, numbers are ok, but I'm lazy...
        # maybe later when you find a collsion... but I don't think the prob is high
        a = ((a + b) % 26) + ord('a')
        a = chr(a)
        newname += a
        ctr += 1
    # print("hash result: ", filename, "->", newname)
    return newname


def worker():
    while process:
        filename = process.pop(0)
        visited.append(filename)
        # trying filename in absolute directory -> system dir -> export dir
        import os
        export_path = os.path.dirname(sys.argv[1])
        export_path = export_path + '/'
        try:
            f = open(filename, 'rb')
        except FileNotFoundError:
            try:
                f = open(export_path + filename, 'rb')
            except FileNotFoundError:
                for path in sysPath:
                    try:
                        f = open(path + filename, 'rb')
                    except FileNotFoundError:
                        pass
                    else:
                        print("using file", path + filename)
                        break
            else:
                print("using file from export path", export_path + filename)
        else:
            print("using file", filename)

        victim = ELFFile(f)
        for secid in range(victim.num_sections()):
            sec = victim.get_section(secid)
            if sec.name == '.dynamic':
                # print("entering the dynamic section of file", filename)
                dynLen = sec.num_tags()
                # print("number of dynamic tags is", dynLen)
                strtab = sec._get_stringtable()
                # print(strtab)
                replace_dict = {}
                for dtId in range(dynLen):
                    # print(sec.get_tag(dtId).entry.d_tag)
                    # print(sec.get_tag(dtId).entry.d_val)
                    if(sec.get_tag(dtId).entry.d_tag == 'DT_NEEDED'):
                        #print(strtab.get_string(sec.get_tag(dtId).entry.d_val))
                        depName = strtab.get_string(sec.get_tag(dtId).entry.d_val)
                        if depName == "libTianGou.so":
                            print("You are using a processed library! Please use a fresh copy instead")
                            exit(1)

                        # use simple hashing
                        # print("hashing file:", depName)
                        hashName = hash(sys.argv[2], depName)
                        # note that it would result in error when pyELFtools still needs to access the file
                        # and you use patchelf to overwrite it. Bad!
                        replace_dict[depName] = hashName
                        if (hashName in process) or (hashName in visited):
                            continue
                        # use the hashed name in process list
                        process.append(hashName)
                        if depName in unchanged:
                            continue
                        for path in sysPath:
                            try:
                                f = open(path+depName, "r")
                            except FileNotFoundError:
                                # print("Search failed when find", path+depName)
                                pass
                            else:
                                print("find file:", depName, "in path:", path)
                                # it's nice to know that copying a soft link results in copying the real file
                                print("copying file to:", export_path + hashName)
                                shutil.copyfile(path+depName, export_path + hashName)
                                break
                break
        # now we don't need the ELF file anymore, hack it!
        print("start rewriting:", filename)
        for key in replace_dict:
            if key in unchanged:
                print("dependency", key, " unchanged")
                continue
            print("modifying dependency:", key, "->", replace_dict[key])
            if filename[0] == '/':
                subprocess.run(["patchelf", "--replace-needed", key, replace_dict[key], filename])
            else:
                subprocess.run(["patchelf", "--replace-needed", key, replace_dict[key], export_path + filename])
        print("finish processing:", filename)    
                    


if __name__ == '__main__':
    if len(sys.argv) != 3:
        # NB: the alias should be the same as the directory name under libs/, otherwise
        # there is nowhere to export it.
        print("usage: python hash-dep.py absolute-filename alias(i.e. hash key)")
        exit(1)
    
    # print("modifying file from", sys.argv[1])
    # doing the job in a BFS style: executable find its dependencies, make a copy
    # using hash name, actually modify the DT_NEEDED of current ELF, repeat the job
    # recursively
    process.append(sys.argv[1])
    worker()
    # OK, hashing dependency is done. Now we insert tiangou
    print("Injecting tiangou...")
    import lief
    binary = lief.parse(sys.argv[1])
    binary.add_library("libTianGou.so")
    # print("overwriting PIE flag...")
    # binary[lief.ELF.DYNAMIC_TAGS.FLAGS_1].remove(lief.ELF.DYNAMIC_FLAGS_1.PIE)
    binary.write(sys.argv[1])
    print("done")

    # test hash function
    # hash("prads", "libc.so.6")
    # hash("prads", "libpthread.so.6")
    # hash("prads", "libfoo.so")
    # hash("Libnids", "libglib-2.0.so")
    # hash("L2Fwd", "librte_eal.so.21")