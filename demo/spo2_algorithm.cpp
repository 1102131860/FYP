/** \file algorithm.cpp ******************************************************
*
* Project: MAXREFDES117#
* Filename: algorithm.cpp
* Description: This module calculates the heart rate/SpO2 level
*
*
* --------------------------------------------------------------------
*
* This code follows the following naming conventions:
*
* char              ch_pmod_value
* char (array)      s_pmod_s_string[16]
* float             f_pmod_value
* int32_t           n_pmod_value
* int32_t (array)   an_pmod_value[16]
* int16_t           w_pmod_value
* int16_t (array)   aw_pmod_value[16]
* uint16_t          uw_pmod_value
* uint16_t (array)  auw_pmod_value[16]
* uint8_t           uch_pmod_value
* uint8_t (array)   auch_pmod_buffer[16]
* uint32_t          un_pmod_value
* int32_t *         pn_pmod_value
*
* ------------------------------------------------------------------------- */
/*******************************************************************************
* Copyright (C) 2016 Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*******************************************************************************
*/

#include "Arduino.h"
#include "spo2_algorithm.h"

// The hyper-tuning parameter and updated by tested results
const int32_t max_n_peak = 16; // initialize with 16
const int32_t max_n_valley = 16; // initialize with 16
const int32_t filter_size = 15; // initalize with 14 to 17 is the best suitable for AMPD
const int32_t ratio_size = 16; // initalize with 16, the ratio size is at most equal to the number of heart interval

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated.  Samples become 16-bit data.
void heart_rate_and_oxygen_saturation(uint16_t* pun_green_buffer, uint16_t *pun_ir_buffer, uint16_t* pun_red_buffer, int32_t buffer_length,
    int32_t sampling_rate, int32_t *pn_spo2, int32_t *pn_heart_rate)
#else
void heart_rate_and_oxygen_saturation(uint32_t *pun_green_buffer, uint32_t *pun_ir_buffer, uint32_t* pun_red_buffer, int32_t buffer_length,
    int32_t sampling_rate, int32_t *pn_spo2, int32_t *pn_heart_rate)
#endif
/**
* \brief        Calculate the heart rate and SpO2 level
* \par          Details
*               By detecting  peaks of PPG cycle and corresponding AC/DC of red/infra-red signal, the an_ratio for the SPO2 is computed.
*               Since this algorithm is aiming for Arm M0/M3. formaula for SPO2 did not achieve the accuracy due to register overflow.
*               Thus, accurate SPO2 is precalculated and save longo uch_spo2_table[] per each an_ratio.
*
* \param[in]    *pun_green_buffer        - Green sensor data buffer
* \param[in]    *pun_ir_buffer           - IR sensor data buffer
* \param[in]    *pun_red_buffer          - Red sensor data buffer
* \param[in]    buffer_length            - data buffer length
* \param[in]    sampling_rate            - the actual sampling rate
* \param[out]    *pn_spo2                - Calculated SpO2 value, -1 represents the value is invalid
* \param[out]    *pn_heart_rate          - Calculated heart rate value, -1 represents the value is invalid
*
* \retval       None
*/
{
    Serial.printf("max_n_peak: %d, max_n_valley: %d, filter_size: %d, ratio_size: %d\n", max_n_peak, max_n_valley, filter_size, ratio_size);
    
    int32_t* peak_locs = (int32_t*)calloc(max_n_peak, sizeof(int32_t)); // peak location index array
    int32_t* valley_locs = (int32_t*)calloc(max_n_valley, sizeof(int32_t)); // valley location index array
    // if (peak_locs == NULL || valley_locs == NULL) { Serial.println(F("Inital Dynamic Memory Allocation Fails !!!")); return; }
    int32_t num_peak, num_val; // the actual peak number and valley number
    int32_t n_i_ratio_count; // the actual ratio counter/number
    int32_t n_peak_interval_sum; // used for update the filter_size

    // HR calculation
    *pn_heart_rate = HR_calculation(pun_green_buffer, buffer_length, peak_locs, &num_peak, max_n_peak, valley_locs, &num_val, max_n_valley, sampling_rate, &n_peak_interval_sum);
    Serial.printf("The num of peak is %d\n", num_peak);

    // SPO2 Calculation
    *pn_spo2 = spo2_calculation(pun_ir_buffer, pun_red_buffer, buffer_length, peak_locs, num_peak, valley_locs, num_val, ratio_size, &n_i_ratio_count);

    // update the hyper-tuning parameters
    // max number of valleys
    //if (num_peak > 3 * max_n_peak / 4)
    //    max_n_peak = 2 * max_n_peak;
    //if (num_peak < max_n_peak / 4 && max_n_peak >= 8)
    //    max_n_peak = max_n_peak / 2;
    //// max number of valleys
    //if (num_val > 3 * max_n_valley / 4)
    //    max_n_valley = 2 * max_n_valley;
    //if (num_val < max_n_valley / 4 && max_n_valley >= 8)
    //    max_n_valley = max_n_valley / 2;
    //// filter size
    //filter_size = n_peak_interval_sum > 280 && n_peak_interval_sum < 360 ? n_peak_interval_sum / 20 : 16;
    //// ratio_size
    //if (n_i_ratio_count > 3 * ratio_size / 4)
    //    ratio_size = 2 * ratio_size;
    //if (n_i_ratio_count < ratio_size / 4 && ratio_size >= 8)
    //    ratio_size = ratio_size / 2;

    // free the dynamic memory
    free(peak_locs); free(valley_locs);
    // avoid wild pointers
    peak_locs = NULL; valley_locs = NULL;
}

