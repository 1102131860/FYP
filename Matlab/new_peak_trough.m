% [peaks,troughs,maximagram,minimagram] = PEAK_TROUGH_FINDER(data, {max-interval})
% data: input data as vector
% sampling_frequency (optional): sampling frequency of input
% Returns: vectors [peaks, troughs, maximagram, minimagram] containing
% indices of the peaks and troughs and the maxima/minima scalograms
function [peaks,troughs] = new_peak_trough(data,varargin)
 N = length(data);
 L = ceil(N/2) - 1;
%  Detrend the data
 if nargin == 2
    data = data - varargin{1};
 else
    data = detrend(data, 'linear');
 end
 Mx = zeros(N, L);
 Mn = zeros(N, L);
 %Produce the local maxima scalogram
 for j=1:L
 	k = j;
 	for i=k+2:N-k+1
 		if data(i-1) > data(i-k-1) && data(i-1) > data(i+k-1)
 			Mx(i-1,j) = true;
 		end
 		if data(i-1) < data(i-k-1) && data(i-1) < data(i+k-1)
 			Mn(i-1,j) = true;
 		end
 	end
 end
 %Form Y the column-wise count of where Mx is 0, a scale-dependent distribution of
 %local maxima. Find d, the scale with the most maxima (== most number
 %of zeros in row). Redimension Mx to contain only the first d scales
 Y = sum(Mx==true, 1);
 [~, d] = max(Y);
 Mx = Mx(:,1:d);
 %Form Y the column-wise count of where Mn is 0, a scale-dependent distribution of
 %local minima. Find d, the scale with the most minima (== most number
 %of zeros in row). Redimension Mn to contain only the first d scales
 Y = sum(Mn==true, 1);
 [~, d] = max(Y);
 Mn = Mn(:,1:d);
 %Form Zx and Zn the row-rise counts of Mx and Mn's non-zero elements.
 %Any row with a zero count contains entirely zeros, thus indicating
 %the presence of a peak or trough
 Zx = sum(Mx==false, 2);
 Zn = sum(Mn==false, 2);
 %Find all the zeros in Zx and Zn. The indices of the zero counts
 %correspond to the position of peaks and troughs respectively
 peaks = find(~Zx);
 troughs = find(~Zn);
end