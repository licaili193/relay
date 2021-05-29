#!/bin/bash

addr=$1
port_2=$2
port_3=$3

tab="--tab"
declare -a cmd_list=(
    "bash -c './hevc_visualizer --alsologtostderr=True --foreign_addr=$addr --foreign_port=$port_1';bash"
    "bash -c './vehicle_communicator_client --alsologtostderr=True --foreign_addr=$addr --foreign_port=$port_2';bash" )
foo=""

for val in "${cmd_list[@]}"; do
  foo+=($tab -e "$val")
done

echo "${foo[@]}"

gnome-terminal "${foo[@]}"

exit 0