void maxim_find_peaks(int32_t *pn_locs, int32_t *n_npks, int32_t *valley_locs, int32_t *n_vals, int32_t *pn_x, int32_t n_size, int32_t max_threshold, int32_t min_threshold)
/**
* \brief        Find peaks
* \par          Details
*               Find at most MAX_NUM peaks above MIN_HEIGHT separated by at least MIN_DISTANCE
*
* \param[out]   *pn_locs                - peaks index array
* \param[out]   *n_npks                 - number of peaks
* \param[out]   *valley_locs            - valley index array
* \param[out]   *n_vals                 - number of valleys
* \param[in]    *pn_x                   - inversed, DC eliminiated, and filtered data buffer
* \param[in]    n_size                  - data buffer size
* \param[in]    max_threshold           - min peak height
* \param[in]    min_threshold           - max valley height
* 
* \retval       None
*/
{
  maxim_peaks_above_min_height(pn_locs, n_npks, pn_x, n_size, max_threshold, max_n_peak);
  maxim_remove_close_peaks(pn_locs, n_npks, pn_x, 5 * filter_size);
  *n_npks = min(*n_npks, max_n_peak); // this might be omitted, *n_npks <= max_n_peak;
  //Serial.printf("The number of valid peak before finding the valley is %d\n", *n_npks);
  //now we need to find the valley locs and number
  //maxim_valley_below_max_height(valley_locs, n_vals, pn_x, n_size, min_threshold, max_n_valley, pn_locs, *n_npks);
}

void maxim_peaks_above_min_height(int32_t *pn_locs, int32_t *n_npks, int32_t *pn_x, int32_t n_size, int32_t max_threshold, int32_t max_n_peaks)
/**
* \brief        Find peaks above max_threshold
* \par          Details
*               Find all peaks above MIN_HEIGHT
*
* \param[out]   *pn_locs                - peaks index array
* \param[out]   *n_npks                 - number of peaks
* \param[in]    *pn_x                   - inversed, DC eliminiated, and filtered data buffer
* \param[in]    n_size                  - data buffer size
* \param[in]    max_threshold           - min peak height/threshold
* \param[in]    max_n_peaks             - the max number of peaks
* 
* \retval       None
*/
{
  int32_t i = 1;
  int32_t n_width;
  *n_npks = 0; // max n_npks is max_n_peak
  
  while (i < n_size-1){ // i+1 < n_size
    if (pn_x[i] > max_threshold && pn_x[i] > pn_x[i-1]){      // find left edge of potential peaks
      n_width = 1;
      while (i+n_width < n_size && pn_x[i] == pn_x[i+n_width])  // find flat peaks
        n_width++;
      if (pn_x[i] > pn_x[i+n_width] && (*n_npks) < max_n_peaks){  // find right edge of peaks, the number must be limited to max_n_peak
        pn_locs[(*n_npks)++] = i;
        // for flat peaks, peak location is left edge
        i += n_width+1;
      } else
        i += n_width;
    } else
        i++;
  }
}

