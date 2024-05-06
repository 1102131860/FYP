function index = loc_min_max(targ, thre, window_size)
% find local maximums and minimums for the targeted line in the
% region restricted by another threshold line
% @return
% index: a vector containing the index implying local maxs and mins, where
% local maximum is marked with 1, suggesting that index hold the max 
% loacl minimum is marked with -1, suggesting that index hold the min
% @args
% targ: a vector which is the line local maximums and minimums locates at
% thre: a vector which marks the local region
% window_size: the size of window, it should be less than the difference of
% len and window_start

index = zeros(1,window_size); % initialize it with 0;

max_diff = targ(1) - thre(1); % initialize firstly
max_inde = 1;
min_diff = targ(1) - thre(1);
min_inde = 1;

% firstly segement mark
% then find maxmimums and minimums
for i = 2 :  window_size  % assume the window_size is larger than 1
    diff = targ(i) - thre(i);
    if (targ(i) > thre(i) && targ(i-1) < thre(i-1)) % when enter a peak stage
        max_diff = diff; % initialization
        max_inde = i;
    end
    if (targ(i) < thre(i) && targ(i-1) > thre(i-1)) % when enter a valley stage
        min_diff = diff; % initialization
        min_inde = i;
    end

    if (diff > 0 && diff > max_diff) % diff > 0 for 1st uncomplete region
        max_diff = diff;
        index(max_inde) = 0; % change the original tested false maximum index to 0
        max_inde = i; % update the new one
        index(max_inde) = 1; % change the nest tested maximum index to 1
    end
    if (diff < 0 && diff < min_diff) % diff < 0 for 1st uncomplete region
        min_diff = diff;
        index(min_inde) = 0; % change the original tested false mainimum to 0
        min_inde = i; % update the new one
        index(min_inde) = -1; % change the nest tested minimum index to -1
    end
end

end
