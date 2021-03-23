set -e

cd build
cmake --build /home/hypermoon/neptune/build --config Release --target all -- -j 98
cd ..

cd glibc-build
make bench BENCHSET="malloc-simple" &> malloc-baseline.txt
make bench BENCHSET="malloc-simple2" &> malloc-custom.txt
diff malloc-baseline.txt malloc-custom.txt