void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance)
/**
* \brief        Remove peaks
* \par          Details
*               Remove peaks separated by less than MIN_DISTANCE
* 
* \param[out]   *pn_locs                - peaks index array
* \param[out]   *pn_npks                - number of peaks
* \param[in]    *pn_x                   - inversed, DC eliminiated, and filtered data buffer
* \param[in]    n_min_distance          - min peak distance
*
* \retval       None
*/
{
  int32_t i, j, n_old_npks, n_dist;
    
  /* Order peaks from large to small */
  maxim_sort_indices_descend(pn_x, pn_locs, *pn_npks); // here pn_npks won't be change, so only pass the value

  // combination (C(2)(pn_npks)) problem
  for (i = -1; i < *pn_npks; i++ ){
    n_old_npks = *pn_npks;
    *pn_npks = i+1; // *pn_npks starts from 0
    for (j = i+1; j < n_old_npks; j++){
      n_dist =  pn_locs[j] - (i == -1 ? -1 : pn_locs[i]); // lag-zero peak of autocorr is at index -1
      if (n_dist > n_min_distance || n_dist < -n_min_distance) // absolute n_dist is larger than n_min_distance (4 indexs)
        pn_locs[(*pn_npks)++] = pn_locs[j];
    }
  }

  // Resort indices int32_to ascending order
  maxim_sort_ascend(pn_locs, *pn_npks); // here pn_npks won't be change, so only pass the value
}

void maxim_valley_below_max_height(int32_t *valley_locs, int32_t *n_vals, int32_t *pn_x, int32_t n_size, int32_t min_threshold, int32_t max_n_valley, int32_t *pn_locs, int32_t n_npks)
/**
* \brief         Find valley below min threshold given the peak indexs array
* \par           Datails
*                Find all the valleys below/under min height threshold
* 
* \param[out]    *valley_locs           - valley index array
* \param[out]    *n_vals                - number of valleys
* \param[in]     *pn_x                  - inversed, DC eliminiated, and filtered data buffer
* \param[in]     n_size                 - data buffer size
* \param[in]     min_threshold          - max height for valley
* \param[in]     max_n_valley           - the maximum number of valley buffer
* \param[in]     *pn_locs               - peak index array
* \param[in]     n_npks                 - exact number of peaks
* 
* \retval        None
*/
{
    *n_vals = 0;
    for (int32_t i = n_npks - 1; i > 0; i--) { // pn_locs[i] must be valid
        for (int32_t j = pn_locs[i]; j > pn_locs[i - 1]; j--) { // find the right edge of potential valley
            if (pn_x[j] < min_threshold && j + 1 < n_size && pn_x[j] < pn_x[j + 1]) { // must smaller than min height
                int32_t width = 1;
                while (j - width < n_size && pn_x[j] == pn_x[j - width]) // find flat peaks
                    width++;
                if (pn_x[j] < pn_x[j - width] && *n_vals < max_n_valley) { // find left edge of valley, the number must be limited in max_n_valley
                    valley_locs[(*n_vals)++] = j;
                    break;
                }
            }
        }
    }

    // the valley index array has the nestest valley index at left and oldest valley index at right
    // we have to sort in the order from the olderest index to nestest index (from left to right)
    if (*n_vals < 2) return; // don't need to sort the array any more
    int32_t middle_index = *n_vals % 2 ? (*n_vals - 1) / 2 : *n_vals / 2;
    for (int32_t i = 0; i < middle_index; i++) {
        int32_t temp = valley_locs[i];
        valley_locs[i] = valley_locs[*n_vals - i - 1];
        valley_locs[*n_vals - i - 1] = temp;
    }
}

