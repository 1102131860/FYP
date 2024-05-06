function min_nb = number_min(max_min_index_arr, window_size)

min_nb = 0;
for i = 1 : window_size
    if (max_min_index_arr(i) == -1)
        min_nb = min_nb + 1;
    end
end

end