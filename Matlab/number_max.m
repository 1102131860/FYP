function max_nb = number_max(max_min_index_arr, window_size)

max_nb = 0;
for i = 1 : window_size
    if (max_min_index_arr(i) == 1)
        max_nb = max_nb + 1;
    end
end

end