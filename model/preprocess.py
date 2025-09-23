import mne
from scipy.io import savemat
import numpy as np
from scipy.io import loadmat
from sklearn import preprocessing



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
    position[0,], position[1,], position[2,], position[3,]  = event_struct[0,0]['latency'][:,0][0][0][0], event_struct[0,0]['latency'][:,0][1][0][0], event_struct[0,0]['latency'][:,0][2][0][0], event_struct[0,0]['latency'][:,0][3][0][0],
    position = position.astype(int)

    types = np.zeros(6,)
    types[0,], types[1,], types[2,], types[3,] = \
    event_struct[0, 0]['type'][:, 0][0][0][0], event_struct[0, 0]['type'][:, 0][1][0][0], \
    event_struct[0, 0]['type'][:, 0][2][0][0], event_struct[0, 0]['type'][:, 0][3][0][0],
    types = types.astype(int)
    events = np.column_stack([position, np.zeros_like(position), types])

    # 创建事件字典
    unique_types = np.unique(types)
    if len(unique_types) > 0 and isinstance(unique_types[0], str):
        event_dict = {t: i + 1 for i, t in enumerate(unique_types)}
    print("不进行滤波")
    # # 2. 滤波 (0.5-45Hz)
    # print("\n步骤2: 带通滤波(0.5-45Hz)...")
    # raw_filtered = raw.copy().filter(l_freq=0.5, h_freq=45, method='fir', phase='zero-double')
    # # raw_filtered.plot(title='滤波后数据', block=False)
    #
    # # 3. 降采样
    # print("\n步骤3: 降采样...")
    # if downsample_freq < original_sfreq:
    #     ratio = downsample_freq / original_sfreq
    #     raw_resampled = raw_filtered.copy().resample(sfreq=downsample_freq)
    #
    #     # 调整事件位置(样本点)
    #     events_resampled = events.copy()
    #     print(events[:, 0] * ratio)
    #     events_resampled[:, 0] = (events[:, 0] * ratio).astype(int)
    #     print(events_resampled)
    #     print(f"降采样到 {downsample_freq}Hz, 事件位置已调整")
    # else:
    #     raw_resampled = raw_filtered.copy()
    #     events_resampled = events.copy()
    #     print("采样率已低于目标频率，跳过降采样")

    print("不进行ICA")
    # # 5. ICA去除眼电
    # print("\n步骤5: ICA去除眼电...")
    # ica = ICA(n_components=2, max_iter='auto', random_state=97)
    # ica.fit(raw_resampled.copy())
    #
    # # 自动检测眼电成分
    # eog_indices, eog_scores = ica.find_bads_eog(raw_resampled.copy(), ch_name=['EEG01', 'EEG02'],
    #                                             threshold=2.0)
    # ica.exclude = eog_indices
    #
    # # 应用ICA去除眼电
    # raw_ica = raw_resampled.copy().copy()
    # print("raw_ica---------------------", raw_ica.get_data().shape)
    #
    # ica.apply(raw_ica)

    # 将事件转换为MATLAB兼容格式
    # mat_events = {
    #     'positions': events_resampled[:, 0],  # 事件位置(样本点)
    #     'types': events_resampled[:, 2],  # 事件类型编码
    #     'latencies': events_resampled[:, 0] / downsample_freq,  # 事件延迟(秒)
    #     'type_dict': event_dict  # 事件类型字典
    # }
    print("原始数据重新保存以对应")
    mat_events = {
        'positions': events[:, 0],  # 事件位置(样本点)
        'types': events[:, 2],  # 事件类型编码
        'latencies': events[:, 0] / downsample_freq,  # 事件延迟(秒)
        'type_dict': event_dict  # 事件类型字典
    }
    # 7. 准备保存为.mat文件的数据
    print("\n准备.mat文件数据...")
    # mat_data = {
    #     'data': raw_ica.get_data(),
    #     'sfreq': raw_ica.info['sfreq'],
    #     'ch_names': raw_ica.info['ch_names'],
    #     'times': raw_ica.times,
    #     'events': mat_events,  # 包含所有事件信息
    #     'annotations': [(ann['onset'], ann['duration'], ann['description'])
    #                     for ann in raw_ica.annotations],
    #     'ica_components': ica.get_components() if hasattr(ica, 'get_components') else None,
    #     'preprocessing_info': {
    #         'filter': '0.5-45Hz',
    #         'downsample': downsample_freq,
    #         'rereference': 'average (REST替代)',
    #         'ica_method': 'infomax'
    #     }
    # }

    mat_data = {
        'data': raw.get_data(),
        'sfreq': raw.info['sfreq'],
        'ch_names': raw.info['ch_names'],
        'times': raw.times,
        'events': mat_events,  # 包含所有事件信息
        'annotations': [(ann['onset'], ann['duration'], ann['description'])
                        for ann in raw.annotations],
        # 'ica_components': ica.get_components() if hasattr(ica, 'get_components') else None,
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
    print("\n步骤2: 不带通滤波(0.5-45Hz)...")
    # raw_filtered = raw.copy().filter(l_freq=0.5, h_freq=45, method='fir', phase='zero-double')


    # 3. 降采样
    print("\n步骤3: 不降采样...")
    # if downsample_freq < original_sfreq:
    #     ratio = downsample_freq / original_sfreq
    #     raw_resampled = raw_filtered.copy().resample(sfreq=downsample_freq)
    #
    # else:
    #     raw_resampled = raw_filtered.copy()
    # 5. ICA去除眼电
    # print("\n步骤5: 不ICA去除眼电...")
    # ica = ICA(n_components=2, max_iter='auto', random_state=97)
    # ica.fit(raw_resampled.copy())

    # # 自动检测眼电成分
    # eog_indices, eog_scores = ica.find_bads_eog(raw_resampled.copy(), ch_name=['EEG01', 'EEG02'],
    #                                             threshold=2.0)
    # ica.exclude = eog_indices
    #
    # # 应用ICA去除眼电
    # raw_ica = raw_resampled.copy().copy()
    # print("raw_ica---------------------", raw_ica.get_data().shape)

    # ica.apply(raw_ica)
    return raw.get_data()

def save_to_train_npy(file_path,output_data_path,output_label_path,scaler):

    data1 = loadmat(file_path)["data"].transpose(1,0)
    event_data1 = loadmat(file_path)["events"]["positions"]
    print(data1.shape)
    scaler.fit(data1)
    data1 = scaler.transform(data1)
    low_data = data1[event_data1[0][0][0][0]:event_data1[0][0][0][1]]
    low_data = low_data[:low_data.shape[0] // 500 * 500]
    low_data = low_data.reshape(-1, 2, 500)
    low_label = np.repeat(0, low_data.shape[0])

    mid_data = data1[event_data1[0][0][0][1]:event_data1[0][0][0][2]]
    mid_data = mid_data[:mid_data.shape[0] // 500 * 500]
    mid_data = mid_data.reshape(-1, 2, 500)
    mid_label = np.repeat(1, mid_data.shape[0])

    high_data = data1[event_data1[0][0][0][2]:event_data1[0][0][0][3]]
    high_data = high_data[:high_data.shape[0] // 500 * 500]
    high_data = high_data.reshape(-1, 2, 500)
    high_label = np.repeat(2, high_data.shape[0])

    data_result = np.concatenate([low_data, mid_data, high_data])
    print(data_result.shape)
    label_result = np.concatenate([low_label, mid_label, high_label])
    print(label_result.shape)

    np.save(output_data_path, data_result)
    np.save(output_label_path, label_result)
    return scaler
def save_to_test_npy(file_path,output_data_path,output_label_path):

    data1 = loadmat(file_path)["data"].transpose(1,0)
    event_data1 = loadmat(file_path)["events"]["positions"]
    # data1 = scaler.transform(data1)
    scaler = preprocessing.StandardScaler()
    scaler.fit(data1)
    data1 = scaler.transform(data1)
    low_data = data1[event_data1[0][0][0][0]:event_data1[0][0][0][1]]
    low_data = low_data[:low_data.shape[0] // 500 * 500]
    low_data = low_data.reshape(-1, 2, 500)
    low_label = np.repeat(0, low_data.shape[0])

    mid_data = data1[event_data1[0][0][0][1]:event_data1[0][0][0][2]]
    mid_data = mid_data[:mid_data.shape[0] // 500 * 500]
    mid_data = mid_data.reshape(-1, 2, 500)
    mid_label = np.repeat(1, mid_data.shape[0])

    high_data = data1[event_data1[0][0][0][2]:event_data1[0][0][0][3]]
    high_data = high_data[:high_data.shape[0] // 500 * 500]
    high_data = high_data.reshape(-1, 2, 500)
    high_label = np.repeat(2, high_data.shape[0])

    data_result = np.concatenate([low_data, mid_data, high_data])
    print(data_result.shape)
    label_result = np.concatenate([low_label, mid_label, high_label])
    print(label_result.shape)


    np.save(output_data_path, data_result)
    np.save(output_label_path, label_result)
# 使用示例
if __name__ == "__main__":
    # filename = "grxx"
    # path = "D:/SubEEG/"
    # input_mat_file = path + filename + "_1.mat"
    # output_mat_file = path + filename + "_process.mat"
    # # # 运行预处理
    # datapth = "D:/SubEEG/data/"
    # labelpth = "D:/SubEEG/label/"
    # output_model_file = "D:/SubEEG/model/" + filename + ".pth"
    # output_npy_data = datapth + filename + ".npy"
    # output_npy_label = labelpth + filename + ".npy"
    # # preprocess_eeg(input_mat_file, output_mat_file, downsample_freq=250)
    # # save_to_npy(output_mat_file,output_npy_data, output_npy_label)
    # data = np.load(output_npy_data)
    # label = np.load(output_npy_label)
    # print(len(data))
    # # # 训练并保存模型
    #
    # # train_loader, val_loader = get_data_loaders(output_npy_data, output_npy_label, batch_size=128)
    # # train_and_save_model(train_loader, val_loader, output_model_file)
    # totol = len(data)
    # plabelist = []
    # correct = 0
    # for i in range(len(data)):
    #     tdata = data[i]
    #     tlabel = label[i]
    #     Plabel = load_model_weights_predict(output_model_file, tdata)[0]
    #     plabelist.append(Plabel)
    #     if tlabel == Plabel:
    #         correct+=1
    # acc = correct / totol
    # print(acc)
    # # data = np.random.rand(2, 500)
    # tdata = preprocess_data(data)
    # label = load_model_weights_predict(output_model_file,tdata)[0]
    # # print(label)
    npy_path_mat = "grgg_0.mat"
    npy_mat = "grgg_0_process.mat"
    preprocess_eeg(npy_path_mat,npy_mat)
    train_npy_data_path = "data/grgg_0.npy"
    train_npy_label = "label/grgg_0.npy"

    # test_npy_data_path = "D:/SubEEG/data/lh.npy"
    scaler = preprocessing.StandardScaler()
    scaler = save_to_train_npy(npy_mat,train_npy_data_path,train_npy_label,scaler)
    npy_path_mat = "grdd_0.mat"
    npy_mat = "grdd_0_process.mat"
    preprocess_eeg(npy_path_mat,npy_mat)
    test_npy_data_path = "data/grdd_0.npy"
    test_npy_label = "label/grdd_0.npy"
    save_to_test_npy(npy_mat,test_npy_data_path,test_npy_label,scaler)
    

