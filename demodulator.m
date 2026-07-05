%% MATLAB AM Real-Time Demodulator Script
% Script này thu âm thanh thời gian thực từ Microphone máy tính,
% giải điều chế AM và phát ra loa máy tính ngay lập tức (real-time).

clear; clc; close all;

%% 1. Cấu hình thông số=
% Chọn cấu hình khớp với CONFIG_FREQ_SEL bên STM32:
% 1 : Fs = 12 kHz,  Fc = 3.0 kHz (Mặc định)
% 2 : Fs = 24 kHz,  Fc = 6.0 kHz
% 3 : Fs = 67.2 kHz, Fc = 16.8 kHz
config_freq_sel = 1;

fs = 48000;         % Tần số lấy mẫu của Card âm thanh máy tính (Hz)

if config_freq_sel == 1
    fc = 3000.00;
elseif config_freq_sel == 2
    fc = 6000.00;
elseif config_freq_sel == 3
    fc = 16800.00;
else
    error('Cấu hình config_freq_sel không hợp lệ!');
end

%% 2. Thiết kế Bộ lọc Thông Băng (Band-Pass Filter) 300 Hz - 3000 Hz
% Thiết kế bộ lọc Butterworth bậc 6, dải thông từ 300 Hz đến 3000 Hz
% Bộ lọc này loại bỏ hài tần số cao và triệt tiêu hoàn toàn DC offset.
[b, a] = butter(6, [300, 3000] / (fs/2), 'bandpass');

% Khởi tạo trạng thái bộ lọc
zi = zeros(max(length(a), length(b)) - 1, 1);

fprintf('--- ĐANG KHỞI TẠO BỘ GIẢI ĐIỀU CHẾ AM REAL-TIME ---\n');

% Kiểm tra xem có sẵn Audio Toolbox (audioDeviceReader) hay không
use_toolbox = exist('audioDeviceReader', 'class') == 8;

if use_toolbox
    try
        % Cấu hình các System Object của Audio Toolbox để tối ưu độ trễ thấp
        frame_size = 1024;
        reader = audioDeviceReader('SampleRate', fs, 'SamplesPerFrame', frame_size);
        writer = audioDeviceWriter('SampleRate', fs);
        
        fprintf('Sử dụng Audio Toolbox (Độ trễ thấp < 50ms).\n');
        fprintf('Nhấn giữ nút PTT trên STM32 và nói vào Mic.\n');
        fprintf('Nhấn Ctrl+C để dừng...\n\n');
        
        sample_idx = 0;
        while true
            r_block = reader();
            
            % Giải điều chế với sóng mang có pha liên tục
            n_samples = length(r_block);
            t = (sample_idx : sample_idx + n_samples - 1)' / fs;
            carrier = cos(2 * pi * fc * t);
            sample_idx = sample_idx + n_samples;
            
            x_mixed = r_block .* carrier;
            
            % Lọc thông băng giữ trạng thái
            [audio_out_block, zi] = filter(b, a, x_mixed, zi);
            
            % Tăng âm lượng kỹ thuật số (gain) và giới hạn biên độ
            audio_out_block = audio_out_block * 5.0;
            audio_out_block(audio_out_block > 0.99) = 0.99;
            audio_out_block(audio_out_block < -0.99) = -0.99;
            
            % Phát trực tiếp ra loa
            writer(audio_out_block);
        end
    catch ME
        fprintf('\nDừng thu âm realtime (Audio Toolbox).\n');
        if exist('reader', 'var'); release(reader); end
        if exist('writer', 'var'); release(writer); end
    end
else
    % Fallback sử dụng audiorecorder chuẩn của MATLAB (Không cần toolbox)
    fprintf('Không tìm thấy Audio Toolbox. Sử dụng bộ thu âm chuẩn (Độ trễ ~ 200ms).\n');
    fprintf('Nhấn giữ nút PTT trên STM32 và nói vào Mic.\n');
    fprintf('Nhấn Ctrl+C để dừng...\n\n');
    
    recObj = audiorecorder(fs, 16, 1);
    record(recObj); % Bắt đầu thu âm không đồng bộ ở nền
    
    processed_idx = 0;
    % Kích thước block xử lý: 200 ms (9600 samples ở 48kHz)
    block_samples = 9600; 
    
    try
        while true
            total_samples = recObj.TotalSamples;
            available_samples = total_samples - processed_idx;
            
            if available_samples >= block_samples
                % Lấy mẫu mới từ buffer
                all_data = getaudiodata(recObj);
                r_chunk = all_data(processed_idx + 1 : total_samples);
                processed_idx = total_samples;
                
                % Giải điều chế với sóng mang pha liên tục
                t = (processed_idx - length(r_chunk) : processed_idx - 1)' / fs;
                carrier = cos(2 * pi * fc * t);
                x_mixed = r_chunk .* carrier;
                
                % Lọc thông băng giữ trạng thái
                [audio_out_chunk, zi] = filter(b, a, x_mixed, zi);
                
                % Tăng âm lượng và giới hạn biên độ
                audio_out_chunk = audio_out_chunk * 5.0;
                audio_out_chunk(audio_out_chunk > 0.99) = 0.99;
                audio_out_chunk(audio_out_chunk < -0.99) = -0.99;
                
                % Phát block âm thanh và chặn (block) để tránh chồng lấp âm thanh
                p = audioplayer(audio_out_chunk, fs);
                playblocking(p);
            else
                % Chờ một chút nếu chưa đủ mẫu
                pause(0.05);
            end
        end
    catch ME
        fprintf('\nDừng thu âm realtime.\n');
        stop(recObj);
    end
end