void maxim_sort_ascend(int32_t *pn_x, int32_t n_size) 
/**
* \brief        Sort array
* \par          Details
*               Sort array in ascending order (insertion sort algorithm)
* 
* \param[out]   *pn_x                   - peaks index array
* \param[out]   *n_size                 - number of peaks
*
* \retval       None
*/
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_x[i];
    for (j = i; j > 0 && n_temp < pn_x[j-1]; j--)
        pn_x[j] = pn_x[j-1];
    pn_x[j] = n_temp;
  }
}

void maxim_sort_indices_descend(int32_t *pn_x, int32_t *pn_indx, int32_t n_size)
/**
* \brief        Sort indices
* \par          Details
*               Sort indices according to descending order (insertion sort algorithm)
* 
* \param[in]    *pn_x                   - inversed, DC eliminiated, and filtered data buffer
* \param[out]   *pn_indx                - peaks index array
* \param[in]    n_size                  - number of peaks
*
* \retval       None
*/ 
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_indx[i];
    for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j-1]]; j--)
      pn_indx[j] = pn_indx[j-1];
    pn_indx[j] = n_temp;
  }
}

void check_valid(int32_t *peak_locs, int32_t *n_npks, int32_t *valley_locs, int32_t *n_vals, int32_t *an_x, int32_t buffer_length, int32_t sampling_rate)
/**
* \brief            check valid of signal waveform
* \par              Datails
*                   It consists absolute and relative artifact detection.
* \param[in\out]    *peak_locs              - peak location index array
* \param[in\out]    *n_npks                 - number of peaks
* \param[in\out]    *valley_locs            - valley location index array
* \param[in\out]    *n_vals                 - number of valleys
* \param[in]        *an_x                   - inversed, DC eliminiated, and filtered data buffer
* \param[in]        buffer_length           - data buffer size
* \param[in]        sampling_rate           - the sampling frequency
* 
* \retval           None
*/
{
    Serial.printf("Enter the check_valid stage.\nBefore checking, the number of peaks: %d, the number of valleys: %d\n", *n_npks, *n_vals);
    
    int32_t num_npks = *n_npks;
    int32_t num_vals = *n_vals;
    *n_npks = 0; // clear again to store the checked valid peak and valley
    *n_vals = 0;

    bool piece_valid = false;
    float PWRT, PWD, PWA;
    float pre_PWRT = 0;
    float pre_PWD = 0;
    float pre_PWA = 0;
    int32_t i, j, k;
    for (i = 0; i < num_vals - 1; i++) { // i is valley index
        for (j = 0; j < num_npks - 1; j++) { // j is peak index
            // distance of adjacent valleys and peaks should exceed five times of filter_size
            // ensure triangle shape
            if (valley_locs[k + 1] - valley_locs[k] > 5 * filter_size && peak_locs[j + 1] - peak_locs[j] > 5 * filter_size &&
                peak_locs[j] > valley_locs[i] && valley_locs[i + 1] > peak_locs[j] && peak_locs[j + 1] > valley_locs[i + 1]) {
                piece_valid = false; // find a signal segment and firstly assume it is false
                PWA = (float) an_x[peak_locs[j]] - an_x[valley_locs[i]]; // pulsewave amplitude

                // absolute check starts
                //1. check pulsewave rising time
                PWRT = (float) (peak_locs[j] - valley_locs[i]) / sampling_rate;
                if (PWRT > 0.6f || PWRT < 0.08f) { Serial.printf("Absolute PWRT error. PWRT is %.2f\n", PWRT); break; }

                //2. check pulsewave duration
                PWD = (float) (valley_locs[i + 1] - valley_locs[i]) / sampling_rate;
                if (PWD > 2.7f || PWD < 0.27f) { Serial.printf("Absolute PWD error. PWD is %.2f\n", PWD); break; }

                //3. check the ratio of systolic phase time and diastolic phase
                float PWSDRatio = (float) PWRT / (PWD - PWRT); 
                if (PWSDRatio > 1.1f) { Serial.printf("Absolute PWSDRatop error. PWSDRatio is %.2f\n", PWSDRatio); break; }

                //4. check the number of peaks at diastolic phase
                /*int32_t number_of_diastolic_peak = 0; 
                for (k = peak_locs[j]; k < valley_locs[i + 1]; k++){
                    if (an_x[k] < an_x[k + 1]) number_of_diastolic_peak++;
                }
                if (number_of_diastolic_peak > 3) { Serial.println(F("Number of peaks at diastolic phase is larger than 3")); break; }*/

                //5. check whether the systolic phase is montonical increasing
                /*bool monotony_increasing = true;
                for (k = valley_locs[i]; k < peak_locs[j]; k++) {
                    if (an_x[k] > an_x[k + 1]) { monotony_increasing = false; break; }
                }
                if (!monotony_increasing) { Serial.println(F("Not monotonical increasing")); break; }*/

                //6. check whether there are points smaller than the valley
                /*for (k = peak_locs[j]; k < valley_locs[i + 1]; k++)
                    if (an_x[k] < an_x[valley_locs[i + 1]]) { Serial.println(F("Exists points smaller than the valley")); break; }*/

                //7. check whether pulse amplitude is distorted
                float left_right_amplitude_ratio = (float) (an_x[peak_locs[j]] - an_x[valley_locs[i]]) / (an_x[peak_locs[j]] - an_x[valley_locs[i + 1]]);
                if (left_right_amplitude_ratio < 0.4f || left_right_amplitude_ratio > 2.5f) { Serial.printf("Absolute pulse amplitude is distorted. left_right_amplitude_ratio is %.2f\n", left_right_amplitude_ratio); break; }

                // if it is the first signal segementation
                if (pre_PWRT == 0 && pre_PWD == 0 && pre_PWA == 0) { piece_valid = true; break; }
                
                // the relative checks start
                //8. check rise time variation validation
                float rise_time_variation = PWRT / pre_PWRT;
                if (rise_time_variation > 3.f || rise_time_variation < 0.33f) { Serial.printf("Relative PWRT error. rise time variation is %.2f\n", rise_time_variation); break; }

                // 9. check duration variation validation
                float duration_variation = PWD / pre_PWD;
                if (duration_variation > 3.f || duration_variation < 0.33f) { Serial.printf("Relative PWD error. duration variation is %.2f\n", duration_variation); break; }

                // 10. check amplitude variation validation
                float amplitude_variation = PWA / pre_PWA;
                if (amplitude_variation > 4.f || amplitude_variation < 0.25f) { Serial.printf("Relative amplitude error. amplitude variation is %.2f\n", amplitude_variation); break; }

                // if past all the checks, then these two peaks and valleys are valid
                piece_valid = true;
                break;
            }
        }
        if (piece_valid == true) { 
            pre_PWRT = PWRT; pre_PWD = PWD; pre_PWA = PWA; // store the value into pre features
            valley_locs[*n_vals] = valley_locs[i]; peak_locs[*n_npks] = peak_locs[j]; // the valid locs will be stored
            *n_vals += 1; *n_npks += 1;
            piece_valid = false; // return to false;
        }
    }
    
    Serial.printf("After checking, the number of valid peaks is %d, number of valid valley is %d\n", *n_npks, *n_vals);

}

