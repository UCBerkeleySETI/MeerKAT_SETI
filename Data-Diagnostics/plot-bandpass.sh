#!/bin/bash
#
# A script to make bandpass plot by splicing ics filterbanks
# Run it in the working directory
# Using conda env cherry-dev2 (to get blimpy)
#------------
current_date_time="`date +%Y-%m-%d-%H:%M:%S`";
echo Start time $current_date_time;

stem=$1 #stem="guppi_59143_55142_000486_GPS-BIIR-11_0001"
fullpath=`find /mnt/blpn49/ -name $stem.0000.raw 2>/dev/null| awk -F 'guppi_' '{print $1}'| head -n 1 `
path=`echo $fullpath| awk -F'/mnt/blpn49/' '{print $2}'`
wd=`pwd`

for i in {48..63} #node IDs
do
    datapath="/mnt/blpn"$i"/"$path
    echo working on node $i $datapath
    cp $datapath/$stem.0000.raw .
    rawspec -f 1 -t 65536 --ics=1 -d $wd $stem
    current_date_time="`date +%Y-%m-%d-%H:%M:%S`";
    echo ---- Done file at $current_date_time;
    fil="`ls g*ics.rawspec.0000.fil`"
    mv $fil blpn$i\_$fil
    rm *.raw
done
splice2 blpn*fil -o Combined_$fil
python plot-bandpass.py Combined_$fil
