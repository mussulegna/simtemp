#function unloadModule {
#    sudo rmmod nxp_simtemp
#    return $?
#}

# If there is an error in any command exit
set -e

# Load kernel module
sudo insmod ../kernel/nxp_simtemp.ko
echo Kernel module loaded.

# Configure sampling, threshold, mode
#echo 1000 | sudo tee /sys/class/simtemp/sampling_ms
sudo sh -c "echo 1000 > /sys/class/simtemp/sampling_ms"
sudo sh -c "echo 30000 > /sys/class/simtemp/threshold_mc"
sudo sh -c "echo ramp > /sys/class/simtemp/mode"
echo Kernel module configured.

# Execute CLI
echo Start application.
cd ../user/cli/
./main-c-app
echo Application closed.

# Unload kernel module
sudo rmmod nxp_simtemp
echo Kernel module unloaded.

echo

# Check module's messages of load/unload
sudo dmesg | grep '[simtemp]' | tail -16

echo
echo Script Done!
exit 0
