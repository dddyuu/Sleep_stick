import mne
import numpy as np
from mne.preprocessing import ICA
from scipy.io import savemat
from scipy.io import loadmat
import numpy as np
from scipy.io import loadmat

from data_trainer import get_data_loaders, load_model_weights_predict
from train import train_and_save_model


def preprocess_eeg(file_path, output_path, downsample_freq=250):
    """
    EEG预处理流程:
    1. 读取.set文件
    2. 0.5-45Hz带通滤波
    3. 降采样
    4. REST重参考
    5. ICA去除眼电
    6. 坏段去除

    参数:
        file_path: .set文件路径
        output_path: 预处理后保存路径
        downsample_freq: 降采样频率(Hz)
    """

    # 1. 读取数据
    print("步骤1: 从.mat文件中加载数据")
    mat_data = loadmat(file_path)
    eeg_struct = mat_data['EEG']
    # events, event_dict = mne.events_from_annotations(raw)

    eeg_data = mat_data['EEG']['eeg'][0,0]
    original_sfreq = mat_data['EEG']['srate']
    print(f"原始采样率: {original_sfreq}Hz")

    num_channels = eeg_struct['eeg'][0,0].shape[0] if eeg_struct['eeg'][0,0].shape[0] < eeg_struct['eeg'][0,0].shape[1] else \
    eeg_struct['eeg'][0,0].shape[1]

    ch_names = [f'EEG{i + 1:02d}' for i in range(num_channels)]
    print(f"Using default channel names: {ch_names}")

    # 创建MNE Raw对象
    info = mne.create_info(ch_names=ch_names, sfreq=original_sfreq, ch_types='eeg')
    raw = mne.io.RawArray(eeg_data, info)

    # 加载事件信息(如果存在)
    events = None
    event_dict = {}

    event_struct = eeg_struct['event']

    position = np.zeros(6,)
    position[0,], position[1,], position[2,], position[3,], position[4,], position[5,]  = event_struct[0,0]['latency'][:,0][0][0][0], event_struct[0,0]['latency'][:,0][1][0][0], event_struct[0,0]['latency'][:,0][2][0][0], event_struct[0,0]['latency'][:,0][3][0][0], event_struct[0,0]['latency'][:,0][4][0][0], event_struct[0,0]['latency'][:,0][5][0][0]
    position = position.astype(int)

    types = np.zeros(6,)
    types[0,], types[1,], types[2,], types[3,], types[4,], types[5,] = \
    event_struct[0, 0]['type'][:, 0][0][0][0], event_struct[0, 0]['type'][:, 0][1][0][0], \
    event_struct[0, 0]['type'][:, 0][2][0][0], event_struct[0, 0]['type'][:, 0][3][0][0], \
    event_struct[0, 0]['type'][:, 0][4][0][0], event_struct[0, 0]['type'][:, 0][5][0][0]
    types = types.astype(int)
    events = np.column_stack([position, np.zeros_like(position), types])

    # 创建事件字典
    unique_types = np.unique(types)
    if len(unique_types) > 0 and isinstance(unique_types[0], str):
        event_dict = {t: i + 1 for i, t in enumerate(unique_types)}

    # 2. 滤波 (0.5-45Hz)
    print("\n步骤2: 带通滤波(0.5-45Hz)...")
    raw_filtered = raw.copy().filter(l_freq=0.5, h_freq=45, method='fir', phase='zero-double')
    # raw_filtered.plot(title='滤波后数据', block=False)

    # 3. 降采样
    print("\n步骤3: 降采样...")
    if downsample_freq < original_sfreq:
        ratio = downsample_freq / original_sfreq
        raw_resampled = raw_filtered.copy().resample(sfreq=downsample_freq)

        # 调整事件位置(样本点)
        events_resampled = events.copy()
        print(events[:, 0] * ratio)
        events_resampled[:, 0] = (events[:, 0] * ratio).astype(int)
        print(events_resampled)
        print(f"降采样到 {downsample_freq}Hz, 事件位置已调整")
    else:
        raw_resampled = raw_filtered.copy()
        events_resampled = events.copy()
        print("采样率已低于目标频率，跳过降采样")


    # 5. ICA去除眼电
    print("\n步骤5: ICA去除眼电...")
    ica = ICA(n_components=2, max_iter='auto', random_state=97)
    ica.fit(raw_resampled.copy())

    # 自动检测眼电成分
    eog_indices, eog_scores = ica.find_bads_eog(raw_resampled.copy(), ch_name=['EEG01', 'EEG02'],
                                                threshold=2.0)
    ica.exclude = eog_indices

    # 应用ICA去除眼电
    raw_ica = raw_resampled.copy().copy()
    print("raw_ica---------------------", raw_ica.get_data().shape)

    ica.apply(raw_ica)

    # 将事件转换为MATLAB兼容格式
    mat_events = {
        'positions': events_resampled[:, 0],  # 事件位置(样本点)
        'types': events_resampled[:, 2],  # 事件类型编码
        'latencies': events_resampled[:, 0] / downsample_freq,  # 事件延迟(秒)
        'type_dict': event_dict  # 事件类型字典
    }

    # 7. 准备保存为.mat文件的数据
    print("\n准备.mat文件数据...")
    mat_data = {
        'data': raw_ica.get_data(),
        'sfreq': raw_ica.info['sfreq'],
        'ch_names': raw_ica.info['ch_names'],
        'times': raw_ica.times,
        'events': mat_events,  # 包含所有事件信息
        'annotations': [(ann['onset'], ann['duration'], ann['description'])
                        for ann in raw_ica.annotations],
        'ica_components': ica.get_components() if hasattr(ica, 'get_components') else None,
        'preprocessing_info': {
            'filter': '0.5-45Hz',
            'downsample': downsample_freq,
            'rereference': 'average (REST替代)',
            'ica_method': 'infomax'
        }
    }

    # 保存预处理后的数据
    print("\n保存预处理后的数据...")
    savemat(output_path, mat_data)
    print(f"预处理完成，结果已保存到 {output_path}")


