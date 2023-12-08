#!/bin/bash

plot() {
    exec 3>&1 4>&2
    exec 1>/dev/null 2>&1
    cd wifi-offline/$2
    gnuplot -persist <<-GNU_PLOT_SCRIPT
set terminal png size 640,480
set key box
set key width 1
set key font "Arial,12"
set grid
set border 3
set tics nomirror
set xlabel "$1"
set ylabel "delivery(%)"
set output "$1-dr.png"
plot "xplot-$1.dat" using $3:6 title 'delivery VS $1' with linespoints
set ylabel "throughput(Kbps)"
set output "$1-tp.png"
plot "xplot-$1.dat" using $3:7 title 'throughput VS $1' with linespoints
GNU_PLOT_SCRIPT
    cd ../../
    exec 1>&3 2>&4
}
run() {
    # echo "../build/scratch/ns3.39-1905072_$8-default --numNodes=$((20 * $1)) --numFlows=$((10 * $2)) --packetsPerSecond=$((100 * $3)) --nodeSpeed=$((5 * $4)) --coverageMultiplier=$((1 * $5)) >/dev/null 2>>wifi-offline/$6/xplot-$7.dat"
    ../build/scratch/ns3.39-1905072_$8-default --numNodes=$((20 * $1)) --numFlows=$((10 * $2)) --packetsPerSecond=$((100 * $3)) --nodeSpeed=$((5 * $4)) --coverageMultiplier=$((1 * $5)) 2>>wifi-offline/$6/xplot-$7.dat
}

plotAll() {
    plot "node" "mobile" 1
    plot "flow" "mobile" 2
    plot "pps" "mobile" 3
    plot "speed" "mobile" 4
}

if [ -n "$1" ]; then
    n=$1
else
    n=5
fi

[ ! -d "wifi-offline" ] && mkdir wifi-offline
[ -d "wifi-offline/mobile" ] && rm -r wifi-offline/mobile
mkdir wifi-offline/mobile

echo "Building 1905072_2.cc"
../ns3 build "1905072_2.cc" >/dev/null 2>&1 && echo "Build successful!"

echo "Running 1905072_2.cc"
for ((i = 1; i <= n; i++)); do
    run $i $i 2 2 1 "mobile" "node" 2
    run 1 $i 1 1 1 "mobile" "flow" 2
    run 4 4 $i 1 1 "mobile" "pps" 2
    run 2 2 2 $i 1 "mobile" "speed" 2

    if [ $# = 2 ] && [ $2 = '-v' ]; then
        plotAll
    fi

    echo "Progress: $i/$n done"
done
echo "Plot data generated"

plot "node" "mobile" 1
plot "flow" "mobile" 2
plot "pps" "mobile" 3
plot "speed" "mobile" 4
echo "Graphs plotted"
