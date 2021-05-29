#!/bin/bash

port_1=$1
let port_2=$1+1
let port_3=$1+2
let port_4=$1+3

tab="--tab"
declare -a cmd_list=(
    "bash -c 'socat TCP4-LISTEN:$port_1 TCP4-LISTEN:$port_2';bash" 
    "bash -c 'socat TCP4-LISTEN:$port_3 TCP4-LISTEN:$port_4';bash" 
    "bash -c 'sleep 1 && ./sample_nv_hevc_sender --alsologtostderr=True --foreign_port=$port_1';bash" 
    "bash -c 'sleep 2 && ./hevc_visualizer --alsologtostderr=True --foreign_port=$port_2';bash"
    "bash -c 'sleep 1 && ./vehicle_communicator_client --alsologtostderr=True --foreign_port=$port_3';bash"
    "bash -c 'sleep 2 && ./fake_vehicle_node --alsologtostderr=True --foreign_port=$port_4';bash" )
foo=""

for val in "${cmd_list[@]}"; do
  foo+=($tab -e "$val")
done

echo "${foo[@]}"

gnome-terminal "${foo[@]}"

exit 0