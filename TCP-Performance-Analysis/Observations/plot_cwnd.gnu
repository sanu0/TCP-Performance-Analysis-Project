# plot_cwnd.gnu
set terminal png
set output 'goodPut_singleFlow.png'

set title 'Goodput over Time'
set xlabel 'Time (s)'
set ylabel 'Goodput'
set grid

plot 'Single_Flow_1to4.gp' using 1:2 with lines title 'TCP-Hybla', \
 'Single_Flow_2to5.gp' using 1:2 with lines title 'TCP-Westwood+', \
 'Single_Flow_3to6.gp' using 1:2 with lines title 'TCP-yEAH'	
