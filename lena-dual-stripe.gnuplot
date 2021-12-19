set label "10" at 514.663,527.43 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "11" at 506.859,521.066 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "12" at 513.912,496.471 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "13" at 568.056,499.462 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "1" at 250.5,0 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "2" at 249.75,0.433013 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "3" at 249.75,-0.433013 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "4" at 0.5,433.013 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "5" at -0.25,433.446 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "6" at -0.25,432.58 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "7" at 500.5,433.013 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "8" at 499.75,433.446 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0
set label "9" at 499.75,432.58 left font "Helvetica,10" textcolor rgb "white" front  point pt 2 ps 0.3 lc rgb "white" offset 0,0

set label "1" at 515.358,522.726 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "2" at 507.011,524.019 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "3" at 510.969,499.666 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "4" at 558.887,495.429 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "5" at 139.919,430.26 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "6" at 569.871,189.56 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "7" at -29.2529,188.19 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "8" at -180.655,-45.8303 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "9" at 537.121,372.773 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "10" at 609.41,302.006 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "11" at 710.305,629.738 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "12" at 611.826,144.231 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "13" at -139.704,145.17 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "14" at 88.2024,53.5121 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "15" at 144.411,155.768 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "16" at 334.492,-164.349 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "17" at 150.248,-102.478 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "18" at 472.08,-23.5991 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "19" at 144.638,419.793 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "20" at 311.207,460.995 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "21" at 685.939,68.4519 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "22" at 535.722,22.5187 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0
set label "23" at -21.9726,347.854 left font "Helvetica,7" textcolor rgb "grey" front point pt 1 ps 0.3 lc rgb "grey" offset 0,0

set view map;
set xlabel "X"
set ylabel "Y"
set cblabel "SINR (dB)"
unset key
plot "lena-dual-stripe.rem" using ($1):($2):(10*log10($4)) with image
set term png  
set output "printme-2.png"
replot
set term x11
