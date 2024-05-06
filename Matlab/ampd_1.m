% import numpy as np
% 
% def AMPD(data):
%     """
%     实现AMPD算法
%     :param data: 1-D numpy.ndarray 
%     :return: 波峰所在索引值的列表
%     """
%     p_data = np.zeros_like(data, dtype=np.int32)
%     count = data.shape[0]
%     arr_rowsum = []
%     for k in range(1, count // 2 + 1):
%         row_sum = 0
%         for i in range(k, count - k):
%             if data[i] > data[i - k] and data[i] > data[i + k]:
%                 row_sum -= 1
%         arr_rowsum.append(row_sum)
%     min_index = np.argmin(arr_rowsum)
%     max_window_length = min_index
%     for k in range(1, max_window_length + 1):
%         for i in range(k, count - k):
%             if data[i] > data[i - k] and data[i] > data[i + k]:
%                 p_data[i] += 1
%     return np.where(p_data == max_window_length)[0]

function [peaks, rescaling_data, max_window_length] = ampd_1(data) 
  N = length(data);
  L = ceil(N/2) - 1;
  scalogram_sum = zeros(L,1);
  rescaling_data = zeros(N,1);
  for k = 1 : L 
      row_sum = 0;
      for i = k : N - k - 1
          if data(i) > data(i-k+1) && data(i) > data(i+k+1) 
              row_sum = row_sum - 1;
          end
      end
      scalogram_sum(k) = row_sum;
  end
  [~,min_index] = min(scalogram_sum);
  max_window_length = min_index;
  for k = 1 : max_window_length
      for i = k : N - k - 1 
          if data(i) > data(i-k+1) && data(i) > data(i+k+1) 
              rescaling_data(i) = rescaling_data(i) + 1; 
          end
      end
  end
  peaks = find(rescaling_data == max_window_length - 1);
end