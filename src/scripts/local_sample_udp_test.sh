#!/bin/bash

port_1=$1

tab="--tab"
declare -a cmd_list=(
    "bash -c 'sleep 1 && ./sample_nv_hevc_udp_sender --alsologtostderr=True --foreign_port=$port_1';bash" 
    "bash -c 'sleep 2 && ./hevc_udp_visualizer --alsologtostderr=True --port=$port_1';bash" )
foo=""

for val in "${cmd_list[@]}"; do
  foo+=($tab -e "$val")
done

echo "${foo[@]}"

gnome-terminal "${foo[@]}"

exit 0