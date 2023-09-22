% processSnowtatos_v0.m

% a script for processing SnowTATOS .bin data files into excel files

% Ian Raphael
% ian.a.raphel.th@dartmouth.edu
% 2023.09.18

close all
clear

%  define some macros (TODO: is there a way to define macros?)
cd("/Users/"+getenv('USER')+"/Desktop/SnowTATOS_data")
addpath(genpath(pwd));
dataDirectory = dir;

% number of snowatatos stations
numStations = 9;

% number of temp sensors in a station
numTempSensors = 3;

% size of one temp sensor's data in bytes
tempDataSize = 2;

% size of the pinger data in bytes
pingerDataSize = 1;

% data size for an individual station
stationDataSize = (numTempSensors * tempDataSize) + pingerDataSize;

% size of the timestamp in bytes
timeStampSize = 4;

% total data size in the bytestream
totalDataSize = stationDataSize * numStations + timeStampSize;

% load the lookup table for the temp sensor positions
load tempPosition_LUT.mat

% allocate a structure to hold the data
data = struct;

for i=1:numStations
    data(i).pinger = [];
    data(i).temp = [];
end

% iterate over the data directory
for i=1:length(dataDirectory)
    
    % if it's one of the binary files we're looking for
    if endsWith(dataDirectory(i).name,'.bin')
        
        % open it as a fileread object
        fr = matlab.io.datastore.DsFileReader(dataDirectory(i).name);
        
        % now for every station
        for stationID = 1:numStations
%             
%             % get the start byte
%             startByte = (stationID - 1) * stationDataSize;
%             
%             % SEEKING SEEKS TO POSITION+SEEK. NOT "SEEKTO" 
              % seek to the correct location
%             seek(fr,startByte,'RespectTextEncoding',true);
            
            % clear the array to hold the temps
            temp_field = [];
            
            % decode the temperature data
            temp_field(1) = swapbytes(read(fr,2,'OutputType','int16'))/1000.0;
            temp_field(2) = swapbytes(read(fr,2,'OutputType','int16'))/1000.0;
            temp_field(3) = swapbytes(read(fr,2,'OutputType','int16'))/1000.0;
            
            % figure out which temp field corresponds with which depth for
            % this station and save temps to the right place
            % and create a row vector to append to the data struct
            newTempRow = [temp_field(tempPositionLUT(stationID,1)),...
                temp_field(tempPositionLUT(stationID,2)),...
                temp_field(tempPositionLUT(stationID,3))];
            % append it to the data
            data(stationID).temp = [data(stationID).temp;newTempRow];
            
            % decode the pinger data
%             pingerByte = startByte+stationDataSize-1;
%             seek(fr,pingerByte,'RespectTextEncoding',true);
            pingerValue_cm = read(fr,1,'OutputType','uint8');
            % append it to the data
            data(stationID).pinger = [data(stationID).pinger;pingerValue_cm];
        end
        
        % get the timestamp
    end
end

% save each to the appropriate cells

% close the file

% if the excel file exists

% open it

% otherwise

% create a new excel file