int32_t spo2_calculation(uint32_t* ir_buffer, uint32_t* red_buffer, int32_t buffer_length, int32_t* valley_locs, int32_t num_val, int32_t* peak_locs, int32_t num_peak, int32_t ratio_size, int32_t* n_i_ratio_count) {
    int32_t* an_ratio = (int32_t*)calloc(ratio_size, sizeof(int32_t)); // don't forget to free
    *n_i_ratio_count = 0; // must initalize with zero first
    for (int32_t k = 0; k < num_val - 1; k++) { // k is valley pointer
        for (int32_t j = 0; j < num_peak - 1; j++) { // j is peak pointer
            // distance of adjacent valleys and peaks should exceed five times of filter_size
            // ensure triangle shape
            if (valley_locs[k + 1] - valley_locs[k] > 5 * filter_size && peak_locs[j + 1] - peak_locs[j] > 5 * filter_size &&
                valley_locs[k] < peak_locs[j] && valley_locs[k + 1] > peak_locs[j] && valley_locs[k + 1] < peak_locs[j + 1]) { 
                int32_t n_x_dc_max_idx = peak_locs[j]; // index of ir peak between adjacent valleys
                int32_t n_y_dc_max_idx = peak_locs[j]; // index of red peak
                int32_t n_x_dc_max = ir_buffer[peak_locs[j]]; // ir peak value
                int32_t n_y_dc_max = red_buffer[peak_locs[j]];// red peak value
                int32_t n_y_ac = (red_buffer[valley_locs[k + 1]] - red_buffer[valley_locs[k]]) * (n_y_dc_max_idx - valley_locs[k]);
                n_y_ac = red_buffer[valley_locs[k]] + n_y_ac / (valley_locs[k + 1] - valley_locs[k]);
                n_y_ac = red_buffer[n_y_dc_max_idx] - n_y_ac;  // subracting linear DC compoenents from raw 
                int32_t n_x_ac = (ir_buffer[valley_locs[k + 1]] - ir_buffer[valley_locs[k]]) * (n_x_dc_max_idx - valley_locs[k]);
                n_x_ac = ir_buffer[valley_locs[k]] + n_x_ac / (valley_locs[k + 1] - valley_locs[k]);
                n_x_ac = ir_buffer[n_x_dc_max_idx] - n_x_ac;  // subracting linear DC compoenents from raw 
                int32_t n_nume = (n_y_ac * n_x_dc_max) >> 7; //formular is (n_y_ac * n_x_dc_max) / ( n_x_ac *n_y_dc_max);
                int32_t n_denom = (n_x_ac * n_y_dc_max) >> 7; //prepare X100 to preserve floating value, 2^7 = 128
                if (n_denom > 0 && *n_i_ratio_count < ratio_size && n_nume != 0) {
                    an_ratio[*n_i_ratio_count] = (n_nume * 100) / n_denom; // multiple by 100
                    *n_i_ratio_count += 1; // ratio count + 1
                }

            }
        }
    }
    if (*n_i_ratio_count == 0) // no ratio found
        return 999;
    maxim_sort_ascend(an_ratio, *n_i_ratio_count); // choose median value since PPG signal may varies from beat to beat
    int32_t n_ratio_median = *n_i_ratio_count % 2 ? an_ratio[(*n_i_ratio_count - 1) / 2] : (an_ratio[*n_i_ratio_count / 2 - 1] + an_ratio[*n_i_ratio_count / 2]) / 2;
    free(an_ratio); an_ratio = NULL;// free the dynamic memoery
    // int32_t int_float_SPO2 = -45.060 * n_ratio_median * n_ratio_median / 10000 + 30.354 * n_ratio_median / 100 + 94.845;
    return (n_ratio_median > 2 && n_ratio_median < 184) ? uch_spo2_table[n_ratio_median] : 999; // must be a valid index for spo2 table
}

