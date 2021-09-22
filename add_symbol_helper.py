from subprocess import run
from sys import argv

exe_path = argv[1]
cp = run(f"readelf -S {exe_path} | grep .text", shell=True, capture_output=True)
addr = cp.stdout.split()[-1]
result_addr = int(addr, 16) + 0x7fff07fff000
print(f'add-symbol-file {exe_path} {hex(result_addr)}')
