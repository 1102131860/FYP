function checked = artifact(peaks,troughs,t,data) 
  checked = zeros(length(data),1);

  for i = 1 : length(troughs) - 1
      if i ~= 1 
          PWRT = new_PWRT;
          PWA = new_PWA;
          PWD = new_PWD;
      end

      % find next peak adjacent to this valley
      for j = 1 : length(peaks) 
          if t(peaks(j)) > t(troughs(i)) 
             break;
          end
      end

      % continous valleys
      if t(peaks(j)) > t(troughs(i+1))
          for k = troughs(i) : troughs(i+1)
             checked(k) = -1;
             continue;
          end
      end
      % continous peaks
      if j < length(peaks) && t(peaks(j+1)) < t(troughs(i+1))
          for k = peaks(j) : peaks(j+1)
             checked(k) = -2;
             continue;
          end
      end

      new_PWRT = t(peaks(j)) - t(troughs(i));
      new_PWA = data(peaks(j)) - data(troughs(i));
      new_PWD = t(troughs(i+1)) - t(troughs(i));
      [diastolicPeaks,~] = new_peak_trough(data(peaks(j):troughs(i+1))); % from peak to right trough
      peak_nb = length(diastolicPeaks);
      mono_flag = true;
      nega_flag = false;
      for k = troughs(i) + 1 : peaks(j)
          if data(k) < data(k-1) 
              mono_flag = false;
              break;
          end    
      end 
      for k = peaks(j) : troughs(i+1) - 1
          if data(k) < data(troughs(i+1)-1) % smaller than the PWE
              nega_flag = true;
              break;
          end
      end

      % checked one wave
      if new_PWRT > 0.49 || new_PWRT < 0.08 ...
        || new_PWRT/(new_PWD-new_PWRT) > 1.1 ...
        || new_PWD > 2.4 || new_PWD < 0.27 
          for k = troughs(i) : troughs(i+1) - 1 
              checked(k) = 1;
          end
      end
      
      if length(peak_nb) > 2
          for k = troughs(i) : troughs(i+1) - 1 
              checked(k) = 3;
          end
      end

      if mono_flag == false
          for k = troughs(i) : troughs(i+1) - 1 
              checked(k) = 4;
          end
      end

      if nega_flag == true
          for k = troughs(i) : troughs(i+1) - 1 
              checked(k) = 5;
          end
      end

      if new_PWA/(data(troughs(i+1))-data(peaks(j))) > 0.4 || (data(troughs(i+1))-data(peaks(j)))/new_PWA > 0.4
        for k = troughs(i) : troughs(i+1) - 1 
              checked(k) = 6;
        end
      end

      if i == 1 
          continue;
      end
      % checked two adjacent waves
      if new_PWRT/PWRT > 3 || new_PWRT/PWRT < 1/3 ...
        || new_PWD/PWD > 3 || new_PWD/PWD < 1/3 ...
        || new_PWA/PWA > 4 || new_PWA/PWA < 1/4
          for k = troughs(i) : troughs(i+1) 
              checked(k) = 2;
          end
      end
      
  end
end