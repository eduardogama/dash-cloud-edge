set view map;
set xlabel "X"
set ylabel "Y"
set cblabel "SINR (dB)"
unset key



DlRxBytes = load ("a2-a4-rsrq-DlRlcStats.txt") (:,10);
DlAverageThroughputKbps = sum (DlRxBytes) * 8 / 1000 / 50

UlRxBytes = load ("a2-a4-rsrq-UlRlcStats.txt") (:,10);
UlAverageThroughputKbps = sum (UlRxBytes) * 8 / 1000 / 50

DlSinr = load ("a2-a4-rsrq-DlRsrpSinrStats.txt") (:,6);

idx = isnan (DlSinr);
DlSinr (idx) = 0;
DlAverageSinrDb = 10 * log10 (mean (DlSinr)) % convert to dB


UlSinr = load ("a2-a4-rsrq-UlSinrStats.txt") (:,5);

idx = isnan (UlSinr);
UlSinr (idx) = 0;
UlAverageSinrDb = 10 * log10 (mean (UlSinr)) % convert to dB



plot "lena-dual-stripe.rem" using ($1):($2):(10*log10($4)) with image
set term png  
set output "printme-2-2.png"
replot
set term x11
