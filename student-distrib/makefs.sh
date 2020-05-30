cd ../syscalls
make -f donotopen
cd ..
cp --remove-destination syscalls/to_fsdir/* fsdir/
./createfs -i fsdir -o student-distrib/filesys_img
cd student-distrib
make clean
make dep
sudo make > /dev/null
