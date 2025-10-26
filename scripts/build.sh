#SCRIPT_PATH=$(shell pwd)

echo "********************************"
echo "* Building driver...           *"
echo "********************************"
cd ../kernel
make

echo "********************************"
echo "* Building C application...    *"
echo "********************************"
cd ../user/cli
gcc -o main-c-app main.c

echo "********************************"
echo "* Done!                        *"
echo "********************************"

#cd $(SCRIPT_PATH)

