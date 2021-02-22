# MeerKAT_DSP

Process all 64 antennas, 1/16 freq band, 5s in time

See [this memo](https://docs.google.com/document/d/1mrrn3YFABuoYqy0pkphNJYT4j44_slB8VltTEUHSlv0/edit#) for further information the DSP plan. 

## Start the program:

To just read a raw file and send to null ouput:
```
./readfile_init.sh
```

To Upchannelize (FFT) and coh/incoh beamform (see "Option 1" of the DSP plan). The follow is a CPU-only version for verification:
```
./cpu_upchan_beamform.sh
```


## Hashpipe monitor:

```
hashpipe_status_monitor.rb
```

## Clean up:

Check existing shared memory and semaphores

```
ipcs -a
```

Remove shared memory and semaphores as desired

```
ipcrm -m

ipcrm -s
```

Remove hashpipe stauts in /dev/shm

```
rm /dev/shm/sem.home_*
```
