#!/bin/bash

plot() {
    exec 3>&1 4>&2
    exec 1>/dev/null 2>&1
    cd plots
    gnuplot -persist <<-GNU_PLOT_SCRIPT
set terminal png
set key box
set key width 1
set key font "Arial,12"
set grid
set border 3
set tics nomirror

set xlabel "Bottleneck Datarate (Mbps)"
set ylabel "Throughput (Kbps)"
set output "bttlnkRate_thr/graph/$1.png"
plot "bttlnkRate_thr/data/$1.dat" using 1:3 title 'TcpNewReno' with l, "" using 1:4 title "$1" with l
set ylabel "Fairness Index"
set output "bttlnkRate_jain/graph/$1.png"
plot "bttlnkRate_thr/data/$1.dat" using 1:5 title 'TcpNewReno+$1' with l

set xlabel "Packet Loss Rate (%)"
set ylabel "Throughput (Kbps)"
set output "errorRate_thr/graph/$1.png"
plot "errorRate_thr/data/$1.dat" using 2:3 title 'TcpNewReno' with l, "" using 2:4 title "$1" with l
set ylabel "Fairness Index"
set output "errorRate_jain/graph/$1.png"
plot "errorRate_thr/data/$1.dat" using 2:5 title 'TcpNewReno+$1' with l

set ylabel "Cwnd (KB)"
set xlabel "Time (s)"
set output "cwnd_time/graph/$1.png"
plot "cwnd_time/data/$1.dat" using 1:2 title "$1 Cwnd" with l, "cwnd_time/data/TcpNewReno.dat" using 1:2 title "TcpNewReno Cwnd"  with l

GNU_PLOT_SCRIPT
    cd ../
    exec 1>&3 2>&4
}

run() {
    echo "Simulation: TcpNewReno vs $1"
    >plots/bttlnkRate_thr/data/$1.dat
    >plots/errorRate_thr/data/$1.dat
    arr=("1" "5" "10" "20" "50" "100" "200" "300")
    for i in "${arr[@]}"; do
        echo -e "Bottleneck Rate = $i Mbps\tError rate = 10^-6 %\tSimulation Time = 50s\tApplication Data rate = 1024Mbps"
        ../build/scratch/ns3.39-1905072-default --bttlnkRate=$i --lossRateExp="6" --algo=$1 --simulTime=50 --appRate=1024 >/dev/null 2>>plots/bttlnkRate_thr/data/$1.dat
    done

    for ((i = 2; i <= 6; i++)); do
        echo -e "Bottleneck Rate = 50 Mbps\tError rate = 10^-$i %\tSimulation Time = 50s\tApplication Data rate = 1024Mbps"
        ../build/scratch/ns3.39-1905072-default --bttlnkRate=50 --lossRateExp=$i --algo=$1 --simulTime=50 --appRate=1024 >/dev/null 2>>plots/errorRate_thr/data/$1.dat
    done

    echo "Cwnd vs Time"
    ../ns3 run "1905072.cc" -- --bttlnkRate=50 --lossRateExp="6" --algo=$1 --traceCwnd=1 --simulTime=50 --appRate=1024 >/dev/null 2>&1
}
[ ! -d "plots" ] && mkdir plots
[ -d "plots/bttlnkRate_thr" ] && rm -r "plots/bttlnkRate_thr"
mkdir -p "plots/bttlnkRate_thr/data"
mkdir -p "plots/bttlnkRate_thr/graph"
[ -d "plots/errorRate_thr" ] && rm -r "plots/errorRate_thr"
mkdir -p "plots/errorRate_thr/data"
mkdir -p "plots/errorRate_thr/graph"
[ -d "plots/bttlnkRate_jain" ] && rm -r "plots/bttlnkRate_jain"
# mkdir -p "plots/bttlnkRate_jain/data"
mkdir -p "plots/bttlnkRate_jain/graph"
[ -d "plots/errorRate_jain" ] && rm -r "plots/errorRate_jain"
# mkdir -p "plots/errorRate_jain/data"
mkdir -p "plots/errorRate_jain/graph"
[ -d "plots/cwnd_time" ] && rm -r "plots/cwnd_time"
mkdir -p "plots/cwnd_time/data"
mkdir -p "plots/cwnd_time/graph"

echo "Building 1905072.cc"
../ns3 build "1905072.cc" >/dev/null 2>&1 && echo "Build successful!"

echo "Running 1905072.cc"
run "TcpAdaptiveReno"
plot "TcpAdaptiveReno"
run "TcpHighSpeed"
plot "TcpHighSpeed"
run "TcpWestwoodPlus"
plot "TcpWestwoodPlus"

echo "Plot data generated"
# plotAll
echo "Graphs plotted!!"
