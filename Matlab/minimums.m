function mins = minimums(min_nb,max_min_index_arr,vector) 

mins = zeros(1,min_nb);
j = 1;
for i = 1:min_nb
    while (max_min_index_arr(j) ~= -1) 
        j = j + 1; 
    end
    mins(i) = vector(j);
    j = j + 1;
end

end