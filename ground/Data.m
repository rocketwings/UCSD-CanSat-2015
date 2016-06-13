data = csvread('datanoheader.csv');

GPS = data(1:41,13);
time = data(1:41,3);

dist = trapz(time,GPS)


dist/85
