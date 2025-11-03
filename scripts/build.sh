
echo "********************************"
echo "* Building driver...           *"
echo "********************************"

cd ../kernel
make

if [ $? -ne 0 ]; then
    exit 1
fi

echo "********************************"
echo "* Building driver Done         *"
echo "********************************"

echo "********************************"
echo "* Building C application...    *"
echo "********************************"

cd ../user/cli
gcc -o main-c-app main.c

if [ $? -ne 0 ]; then
    exit 1
fi

echo "********************************"
echo "* Building C application Done  *"
echo "********************************"

echo "********************************"
echo "* All done!                    *"
echo "********************************"
exit 0
