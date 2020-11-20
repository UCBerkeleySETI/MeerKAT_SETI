# MeerKAT_DSP

Process all 64 antennas, 1/16 freq band, 5s in time

See [link](https://docs.google.com/document/d/1mrrn3YFABuoYqy0pkphNJYT4j44_slB8VltTEUHSlv0/edit#) for further information the DSP plan. 

## Start the program:

Currently supporting "Option 1" in the DSP plan, i.e. Upchannelize all the way to 1Hz channels and then coh/incoh beamform (CPU verification)

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
