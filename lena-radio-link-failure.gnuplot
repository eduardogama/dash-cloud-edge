set terminal png
set output  "lena-radio-link-failure-one-enb-thrput.png"
set multiplot
set xlabel "Time [s]"
set ylabel "Instantaneous throughput UE [Mbps]"
set grid
set title "LTE RLF example 1 eNB DL instantaneous throughput"
plot "rlf_dl_thrput_2_eNB_ideal_rrc" using ($1):($2) with linespoints title 'Ideal RRC' linestyle 1 lw 2 lc rgb 'blue', "rlf_dl_thrput_2_eNB_real_rrc" using ($1):($2) with linespoints title 'Real RRC' linestyle 2 lw 2 lc rgb 'red'

unset multiplot