int32_t HR_calculation(uint32_t* pun_green_buffer, int32_t buffer_length, int32_t* peak_locs, int32_t* num_peak, int32_t max_num_peak, int32_t* valley_locs, int32_t* num_val, int32_t max_num_valley, int32_t sampling_rate, int32_t* n_peak_interval) {
    int32_t* green_buffer = (int32_t*)calloc(buffer_length, sizeof(int32_t));
    for (int32_t i = 0; i < buffer_length; i++)
        green_buffer[i] = pun_green_buffer[i];
    // preprocess signal
    preprocessing(green_buffer, buffer_length, filter_size);

    int32_t* invertedData = (int32_t*)calloc(buffer_length, sizeof(int32_t));
    for (int32_t k = 0; k < buffer_length; k++)
        invertedData[k] = -1 * green_buffer[k];
    // find peaks and valleys
    AMPD(green_buffer, buffer_length, peak_locs, num_peak, max_num_peak);
    AMPD(invertedData, buffer_length, valley_locs, num_val, max_num_valley);
    /*maxim_peaks_above_min_height(peak_locs, num_peak, green_buffer, buffer_length, 0, max_num_peak);
    maxim_peaks_above_min_height(valley_locs, num_val, invertedData, buffer_length, 0, max_num_valley);*/
    maxim_remove_close_peaks(peak_locs, num_peak, green_buffer, 10*filter_size);
    *num_peak = min(*num_peak, max_num_peak);
    maxim_remove_close_peaks(valley_locs, num_val, invertedData, 10*filter_size);
    *num_val = min(*num_val, max_num_valley);
    Serial.printf("The number of peak is %d\n", *num_peak);
    for (int32_t i = 0; i < *num_peak; i++) {
        Serial.print(peak_locs[i], DEC);
        Serial.print("\t");
    }
    Serial.printf("\nThe number of valley is %d\n", *num_val);
    for (int32_t i = 0; i < *num_val; i++) {
        Serial.print(valley_locs[i], DEC);
        Serial.print("\t");
    }
    Serial.println();
    delay(1000);
     
    // check and remove artifact
    //check_valid(peak_locs, num_peak, valley_locs, num_val, green_buffer, buffer_length, sampling_rate);
    free(invertedData); invertedData = NULL; // release memory on time
    free(green_buffer); green_buffer = NULL; // release memory on time
    
    // calculate HR
    *n_peak_interval = 0;
    if (*num_peak < 2)
        return 999; // invalid

    int32_t num_interval = *num_peak - 1;
    int32_t* peak_interval_arr = (int32_t*)calloc(num_interval, sizeof(int32_t)); // DMA
    for (int32_t k = 0; k < num_interval; k++)
        peak_interval_arr[k] = peak_locs[k + 1] - peak_locs[k];
    maxim_sort_ascend(peak_interval_arr, num_interval); // median (n peaks and n-1 intervals)
    *n_peak_interval = num_interval % 2 ? peak_interval_arr[(num_interval - 1) / 2] : (peak_interval_arr[num_interval / 2] + peak_interval_arr[num_interval / 2 - 1]) / 2;
    free(peak_interval_arr); peak_interval_arr = NULL; // free memory
    return (sampling_rate * 60) / *n_peak_interval;
}

