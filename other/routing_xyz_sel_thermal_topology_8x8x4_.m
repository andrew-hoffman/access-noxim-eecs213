% fname: routing_xyz_sel_thermal_topology_8x8x4_.m
% ../bin/noxim -routing xyz -sel thermal -dimx 8 -dimy 8 -dimz 4  -sim 800000 -warmup 10000 -size 8 8 -buffer 4 -traffic random -vertical mesh -throt normal -clean 1000 

function [max_pir, max_throughput, min_delay] = routing_xyz_sel_thermal_topology_8x8x4_(symbol)

data = [
%             pir      avg_delay     throughput      max_delay   total_energy       rpackets         rflits
            0.018        20.9586       0.142461            219        2.27151        3601400       28811349
             0.02        22.5686        0.15835            405        2.52106        4003080       32024778
            0.022        24.7646       0.174262            423         2.7678        4405330       35242823
            0.024        28.1572       0.189992            524        3.01476        4802974       38423910
];

rows = size(data, 1);
cols = size(data, 2);

data_delay = [];
for i = 1:rows/1,
   ifirst = (i - 1) * 1 + 1;
   ilast  = ifirst + 1 - 1;
   tmp = data(ifirst:ilast, cols-6+1);
   avg = mean(tmp);
   [h sig ci] = ttest(tmp, 0.1);
   ci = (ci(2)-ci(1))/2;
   data_delay = [data_delay; data(ifirst, 1:cols-6), avg ci];
end

figure(1);
hold on;
plot(data_delay(:,1), data_delay(:,2), symbol);

data_throughput = [];
for i = 1:rows/1,
   ifirst = (i - 1) * 1 + 1;
   ilast  = ifirst + 1 - 1;
   tmp = data(ifirst:ilast, cols-6+2);
   avg = mean(tmp);
   [h sig ci] = ttest(tmp, 0.1);
   ci = (ci(2)-ci(1))/2;
   data_throughput = [data_throughput; data(ifirst, 1:cols-6), avg ci];
end

figure(2);
hold on;
plot(data_throughput(:,1), data_throughput(:,2), symbol);

data_maxdelay = [];
for i = 1:rows/1,
   ifirst = (i - 1) * 1 + 1;
   ilast  = ifirst + 1 - 1;
   tmp = data(ifirst:ilast, cols-6+3);
   avg = mean(tmp);
   [h sig ci] = ttest(tmp, 0.1);
   ci = (ci(2)-ci(1))/2;
   data_maxdelay = [data_maxdelay; data(ifirst, 1:cols-6), avg ci];
end

figure(3);
hold on;
plot(data_maxdelay(:,1), data_maxdelay(:,2), symbol);

data_totalenergy = [];
for i = 1:rows/1,
   ifirst = (i - 1) * 1 + 1;
   ilast  = ifirst + 1 - 1;
   tmp = data(ifirst:ilast, cols-6+4);
   avg = mean(tmp);
   [h sig ci] = ttest(tmp, 0.1);
   ci = (ci(2)-ci(1))/2;
   data_totalenergy = [data_totalenergy; data(ifirst, 1:cols-6), avg ci];
end

figure(4);
hold on;
plot(data_totalenergy(:,1), data_totalenergy(:,2), symbol);


%-------- Saturation Analysis -----------
slope=[];
for i=2:size(data_throughput,1),
    slope(i-1) = (data_throughput(i,2)-data_throughput(i-1,2))/(data_throughput(i,1)-data_throughput(i-1,1));
end

for i=2:size(slope,2),
    if slope(i) < (0.95*mean(slope(1:i)))
        max_pir = data_throughput(i, 1);
        max_throughput = data_throughput(i, 2);
        min_delay = data_delay(i, 2);
        break;
    end
end
