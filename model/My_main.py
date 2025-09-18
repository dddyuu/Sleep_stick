import socket
import struct
import os
import threading
import time
import logging
from datetime import datetime
import traceback
import numpy as np
from scipy.io import savemat
from scipy.io import loadmat
from data_trainer import *
# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('tcp_receiver.log'),
        logging.StreamHandler()
    ]
)

# 导入您的处理模块
try:
    from data_trainer import get_data_loaders, load_model_weights_predict
    from preprocess import preprocess_eeg, save_to_train_npy, preprocess_data, save_to_test_npy
    from train import train_and_save_model

    MODULES_AVAILABLE = True
except ImportError as e:
    logging.warning(f"导入处理模块失败: {e}")
    MODULES_AVAILABLE = False

# 路径配置
PATH_CONFIG = {
    'base': "D:/subEEG/",
    'data': "D:/subEEG/data/",
    'label': "D:/subEEG/label/",
    'model': "D:/subEEG/model/"
}


class EEGDataReceiver:
    def __init__(self, host='127.0.0.1', port=8888):
        self.host = host
        self.port = port
        self.socket = None
        self.client_socket = None
        self.receiving_data = False
        self.running = True
        self.packet_count = 0
        self.buffer = b''
        self.processing_threads = []  # 跟踪后台处理线程

        # 计数器和标志
        self.event_51_count = 1
        self.current_model_path = "D:/SubEEG/model/lh.pth"
        # self.current_model_path = ""
        self.first_training_completed = True

        # 实时分类缓冲区（仅用于第二次及之后，固定2通道）
        self.realtime_buffer = [[], []]  # 固定2通道
        self.target_samples = 750

        # 创建必要的目录
        self._ensure_directories()

    def _ensure_directories(self):
        """确保所有必要的目录存在"""
        for path in PATH_CONFIG.values():
            try:
                os.makedirs(path, exist_ok=True)
                logging.debug(f"目录确认存在: {path}")
            except Exception as e:
                logging.warning(f"创建目录失败 {path}: {e}")

    def send_label_to_cpp(self, label_value):
        """发送标签值回C++"""
        try:
            if not self.client_socket:
                return False

            label_packet = struct.pack('>i', int(label_value))
            bytes_sent = self.client_socket.send(label_packet)
            logging.info(f"★ 发送分类结果到C++: {label_value}")
            return bytes_sent == 4
        except Exception as e:
            logging.error(f"发送标签失败: {e}")
            return False

    def send_2label_to_cpp(self, label_value):
        """发送标签值回C++"""
        try:
            if not self.client_socket:
                return False

            # 处理列表格式的标签
            if isinstance(label_value, (list, tuple, np.ndarray)):
                # 确保标签值为整数列表
                label_list = [int(x) for x in label_value]

                # 打包格式：先发送列表长度，然后发送每个元素
                # 使用大端序格式
                length_packet = struct.pack('>i', len(label_list))
                data_packet = struct.pack('>' + 'B' * len(label_list), *label_list)

                # 组合数据包
                full_packet = length_packet + data_packet

                bytes_sent = self.client_socket.send(full_packet)
                logging.info(f"★ 发送分类结果到C++: {label_list}")
                return bytes_sent == len(full_packet)
            else:
                # 保持原有的单个整数发送方式（向后兼容）
                label_packet = struct.pack('>i', int(label_value))
                bytes_sent = self.client_socket.send(label_packet)
                logging.info(f"★ 发送分类结果到C++: {label_value}")
                return bytes_sent == 4

        except Exception as e:
            logging.error(f"发送标签失败: {e}")
            return False
    def process_realtime_classification(self, eeg_data):
        """处理实时EEG数据进行分类 - 确保正确处理2通道数据"""
        try:
            # 只有在第二次及之后的事件51且模型可用时才进行实时分类
            if self.event_51_count < 2:
                return  # 第一次事件51不进行实时分类

            if not self.first_training_completed or not self.current_model_path:
                return

            if not os.path.exists(self.current_model_path):
                logging.warning(f"模型文件不存在: {self.current_model_path}")
                return

            # 确保接收到的数据是2通道格式
            if len(eeg_data) == 0:
                return

            # 检查数据格式和通道数
            logging.debug(
                f"接收到EEG数据 - 通道数: {len(eeg_data)}, 每通道样本数: {len(eeg_data[0]) if eeg_data else 0}")

            # 确保有2个通道的数据
            if len(eeg_data) < 2:
                logging.warning(f"数据通道数不足: {len(eeg_data)}, 期望2个通道")
                # 如果只有1个通道，复制为第2个通道（临时解决方案）
                if len(eeg_data) == 1:
                    eeg_data = [eeg_data[0], eeg_data[0].copy()]
                    logging.info("数据通道扩展为2通道")
                else:
                    return

            # 只使用前2个通道
            eeg_data = eeg_data[:2]

            # 将新数据添加到实时缓冲区
            if len(eeg_data[0]) > 0:
                # 确保缓冲区是2通道
                if len(self.realtime_buffer) != 2:
                    self.realtime_buffer = [[], []]

                # 添加新数据到缓冲区（只使用前2个通道）
                for ch in range(2):
                    if ch < len(eeg_data):
                        self.realtime_buffer[ch].extend(eeg_data[ch])
                    else:
                        # 如果数据不足2通道，用第一个通道的数据填充
                        self.realtime_buffer[ch].extend(eeg_data[0])

                # 检查是否达到目标采样点数
                if len(self.realtime_buffer[0]) >= self.target_samples and len(
                        self.realtime_buffer[1]) >= self.target_samples:
                    # 提取最新的500个采样点 - 确保是2通道
                    data_for_classification = np.zeros((2, 2, 500))
                    logging.info(f"实时分类 - 数据形状: {data_for_classification.shape}")
                    # 修改数据形状为(2, 2, 500)，总共1000个样本点
                    # 修改数据形状为(2, 2, 500)，第一维度2为batch size，第二维度2为通道
                    for batch_idx in range(2):  # 遍历batch
                        for ch in range(2):  # 遍历通道
                            # 计算当前batch和通道对应的起始位置
                            overlap_samples = 250  # 重叠的采样点数
                            samples_per_batch = 500  # 每个batch的采样点数
                            start_idx = batch_idx * overlap_samples
                            end_idx = start_idx + samples_per_batch

                            # 取对应的采样点
                            if len(self.realtime_buffer[ch]) >= end_idx:
                                channel_data = self.realtime_buffer[ch][start_idx:end_idx]
                                data_for_classification[batch_idx, ch, :] = channel_data
                            else:
                                # 如果数据不足，用零填充
                                available_data = self.realtime_buffer[ch][start_idx:]
                                if len(available_data) > 0:
                                    data_for_classification[batch_idx, ch, :len(available_data)] = available_data
                                    # 剩余部分保持为零（如果初始化为零）
                                # 如果没有可用数据，该位置保持为零

                    logging.info(f"实时分类 - 数据形状: {data_for_classification.shape}")
                    # for ch in range(2):
                    #     # 取最后500个采样点
                    #     if len(self.realtime_buffer[ch]) >= self.target_samples:
                    #         channel_data = self.realtime_buffer[ch][-self.target_samples:]
                    #         data_for_classification[ch] = channel_data
                    #     else:
                    #         # 如果数据不足，用零填充
                    #         available_data = self.realtime_buffer[ch]
                    #         if len(available_data) > 0:
                    #             data_for_classification[ch][:len(available_data)] = available_data
                    #
                    # logging.info(f"实时分类 - 数据形状: {data_for_classification.shape}")

                    # 验证数据形状
                    if data_for_classification.shape[1] != 2:
                        logging.error(f"数据通道数错误: {data_for_classification.shape[0]}, 期望2")
                        return

                    # 预处理数据
                    try:
                        # preprocessed_data = preprocess_data(data_for_classification)
                        # logging.info(f"不预处理后数据形状: {preprocessed_data.shape}")

                        # 进行分类预测
                        # logging.info(f"不进行预处理-数据形状: {preprocessed_data.shape}")
                        # label = load_model_weights_predict(self.current_model_path, preprocessed_data)[0]
                        scaler = preprocessing.StandardScaler()
                        train_npy_mat = "D:/SubEEG/lhxl_process.mat"
                        # test_npy_label = "D:/SubEEG/label/grr.npy"
                        # model_path = "D:/SubEEG/model/grr.pth"
                        train_npy_data_path = "D:/SubEEG/data/lhxl.npy"
                        # test_npy_data_path = "D:/SubEEG/data/grr.npy"
                        train_npy_label = "D:/SubEEG/label/lhxl.npy"
                        scaler = save_to_train_npy(train_npy_mat, train_npy_data_path, train_npy_label, scaler)
                        label = pro_load_model_weights_predict(self.current_model_path, data_for_classification, scaler)
                        # label = load_model_weights_predict(self.current_model_path, preprocessed_data)[0]

                        logging.info(f"★ 实时分类结果: {label} (模型: {os.path.basename(self.current_model_path)})")

                        # 发送标签到C++
                        self.send_2label_to_cpp(label)

                    except Exception as e:
                        logging.error(f"实时分类处理失败: {e}")
                        logging.debug(traceback.format_exc())

                    # 保持缓冲区大小，移除旧数据（保留250个采样点的重叠）
                    overlap = 250
                    for ch in range(2):
                        if len(self.realtime_buffer[ch]) > overlap:
                            self.realtime_buffer[ch] = self.realtime_buffer[ch][-overlap:]

        except Exception as e:
            logging.error(f"实时分类异常: {e}")
            logging.debug(traceback.format_exc())

    def parse_data_packet(self, buffer):
        """解析数据包 - 确保正确处理2通道数据"""
        try:
            offset = 0
            if len(buffer) < 16:
                return None

            # 解析包头
            identifier = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4
            if identifier != 0x12345678:
                return None

            event_type = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            # 解析文件名
            if len(buffer) < offset + 4:
                return None
            filename_length = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            if filename_length > 1024:
                logging.warning(f"文件名长度异常: {filename_length}")
                return None
            if len(buffer) < offset + filename_length:
                return None

            filename = ""
            if filename_length > 0:
                try:
                    filename_bytes = buffer[offset:offset + filename_length]
                    for encoding in ['utf-16be', 'utf-8', 'latin1']:
                        try:
                            filename = filename_bytes.decode(encoding, errors='ignore')
                            break
                        except:
                            continue
                except Exception as e:
                    logging.debug(f"文件名解码失败: {e}")
                    filename = "unknown"
            offset += filename_length

            # 解析通道数和采样点数
            if len(buffer) < offset + 8:
                return None
            channel_count = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4
            sample_count = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            # 验证通道数
            logging.debug(f"解析数据包 - 通道数: {channel_count}, 采样点数: {sample_count}")

            if channel_count > 100 or sample_count > 100000:
                logging.warning(f"数据维度异常: channels={channel_count}, samples={sample_count}")
                return None

            # 解析EEG数据
            data_size = channel_count * sample_count * 8
            if len(buffer) < offset + data_size:
                return None

            eeg_data = []
            if data_size > 0 and data_size <= len(buffer) - offset:
                try:
                    num_doubles = channel_count * sample_count
                    doubles = struct.unpack(f'>{num_doubles}d', buffer[offset:offset + data_size])

                    for ch in range(channel_count):
                        channel_data = []
                        for sample in range(sample_count):
                            index = ch * sample_count + sample
                            if index < len(doubles):
                                channel_data.append(doubles[index])
                        if channel_data:
                            eeg_data.append(channel_data)

                    # 确保有2个通道，如果不足则补充
                    while len(eeg_data) < 2:
                        if len(eeg_data) > 0:
                            # 复制第一个通道的数据
                            eeg_data.append(eeg_data[0].copy())
                            logging.debug("复制通道数据以达到2通道要求")
                        else:
                            # 创建零数据
                            eeg_data.append([0.0] * sample_count)
                            logging.debug("创建零数据以达到2通道要求")

                    # 只保留前2个通道
                    eeg_data = eeg_data[:2]

                except struct.error as e:
                    logging.debug(f"EEG数据解析失败: {e}")
                    eeg_data = []

            offset += data_size

            return {
                'type': 'data',
                'identifier': identifier,
                'event_type': event_type,
                'filename': filename,
                'channel_count': len(eeg_data),
                'sample_count': len(eeg_data[0]) if eeg_data else 0,
                'eeg_data': eeg_data,
                'total_size': offset
            }
        except Exception as e:
            logging.debug(f"数据包解析异常: {e}")
            return None

    def parse_event_notification(self, buffer):
        """解析事件通知包 - 保持原有逻辑"""
        try:
            offset = 0
            if len(buffer) < 16:
                return None

            identifier = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4
            if identifier != 0x87654321:
                return None

            event_type = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            if len(buffer) < offset + 4:
                return None
            filename_length = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            if filename_length > 1024:
                logging.warning(f"事件通知文件名长度异常: {filename_length}")
                return None
            if len(buffer) < offset + filename_length:
                return None

            filename = ""
            if filename_length > 0:
                try:
                    filename_bytes = buffer[offset:offset + filename_length]
                    for encoding in ['utf-16be', 'utf-8', 'latin1']:
                        try:
                            filename = filename_bytes.decode(encoding, errors='ignore')
                            break
                        except:
                            continue
                except Exception as e:
                    logging.debug(f"事件通知文件名解码失败: {e}")
                    filename = "unknown"
            offset += filename_length

            if len(buffer) < offset + 16:
                return None
            sample_count = struct.unpack('>Q', buffer[offset:offset + 8])[0]
            offset += 8
            timestamp = struct.unpack('>q', buffer[offset:offset + 8])[0]
            offset += 8

            return {
                'type': 'event',
                'identifier': identifier,
                'event_type': event_type,
                'filename': filename,
                'sample_count': sample_count,
                'timestamp': timestamp,
                'total_size': offset
            }
        except Exception as e:
            logging.debug(f"事件通知解析异常: {e}")
            return None

    def parse_raw_data(self, data):
        """解析原始double数据 - 确保正确处理2通道数据"""
        try:
            num_doubles = len(data) // 8
            remainder = len(data) % 8

            if remainder != 0:
                logging.debug(f"原始数据长度不是8的倍数，剩余 {remainder} 字节")
                if not self.receiving_data:
                    return None

            if num_doubles > 0:
                try:
                    doubles = struct.unpack(f'>{num_doubles}d', data[:num_doubles * 8])
                except struct.error as e:
                    logging.debug(f"原始数据解析失败: {e}")
                    return None

                # 确保是2通道数据
                channels = 2
                samples_per_channel = num_doubles // channels

                if num_doubles % channels == 0 and samples_per_channel > 0:
                    channels_data = []
                    try:
                        for ch in range(channels):
                            channel_data = []
                            for sample in range(samples_per_channel):
                                index = ch * samples_per_channel + sample
                                if index < len(doubles):
                                    channel_data.append(doubles[index])
                            if channel_data:
                                channels_data.append(channel_data)

                        # 确保有2个通道
                        if len(channels_data) != 2:
                            logging.warning(f"原始数据通道数不正确: {len(channels_data)}, 期望2")
                            # 如果只有1个通道，复制它
                            if len(channels_data) == 1:
                                channels_data.append(channels_data[0].copy())
                            else:
                                return None

                        return {
                            'type': 'raw',
                            'num_doubles': num_doubles,
                            'channels_data': channels_data,
                            'raw_doubles': doubles
                        }
                    except Exception as e:
                        logging.debug(f"原始数据重组失败: {e}")
                        return None
                else:
                    logging.debug(f"原始数据无法平均分配到2通道: {num_doubles} doubles")
                    # 尝试其他分配方式
                    if num_doubles >= 2:
                        # 如果数据点不能被2整除，取前面能被2整除的部分
                        usable_doubles = (num_doubles // 2) * 2
                        channels_data = [[], []]
                        for i in range(0, usable_doubles, 2):
                            channels_data[0].append(doubles[i])
                            channels_data[1].append(doubles[i + 1])

                        if len(channels_data[0]) > 0:
                            return {
                                'type': 'raw',
                                'num_doubles': usable_doubles,
                                'channels_data': channels_data,
                                'raw_doubles': doubles[:usable_doubles]
                            }
                    return None

        except Exception as e:
            logging.debug(f"原始数据解析异常: {e}")

        return None

    def process_event_54(self, filename):
        """处理事件54 - 完全保持原有逻辑"""

        def safe_background_process():
            thread_id = threading.current_thread().ident
            scaler = preprocessing.StandardScaler()
            try:
                if not filename:
                    logging.warning(f"[线程{thread_id}] 事件54: 文件名为空，跳过处理")
                    return

                # 第二次及之后的事件54不进行训练
                if self.event_51_count >= 2:
                    logging.info(f"[线程{thread_id}] 第{self.event_51_count}次事件54: 跳过训练")
                    return

                # 第一次事件54 - 完全使用原有训练逻辑
                base_filename = os.path.basename(filename)
                filename_without_ext = os.path.splitext(base_filename)[0]
                logging.info(f"[线程{thread_id}] 开始处理文件: {filename_without_ext}")

                input_mat_file = os.path.join(PATH_CONFIG['base'], f"{filename_without_ext}_0.mat")
                output_mat_file = os.path.join(PATH_CONFIG['base'], f"{filename_without_ext}_process.mat")
                output_npy_data = os.path.join(PATH_CONFIG['data'], f"{filename_without_ext}.npy")
                output_npy_label = os.path.join(PATH_CONFIG['label'], f"{filename_without_ext}.npy")
                output_model_file = os.path.join(PATH_CONFIG['model'], f"{filename_without_ext}.pth")

                if not MODULES_AVAILABLE:
                    logging.error(f"[线程{thread_id}] 处理模块不可用，跳过文件处理")
                    return

                if not os.path.exists(input_mat_file):
                    logging.warning(f"[线程{thread_id}] 输入文件不存在: {input_mat_file}")
                    for i in range(5):
                        time.sleep(1)
                        if os.path.exists(input_mat_file):
                            logging.info(f"[线程{thread_id}] 文件出现，继续处理")
                            break
                    else:
                        logging.error(f"[线程{thread_id}] 等待超时，文件仍不存在: {input_mat_file}")
                        return

                # 步骤1: 预处理EEG数据 - 保持原有
                try:
                    logging.info(f"[线程{thread_id}] 步骤1: 预处理EEG数据")
                    print("\n准备.mat文件数据...")

                    preprocess_eeg(input_mat_file, output_mat_file, downsample_freq=250)

                    logging.info(f"[线程{thread_id}] 步骤1完成: EEG预处理")
                except Exception as e:
                    logging.error(f"[线程{thread_id}] 步骤1失败 - EEG预处理错误: {e}")
                    logging.debug(f"[线程{thread_id}] 预处理错误详情:\n{traceback.format_exc()}")
                    return

                if not os.path.exists(output_mat_file):
                    logging.error(f"[线程{thread_id}] 预处理输出文件不存在: {output_mat_file}")
                    return

                # 步骤2: 转换为numpy格式 - 保持原有
                try:
                    logging.info(f"[线程{thread_id}] 步骤2: 转换为numpy格式")



                    logging.info(f"[线程{thread_id}] 步骤2完成: numpy转换")
                except Exception as e:
                    logging.error(f"[线程{thread_id}] 步骤2失败 - numpy转换错误: {e}")
                    logging.debug(f"[线程{thread_id}] numpy转换错误详情:\n{traceback.format_exc()}")
                    return
                #
                # if not os.path.exists(output_npy_data) or not os.path.exists(output_npy_label):
                #     logging.error(f"[线程{thread_id}] numpy文件生成失败")
                #     return

                # 步骤3: 训练模型 - 保持原有
                try:
                    logging.info(f"[线程{thread_id}] 步骤3: 训练模型")
                    # train_loader, val_loader = get_data_loaders(output_npy_data, output_npy_label, batch_size=128)
                    # train_and_save_model(train_loader, val_loader, output_model_file)
                    train_npy_data_path = "D:/SubEEG/data/lhxl.npy"
                    # test_npy_data_path = "D:/SubEEG/data/grr.npy"
                    train_npy_label = "D:/SubEEG/label/lhxl.npy"
                    train_npy_mat = "D:/SubEEG/lhxl_process.mat"
                    # test_npy_label = "D:/SubEEG/label/grr.npy"
                    # model_path = "D:/SubEEG/model/grr.pth"
                    scaler = save_to_train_npy(train_npy_mat, train_npy_data_path, train_npy_label, scaler)
                    save_to_test_npy(output_mat_file, output_npy_data, output_npy_label,scaler)
                    train_loder, test_loder = Pget_data_loaders(train_npy_data_path, train_npy_label,
                                                                output_npy_data, output_npy_label)
                    train_and_save_model(train_loder, test_loder, output_model_file)
                    logging.info(f"[线程{thread_id}] 步骤3完成: 模型训练")

                    # 训练完成后设置标志
                    self.current_model_path = output_model_file
                    self.first_training_completed = True
                    logging.info(f"[线程{thread_id}] ★ 第一次训练完成，模型可用: {self.current_model_path}")

                except Exception as e:
                    logging.error(f"[线程{thread_id}] 步骤3失败 - 模型训练错误: {e}")
                    logging.debug(f"[线程{thread_id}] 模型训练错误详情:\n{traceback.format_exc()}")
                    return

                logging.info(f"[线程{thread_id}] ✓ 文件 {filename_without_ext} 所有步骤处理完成")

            except Exception as e:
                logging.error(f"[线程{thread_id}] 事件54处理发生未预期错误: {e}")
                logging.debug(f"[线程{thread_id}] 未预期错误详情:\n{traceback.format_exc()}")
            finally:
                try:
                    if threading.current_thread() in self.processing_threads:
                        self.processing_threads.remove(threading.current_thread())
                except:
                    pass
                logging.debug(f"[线程{thread_id}] 后台处理线程结束")

        try:
            thread = threading.Thread(target=safe_background_process, daemon=True)
            thread.name = f"Event54-{len(self.processing_threads)}"
            self.processing_threads.append(thread)
            thread.start()
            logging.info(f"事件54后台处理线程已启动: {thread.name}")
        except Exception as e:
            logging.error(f"启动事件54处理线程失败: {e}")

    def handle_packet(self, packet_data):
        """处理解析后的数据包 - 最小修改原有逻辑"""
        try:
            if packet_data['type'] == 'data':
                if self.receiving_data:
                    eeg_data = packet_data.get('eeg_data', [])
                    if eeg_data:
                        logging.debug(
                            f"接收EEG数据: {packet_data['channel_count']}通道, {packet_data['sample_count']}采样点")
                        # 只在第二次及之后进行实时分类
                        if self.event_51_count >= 2:
                            self.process_realtime_classification(eeg_data)
                else:
                    logging.debug("数据传输已停止，忽略数据包")

            elif packet_data['type'] == 'event':
                event_type = packet_data['event_type']
                logging.info(f"接收到事件通知: 类型={event_type}, 文件={packet_data.get('filename', 'N/A')}")

                if event_type == 51:
                    # 简单计数
                    self.event_51_count += 1
                    self.receiving_data = True
                    logging.info(f"*** 事件51 (第{self.event_51_count}次): 开始数据接收 ***")

                    # 第二次及之后才初始化实时缓冲区
                    if self.event_51_count >= 2:
                        self.realtime_buffer = [[], []]  # 重置为2通道空缓冲区
                        logging.info("开启实时分类模式 - 2通道缓冲区已重置")

                elif event_type == 54:
                    self.receiving_data = False
                    logging.info(f"*** 事件54 (第{self.event_51_count}次): 停止数据接收，开始后台处理 ***")
                    self.process_event_54(packet_data.get('filename', ''))

                else:
                    logging.info(f"*** 其他事件类型: {event_type} ***")

            elif packet_data['type'] == 'raw':
                if self.receiving_data:
                    logging.debug(f"接收原始数据: {packet_data.get('num_doubles', 0)} doubles")
                    # 只在第二次及之后进行实时分类
                    if self.event_51_count >= 2:
                        channels_data = packet_data.get('channels_data', [])
                        if channels_data and len(channels_data) >= 2:
                            self.process_realtime_classification(channels_data)
                else:
                    logging.debug("数据传输已停止，忽略原始数据")

        except Exception as e:
            logging.error(f"处理数据包时出错: {e}")
            logging.debug(f"数据包处理错误详情:\n{traceback.format_exc()}")

    def process_buffer(self):
        """处理接收缓冲区中的数据 - 保持原有逻辑"""
        try:
            while len(self.buffer) > 0 and self.running:
                processed = False
                original_buffer_size = len(self.buffer)

                if original_buffer_size > 1024 * 1024:
                    logging.warning(f"缓冲区过大 ({original_buffer_size} bytes)，清空部分数据")
                    self.buffer = self.buffer[-1024:]
                    break

                if len(self.buffer) >= 4:
                    try:
                        if self.buffer[:4] == struct.pack('>I', 0x12345678):
                            packet = self.parse_data_packet(self.buffer)
                            if packet:
                                self.handle_packet(packet)
                                self.buffer = self.buffer[packet['total_size']:]
                                processed = True

                        elif self.buffer[:4] == struct.pack('>I', 0x87654321):
                            packet = self.parse_event_notification(self.buffer)
                            if packet:
                                self.handle_packet(packet)
                                self.buffer = self.buffer[packet['total_size']:]
                                processed = True
                    except Exception as e:
                        logging.debug(f"结构化数据包解析失败: {e}")

                if not processed and len(self.buffer) >= 8:
                    try:
                        next_data_header = self.buffer.find(struct.pack('>I', 0x12345678), 1)
                        next_event_header = self.buffer.find(struct.pack('>I', 0x87654321), 1)

                        next_header = min(
                            [pos for pos in [next_data_header, next_event_header] if pos != -1],
                            default=len(self.buffer)
                        )

                        raw_data = self.buffer[:next_header]
                        if len(raw_data) >= 8:
                            parsed_raw = self.parse_raw_data(raw_data)
                            if parsed_raw:
                                self.handle_packet(parsed_raw)

                        self.buffer = self.buffer[next_header:]
                        processed = True
                    except Exception as e:
                        logging.debug(f"原始数据处理失败: {e}")

                if not processed:
                    if len(self.buffer) == original_buffer_size:
                        self.buffer = self.buffer[1:]
                    else:
                        break

        except Exception as e:
            logging.error(f"缓冲区处理异常: {e}")
            self.buffer = b''

    def handle_client(self, client_socket):
        """处理客户端连接 - 保持原有逻辑"""
        self.client_socket = client_socket  # 保存引用用于发送数据
        thread_id = threading.current_thread().ident
        try:
            client_socket.settimeout(1.0)

            while self.running:
                try:
                    data = client_socket.recv(4096)
                    if not data:
                        logging.debug(f"[线程{thread_id}] 客户端发送空数据，连接关闭")
                        break

                    self.packet_count += 1
                    logging.debug(f"[线程{thread_id}] 接收数据包 #{self.packet_count}: {len(data)} 字节")

                    self.buffer += data
                    self.process_buffer()

                except socket.timeout:
                    continue
                except socket.error as e:
                    logging.debug(f"[线程{thread_id}] 套接字错误: {e}")
                    break
                except Exception as e:
                    logging.error(f"[线程{thread_id}] 客户端处理异常: {e}")
                    break

        except Exception as e:
            logging.error(f"[线程{thread_id}] 客户端处理严重错误: {e}")
        finally:
            try:
                client_socket.close()
            except:
                pass
            self.client_socket = None
            logging.info(f"[线程{thread_id}] 客户端连接已关闭")

    def start_server(self):
        """启动TCP服务器 - 保持原有逻辑"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket.bind((self.host, self.port))
            self.socket.listen(5)
            self.socket.settimeout(1.0)

            logging.info(f"TCP服务器启动成功，监听地址: {self.host}:{self.port}")

            while self.running:
                try:
                    client_socket, address = self.socket.accept()
                    logging.info(f"客户端连接: {address}")

                    client_thread = threading.Thread(
                        target=self.handle_client,
                        args=(client_socket,),
                        daemon=True
                    )
                    client_thread.name = f"Client-{address[0]}:{address[1]}"
                    client_thread.start()

                except socket.timeout:
                    continue
                except socket.error as e:
                    if self.running:
                        logging.warning(f"Accept错误: {e}")
                except Exception as e:
                    logging.error(f"服务器运行异常: {e}")

        except Exception as e:
            logging.error(f"服务器启动失败: {e}")
        finally:
            self.stop_server()

    def stop_server(self):
        """停止服务器 - 保持原有逻辑"""
        self.running = False
        if self.socket:
            try:
                self.socket.close()
            except:
                pass

        if self.processing_threads:
            logging.info(f"等待 {len(self.processing_threads)} 个后台处理线程完成...")
            for thread in self.processing_threads[:]:
                try:
                    thread.join(timeout=2)
                    if thread.is_alive():
                        logging.warning(f"线程 {thread.name} 未能及时结束")
                except:
                    pass

        logging.info("TCP服务器已停止")


def main():
    """主函数"""
    print("=== EEG数据接收器 (2通道数据修复版) ===")
    print("功能:")
    print("- 第1次事件51/54: 完全使用原有逻辑进行训练")
    print("- 第2次及之后事件51: 增加实时分类功能(确保2通道)")
    print("- 第2次及之后事件54: 跳过训练")
    print("- 修复2通道数据处理问题")
    print("- 自动发送分类结果到C++")
    print("-" * 50)

    receiver = EEGDataReceiver()

    try:
        receiver.start_server()
    except KeyboardInterrupt:
        print("\n用户中断服务器")
    except Exception as e:
        logging.error(f"主程序严重错误: {e}")
        logging.debug(f"主程序错误详情:\n{traceback.format_exc()}")
    finally:
        try:
            receiver.stop_server()
        except:
            pass


if __name__ == "__main__":
    main()