void preprocessing(int32_t* green_buffer, int32_t buffer_length, int32_t filter_size) {
    DC_removing_inverting_filter(green_buffer, buffer_length);
    median_filter(green_buffer, buffer_length, filter_size);
    mean_filter(green_buffer, buffer_length, filter_size);
}

void DC_removing_inverting_filter(int32_t* green_buffer, int32_t buffer_length) {
    int32_t green_DC = 0;
    for (int32_t k = 0; k < buffer_length; k++)
        green_DC += green_buffer[k];
    green_DC /= buffer_length; // calculates DC component
    for (int32_t k = 0; k < buffer_length; k++) // invert signal: y_avg - (y - y_avg) = 2*y_avg - y
        green_buffer[k] = green_DC - green_buffer[k]; // remove DC component: (2*y_avg - y) - y_avg = y_avg - y
}

void median_filter(int32_t* green_buffer, int32_t buffer_length, int32_t filter_size) {
    int32_t* med_filter = (int32_t*)calloc(filter_size, sizeof(int32_t));
    int32_t* med_filter_copy = (int32_t*)calloc(filter_size, sizeof(int32_t));
    int32_t actual_size;
    // N points Moving Median Filter
    for (int32_t k = 0; k < buffer_length; k++) {
        actual_size = k + 1 < filter_size ? k + 1 : filter_size; // the actual size of filter
        med_filter[k % filter_size] = green_buffer[k]; // cover old value with new value
        for (int32_t i = 0; i < actual_size; i++) 
            med_filter_copy[i] = med_filter[i]; // copy filter and avoid changing the original order
        if (k == 0)
            continue; // don't need sort
        maxim_sort_ascend(med_filter_copy, actual_size);
        green_buffer[k] = actual_size % 2 ? med_filter_copy[(actual_size - 1) / 2] : (med_filter_copy[actual_size / 2] + med_filter_copy[actual_size / 2 - 1]) / 2;
    }
    free(med_filter); med_filter = NULL;
    free(med_filter_copy); med_filter_copy = NULL;
}

