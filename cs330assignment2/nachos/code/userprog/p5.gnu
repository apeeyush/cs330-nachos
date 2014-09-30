reset
set term png truecolor
set output "Maximum_CPU_Utilisation"
set xlabel "Time Quantum"
set ylabel "CPU Utilisation"
set grid
set boxwidth 0.95 relative
set style fill transparent solid 0.5 noborder
plot "p4.out" u 1:2 w boxes lc rgb"green" notitle