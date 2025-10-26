# Load kernel module
sudo insmod ../kernel/nxp_simtemp.ko

# Read temperature value
cat /dev/simtemp

# Unload kernel module
sudo rmmod nxp_simtemp

# Check module's messages of load/unload
sudo dmesg | grep 'simtemp'