void mean_filter(int32_t* green_buffer, int32_t buffer_length, int32_t filter_size) {
    int32_t* mea_filter = (int32_t*)calloc(filter_size, sizeof(int32_t));
    int32_t sum = 0; // the sum of filter
    // N points Moving Average Filter
    for (int32_t k = 0; k < buffer_length; k++) {
        sum += green_buffer[k]; // add newest value
        sum -= mea_filter[k % filter_size]; // subtract old value from sum, inital values are all zero
        green_buffer[k] = k + 1 < filter_size ? sum / (k + 1) : sum / filter_size;
        mea_filter[k % filter_size] = green_buffer[k]; // cover old value with new value
    }
    free(mea_filter); mea_filter = NULL;
}

//find both peaks and valleys
//index: the index array
//len_index:the length of index array 
//Automatic Multiscale-based Detection (Based on Largest Scale Magnitude)
void AMPD(int32_t* data, int32_t bufferSize, int32_t* index, int32_t* len_index, int32_t max_num_index) {
    int32_t size = bufferSize;
    int32_t rowNum = size / 2; // the scale array (to find the largest scale magnitude)
    int32_t* p_data = (int32_t*)calloc(size, sizeof(int32_t)); // initalize with size of zeros
    int32_t* arr_rowsum = (int32_t*)calloc(rowNum, sizeof(int32_t)); //initialize with rowNum of zeros
    int32_t min_index, max_window_length;
    for (int32_t k = 1; k < size / 2 + 1; k++) {
        int32_t row_sum = 0; // for scale magnitude = k
        for (int32_t i = k; i < size - k; i++) {
            if ((data[i] > data[i - k]) && (data[i] > data[i + k])) // find the local maximum with an interval of 2*k
                row_sum -= 1;
        }
        *(arr_rowsum + k - 1) = row_sum;
    }
    min_index = argmin(arr_rowsum, rowNum); // find the largest window
    max_window_length = min_index;
    for (int k = 1; k < max_window_length + 1; k++) {
        for (int i = k; i < size - k; i++) { // the original one (int i = 1; i < size - k; i++)
            if ((data[i] > data[i - k]) && (data[i] > data[i + k]))
                p_data[i] += 1;
        }
    }
    *len_index = 0; // must clear firstly
    for (int i_find = 0; i_find < size; i_find++) {
        if (p_data[i_find] == max_window_length && *len_index < max_num_index) { // ensure peak numbers will not exceed the range
            index[*len_index] = i_find;
            *len_index += 1;
        }
    }
    free(p_data); p_data = NULL; // as this poiter is writen in the function, it will not be a wild pointer
    free(arr_rowsum); p_data = NULL;
}

// find the index of minmum value in the given array
int32_t argmin(int32_t* index, int32_t index_len) {
    int32_t min_index = 0;
    int32_t min = index[0];
    for (int32_t i = 1; i < index_len; i++) {
        if (index[i] < min) {
            min = index[i];
            min_index = i;
        }
    }
    return min_index;
}