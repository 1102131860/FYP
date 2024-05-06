function next = valley_next_peak(t_vec,t_nb,i,time)
% t_max: time vector of peak or valley
% t_nb: the length of time vector
% i: this index
% time: this time
% next: the index of next peak or valley to this give peak or valley
% if no next, return -1

next = -1; % indicates there is no next
right_find = false;

j = i;
while(j <= t_nb)
    if (t_vec(j) > time) 
        next = j;
        right_find = true;
        break;
    end
    j = j + 1;
end

if (right_find == false && j < t_nb) % avoid find from last index of vector
    j = i - 1;
    while(j >= 1) % find left side
        if (t_vec(j) < time)
            next = j + 1;
            break;
        end
        j = j - 1;
    end
end

end