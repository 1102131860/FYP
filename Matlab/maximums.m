function maxs = maximums(max_nb,max_min_index_arr,vector) 

maxs = zeros(1,max_nb);
j = 1;
for i = 1:max_nb
    while (max_min_index_arr(j) ~= 1) 
        j = j + 1; 
    end
    maxs(i) = vector(j); % this is for addition
    j = j + 1;
end

end