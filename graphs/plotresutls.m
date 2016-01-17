prof1 = csvread('profile1.dat');
prof2 = csvread('profile2.dat');

figure, 
time = 0.05 * length(prof1(:,2));
x = linspace(0, time , length(prof1(:,2)) );
subplot(2,1,1);
plot (x,prof1(:,2:3));
hold all; 

time = 0.05 * length(prof2(:,2));
x = linspace(0, time , length(prof2(:,2)) );
plot (x,prof2(:,2:3));
ylabel('Page Faults');
xlabel('Time in seconds')

clear out
for i=1:12
a = ['profile3_' num2str(i,'%u') '.data' ];
prof = csvread(a);
out(i) = mean(prof(:,4)) / 10;
end

%figure,
subplot(2,1,2);
plot(out);
ylabel('% CPU Utilization');
xlabel('Degree of multiprograming')


% time = 0.05 * length(prof3_1(:,4));
% x = linspace(0, time , length(prof3_1(:,4)) );
% plot (x,prof3_1(:,4));
% hold all; 
% 
% time = 0.05 * length(prof3_2(:,4));
% x = linspace(0, time , length(prof3_2(:,4)) );
% plot (x,prof3_2(:,4));
% hold all; 
% 
% time = 0.05 * length(prof3_3(:,4));
% x = linspace(0, time , length(prof3_3(:,4)) );
% plot (x,prof3_3(:,4));
% hold all; 
