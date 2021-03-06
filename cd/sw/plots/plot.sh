#!/bin/sh
if [ "$1" -a "$2" ]; then
gnuplot << EOF
p = $2              # interval in seconds between measurements
#plot '$1' using (\$0*p):1 #To get the max and min value
#ymax=GPVAL_DATA_Y_MAX
#ymin=GPVAL_DATA_Y_MIN
#ylen=ymax-ymin
#xmax=GPVAL_DATA_X_MAX
#xmin=GPVAL_DATA_X_MIN
#xlen=xmax-xmin
# plotting
#set title "Local clock offset with adjustments and NTP poll interval 16s"
set terminal png
set output "$1.png"
set grid y
#set xrange [xmin:xmax]
#set yrange [ymin-0.15*ylen:ymax+0.15*ylen]
#set format y "%l ns"
set ylabel "Offset [ns]"
set xlabel "Uptime [s]"
#set key box
set xzeroaxis linetype -1           # xzeroaxis same as border
plot '$1' using (\$0*p):1 notitle with linespoints
#plot '$1' using (\$0*p):1 notitle with linespoints,\
#     '$1' using (xmax+0.1*xlen):(\$1):(1.1*xlen)\
#     smooth unique with xerrorbars notitle,\
#     ymax with l lt 3 notitle,\
#     ymin with l lt 3 notitle
#pause -1
EOF

else
echo "Pass the name of input file and interval between measurements as parameters"
fi
