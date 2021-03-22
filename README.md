# MeerKAT_DSP

Process all 64 antennas, 1/16 freq band, 5s in time

Relvant memos include:

* BL MeerKAT presentation slide deck: [link](https://docs.google.com/presentation/d/1tKlvAaVFdGViZfZ6mD9XTiMshtWjChFadIcHzaJMpx8/edit?usp=sharing)
* BLUSE overview:  [link](https://docs.google.com/document/d/1uj7vAF1FXq7kQcGdi2lr7K2eg98MFW3d3eqsAB2Z3LQ/edit#heading=h.twuqnlahbx18)
* DSP plan: Upchannelization and beamforming schemes [link](https://docs.google.com/document/d/1mrrn3YFABuoYqy0pkphNJYT4j44_slB8VltTEUHSlv0/edit#)
* RAW input file format: [link](https://docs.google.com/document/d/1dnye0HHSlVqRXH7rQ7v3wly0qKg-3_9tGJzaTI-76s4/edit#)
* MK hashpipe to-do list: [link](https://docs.google.com/document/d/1NrggefvZZ1pxu1ArdtUJGn7RGECHQMxR_JlFeLr0jpc/edit#)

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
