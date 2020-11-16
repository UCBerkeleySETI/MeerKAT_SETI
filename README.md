# MeerKAT_DSP

Start the program:
# Process all 64 antennas, 1/16 freq band, 5s in time
# Upchannelize all the way to 1Hz channels and then coh/incoh beamform (CPU verification)
./cpu_upchan_beamform.sh

Clean up:
# Check existing shared memory and semaphores
ipcs -a
# Remove shared memory and semaphores as desired
ipcrm -m
ipcrm -s
# Remove hashpipe stauts in /dev/shm
rm /dev/shm/sem.home_*