def preprocess_data(data, downsample_freq=250):
    """
    EEG预处理流程:
    1. 读取.set文件
    2. 0.5-45Hz带通滤波
    3. 降采样
    4. REST重参考
    5. ICA去除眼电
    6. 坏段去除

    参数:
        file_path: .set文件路径
        output_path: 预处理后保存路径
        downsample_freq: 降采样频率(Hz)
    """


    eeg_data = data
    original_sfreq = 500
    num_channels = 2

    ch_names = ['EEG01', 'EEG02']
    print(f"Using default channel names: {ch_names}")

    # 创建MNE Raw对象
    info = mne.create_info(ch_names=ch_names, sfreq=original_sfreq, ch_types='eeg')
    raw = mne.io.RawArray(eeg_data, info)

    # 2. 滤波 (0.5-45Hz)
    print("\n步骤2: 带通滤波(0.5-45Hz)...")
    raw_filtered = raw.copy().filter(l_freq=0.5, h_freq=45, method='fir', phase='zero-double')


    # 3. 降采样
    print("\n步骤3: 降采样...")
    if downsample_freq < original_sfreq:
        ratio = downsample_freq / original_sfreq
        raw_resampled = raw_filtered.copy().resample(sfreq=downsample_freq)

    else:
        raw_resampled = raw_filtered.copy()
    # 5. ICA去除眼电
    print("\n步骤5: ICA去除眼电...")
    ica = ICA(n_components=2, max_iter='auto', random_state=97)
    ica.fit(raw_resampled.copy())

    # 自动检测眼电成分
    eog_indices, eog_scores = ica.find_bads_eog(raw_resampled.copy(), ch_name=['EEG01', 'EEG02'],
                                                threshold=2.0)
    ica.exclude = eog_indices

    # 应用ICA去除眼电
    raw_ica = raw_resampled.copy().copy()
    print("raw_ica---------------------", raw_ica.get_data().shape)

    ica.apply(raw_ica)
    return raw_ica.get_data()

def save_to_npy(file_path,output_data_path,output_label_path):

    data1 = loadmat(file_path)["data"].transpose(1,0)
    event_data1 = loadmat(file_path)["events"]["positions"]

    low_data = data1[event_data1[0][0][0][0]:event_data1[0][0][0][1]]
    low_data = low_data[:low_data.shape[0] // 250 * 250]
    low_data = low_data.reshape(-1, 2, 250)
    low_label = np.repeat(0, low_data.shape[0])

    mid_data = data1[event_data1[0][0][0][2]:event_data1[0][0][0][3]]
    mid_data = mid_data[:mid_data.shape[0] // 250 * 250]
    mid_data = mid_data.reshape(-1, 2, 250)
    mid_label = np.repeat(1, mid_data.shape[0])

    high_data = data1[event_data1[0][0][0][4]:event_data1[0][0][0][5]]
    high_data = high_data[:high_data.shape[0] // 250 * 250]
    high_data = high_data.reshape(-1, 2, 250)
    high_label = np.repeat(2, high_data.shape[0])

    data_result = np.concatenate([low_data, mid_data, high_data])
    print(data_result.shape)
    label_result = np.concatenate([low_label, mid_label, high_label])
    print(label_result.shape)

    np.save(output_data_path, data_result)
    np.save(output_label_path, label_result)

# 使用示例
if __name__ == "__main__":
    filename = "gr_0"
    path = "D:/curwork/9_month/liu/"
    input_mat_file = path + filename + ".mat"  
    output_mat_file = path + filename + "_process.mat"
    # # 运行预处理
    datapth = "D:/curwork/9_month/liu/data/"
    labelpth = "D:/curwork/9_month/liu/label/"
    output_model_file = "D:/curwork/9_month/liu/model/" + filename + ".pth"
    output_npy_data = datapth + filename + ".npy"
    output_npy_label = labelpth + filename + ".npy"
    # preprocess_eeg(input_mat_file, output_mat_file, downsample_freq=250)
    # save_to_npy(output_mat_file,output_npy_data, output_npy_label)

    # # 训练并保存模型
    # train_loader, val_loader = get_data_loaders(output_npy_data, output_npy_label, batch_size=128)
    # train_and_save_model(train_loader, val_loader, output_model_file)

    data = np.random.rand(2,500)
    tdata = preprocess_data(data)
    label = load_model_weights_predict(output_model_file,tdata)[0]
    print(label)

    
