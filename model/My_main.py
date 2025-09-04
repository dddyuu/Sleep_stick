import socket
import struct
import os
import threading
import time
import logging
from datetime import datetime
import traceback

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
    from data_trainer import get_data_loaders
    from preprocess import preprocess_eeg, save_to_npy
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

    def parse_data_packet(self, buffer):
        """解析数据包 - 增强错误处理"""
        try:
            offset = 0

            if len(buffer) < 16:
                return None

            # 1. 解析标识符
            identifier = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            if identifier != 0x12345678:
                return None

            # 2. 解析事件类型
            event_type = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            # 3. 解析文件名
            if len(buffer) < offset + 4:
                return None

            filename_length = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            # 限制文件名长度防止内存问题
            if filename_length > 1024:  # 1KB限制
                logging.warning(f"文件名长度异常: {filename_length}")
                return None

            if len(buffer) < offset + filename_length:
                return None

            filename = ""
            if filename_length > 0:
                try:
                    filename_bytes = buffer[offset:offset + filename_length]
                    # 尝试多种编码方式
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

            # 4. 解析通道数和采样点数
            if len(buffer) < offset + 8:
                return None

            channel_count = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4
            sample_count = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            # 数据合理性检查
            if channel_count > 100 or sample_count > 100000:
                logging.warning(f"数据维度异常: channels={channel_count}, samples={sample_count}")
                return None

            # 5. 解析EEG数据
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
                        if channel_data:  # 只添加非空通道
                            eeg_data.append(channel_data)

                except struct.error as e:
                    logging.debug(f"EEG数据解析失败: {e}")
                    eeg_data = []

            offset += data_size

            return {
                'type': 'data',
                'identifier': identifier,
                'event_type': event_type,
                'filename': filename,
                'channel_count': len(eeg_data),  # 使用实际解析的通道数
                'sample_count': len(eeg_data[0]) if eeg_data else 0,
                'eeg_data': eeg_data,
                'total_size': offset
            }

        except Exception as e:
            logging.debug(f"数据包解析异常: {e}")
            return None

    def parse_event_notification(self, buffer):
        """解析事件通知包 - 增强错误处理"""
        try:
            offset = 0

            if len(buffer) < 16:
                return None

            # 1. 解析事件通知标识符
            identifier = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            if identifier != 0x87654321:
                return None

            # 2. 解析事件类型
            event_type = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            # 3. 解析文件名
            if len(buffer) < offset + 4:
                return None

            filename_length = struct.unpack('>I', buffer[offset:offset + 4])[0]
            offset += 4

            # 限制文件名长度
            if filename_length > 1024:
                logging.warning(f"事件通知文件名长度异常: {filename_length}")
                return None

            if len(buffer) < offset + filename_length:
                return None

            filename = ""
            if filename_length > 0:
                try:
                    filename_bytes = buffer[offset:offset + filename_length]
                    # 尝试多种编码方式
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

            # 4. 解析采样点数和时间戳
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
        """解析原始double数据 - 增强错误处理"""
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

                # 假设2通道重新组织数据
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

                        return {
                            'type': 'raw',
                            'num_doubles': num_doubles,
                            'channels_data': channels_data,
                            'raw_doubles': doubles
                        }
                    except Exception as e:
                        logging.debug(f"原始数据重组失败: {e}")
                        return None

        except Exception as e:
            logging.debug(f"原始数据解析异常: {e}")

        return None

    def process_event_54(self, filename):
        """处理事件54 - 增强的错误处理和容错机制"""

        def safe_background_process():
            """安全的后台处理函数，包含完整的错误处理"""
            thread_id = threading.current_thread().ident
            try:
                if not filename:
                    logging.warning(f"[线程{thread_id}] 事件54: 文件名为空，跳过处理")
                    return

                # 提取文件名
                try:
                    base_filename = os.path.basename(filename)
                    filename_without_ext = os.path.splitext(base_filename)[0]
                except Exception as e:
                    logging.error(f"[线程{thread_id}] 文件名处理失败: {e}")
                    filename_without_ext = "unknown_file"

                logging.info(f"[线程{thread_id}] 开始处理文件: {filename_without_ext}")

                # 构建文件路径
                input_mat_file = os.path.join(PATH_CONFIG['base'], f"{filename_without_ext}_0.mat")
                output_mat_file = os.path.join(PATH_CONFIG['base'], f"{filename_without_ext}_process.mat")
                output_npy_data = os.path.join(PATH_CONFIG['data'], f"{filename_without_ext}.npy")
                output_npy_label = os.path.join(PATH_CONFIG['label'], f"{filename_without_ext}.npy")
                output_model_file = os.path.join(PATH_CONFIG['model'], f"{filename_without_ext}.pth")

                # 检查模块可用性
                if not MODULES_AVAILABLE:
                    logging.error(f"[线程{thread_id}] 处理模块不可用，跳过文件处理")
                    return

                # 检查输入文件是否存在
                if not os.path.exists(input_mat_file):
                    logging.warning(f"[线程{thread_id}] 输入文件不存在: {input_mat_file}")
                    # 尝试等待文件出现
                    for i in range(5):  # 最多等待5秒
                        time.sleep(1)
                        if os.path.exists(input_mat_file):
                            logging.info(f"[线程{thread_id}] 文件出现，继续处理")
                            break
                    else:
                        logging.error(f"[线程{thread_id}] 等待超时，文件仍不存在: {input_mat_file}")
                        return

                # 步骤1: 预处理EEG数据
                try:
                    logging.info(f"[线程{thread_id}] 步骤1: 预处理EEG数据")
                    preprocess_eeg(input_mat_file, output_mat_file, downsample_freq=250)
                    logging.info(f"[线程{thread_id}] 步骤1完成: EEG预处理")
                except Exception as e:
                    logging.error(f"[线程{thread_id}] 步骤1失败 - EEG预处理错误: {e}")
                    logging.debug(f"[线程{thread_id}] 预处理错误详情:\n{traceback.format_exc()}")
                    # 继续执行，不因为这个步骤失败而中断
                    return

                # 检查预处理输出文件
                if not os.path.exists(output_mat_file):
                    logging.error(f"[线程{thread_id}] 预处理输出文件不存在: {output_mat_file}")
                    return

                # 步骤2: 转换为numpy格式
                try:
                    logging.info(f"[线程{thread_id}] 步骤2: 转换为numpy格式")
                    save_to_npy(output_mat_file, output_npy_data, output_npy_label)
                    logging.info(f"[线程{thread_id}] 步骤2完成: numpy转换")
                except Exception as e:
                    logging.error(f"[线程{thread_id}] 步骤2失败 - numpy转换错误: {e}")
                    logging.debug(f"[线程{thread_id}] numpy转换错误详情:\n{traceback.format_exc()}")
                    return

                # 检查numpy文件
                if not os.path.exists(output_npy_data) or not os.path.exists(output_npy_label):
                    logging.error(f"[线程{thread_id}] numpy文件生成失败")
                    return

                # 步骤3: 训练模型
                try:
                    logging.info(f"[线程{thread_id}] 步骤3: 训练模型")
                    train_loader, val_loader = get_data_loaders(output_npy_data, output_npy_label, batch_size=128)
                    train_and_save_model(train_loader, val_loader, output_model_file)
                    logging.info(f"[线程{thread_id}] 步骤3完成: 模型训练")
                except Exception as e:
                    logging.error(f"[线程{thread_id}] 步骤3失败 - 模型训练错误: {e}")
                    logging.debug(f"[线程{thread_id}] 模型训练错误详情:\n{traceback.format_exc()}")
                    return

                logging.info(f"[线程{thread_id}] ✓ 文件 {filename_without_ext} 所有步骤处理完成")

            except Exception as e:
                # 最外层捕获所有未预期的异常
                logging.error(f"[线程{thread_id}] 事件54处理发生未预期错误: {e}")
                logging.debug(f"[线程{thread_id}] 未预期错误详情:\n{traceback.format_exc()}")

            finally:
                # 清理线程引用
                try:
                    if threading.current_thread() in self.processing_threads:
                        self.processing_threads.remove(threading.current_thread())
                except:
                    pass
                logging.debug(f"[线程{thread_id}] 后台处理线程结束")

        # 在后台线程中运行处理，使用daemon线程确保主程序可以正常退出
        try:
            thread = threading.Thread(target=safe_background_process, daemon=True)
            thread.name = f"Event54-{len(self.processing_threads)}"
            self.processing_threads.append(thread)
            thread.start()
            logging.info(f"事件54后台处理线程已启动: {thread.name}")
        except Exception as e:
            logging.error(f"启动事件54处理线程失败: {e}")

    def handle_packet(self, packet_data):
        """处理解析后的数据包 - 增强错误处理"""
        try:
            if packet_data['type'] == 'data':
                if self.receiving_data:
                    eeg_data = packet_data.get('eeg_data', [])
                    if eeg_data:
                        logging.debug(
                            f"接收EEG数据: {packet_data['channel_count']}通道, {packet_data['sample_count']}采样点")
                        # 这里可以添加实时数据处理逻辑
                else:
                    logging.debug("数据传输已停止，忽略数据包")

            elif packet_data['type'] == 'event':
                event_type = packet_data['event_type']
                logging.info(f"接收到事件通知: 类型={event_type}, 文件={packet_data.get('filename', 'N/A')}")

                if event_type == 51:
                    self.receiving_data = True
                    logging.info("*** 事件51: 开始数据接收 ***")

                elif event_type == 54:
                    self.receiving_data = False
                    logging.info("*** 事件54: 停止数据接收，开始后台处理 ***")
                    # 在后台处理文件，不阻塞主流程
                    self.process_event_54(packet_data.get('filename', ''))

                else:
                    logging.info(f"*** 其他事件类型: {event_type} ***")

            elif packet_data['type'] == 'raw':
                if self.receiving_data:
                    logging.debug(f"接收原始数据: {packet_data.get('num_doubles', 0)} doubles")
                else:
                    logging.debug("数据传输已停止，忽略原始数据")

        except Exception as e:
            logging.error(f"处理数据包时出错: {e}")
            logging.debug(f"数据包处理错误详情:\n{traceback.format_exc()}")

    def process_buffer(self):
        """处理接收缓冲区中的数据 - 增强错误处理"""
        try:
            while len(self.buffer) > 0 and self.running:
                processed = False
                original_buffer_size = len(self.buffer)

                # 防止无限循环
                if original_buffer_size > 1024 * 1024:  # 1MB限制
                    logging.warning(f"缓冲区过大 ({original_buffer_size} bytes)，清空部分数据")
                    self.buffer = self.buffer[-1024:]  # 保留最后1KB
                    break

                # 尝试解析数据包
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

                # 如果无法解析为结构化数据包，尝试原始数据
                if not processed and len(self.buffer) >= 8:
                    try:
                        # 查找下一个可能的包头
                        next_data_header = self.buffer.find(struct.pack('>I', 0x12345678), 1)
                        next_event_header = self.buffer.find(struct.pack('>I', 0x87654321), 1)

                        next_header = min(
                            [pos for pos in [next_data_header, next_event_header] if pos != -1],
                            default=len(self.buffer)
                        )

                        # 提取原始数据部分
                        raw_data = self.buffer[:next_header]
                        if len(raw_data) >= 8:
                            parsed_raw = self.parse_raw_data(raw_data)
                            if parsed_raw:
                                self.handle_packet(parsed_raw)

                        self.buffer = self.buffer[next_header:]
                        processed = True
                    except Exception as e:
                        logging.debug(f"原始数据处理失败: {e}")

                # 防止无限循环
                if not processed:
                    if len(self.buffer) == original_buffer_size:
                        # 缓冲区大小没有变化，移除第一个字节
                        self.buffer = self.buffer[1:]
                    else:
                        break

        except Exception as e:
            logging.error(f"缓冲区处理异常: {e}")
            # 清空缓冲区防止持续错误
            self.buffer = b''

    def handle_client(self, client_socket):
        """处理客户端连接 - 增强错误处理"""
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

                    # 添加到缓冲区
                    self.buffer += data

                    # 处理缓冲区
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
            logging.info(f"[线程{thread_id}] 客户端连接已关闭")

    def start_server(self):
        """启动TCP服务器 - 增强错误处理"""
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

                    # 为每个客户端创建处理线程
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
        """停止服务器"""
        self.running = False
        if self.socket:
            try:
                self.socket.close()
            except:
                pass

        # 等待后台处理线程完成（最多等待10秒）
        if self.processing_threads:
            logging.info(f"等待 {len(self.processing_threads)} 个后台处理线程完成...")
            for thread in self.processing_threads[:]:  # 创建副本避免迭代时修改
                try:
                    thread.join(timeout=2)  # 每个线程最多等待2秒
                    if thread.is_alive():
                        logging.warning(f"线程 {thread.name} 未能及时结束")
                except:
                    pass

        logging.info("TCP服务器已停止")


def main():
    """主函数 - 增强错误处理"""
    print("=== EEG数据接收器 (增强容错版) ===")
    print("功能:")
    print("- 自动解析数据包和事件通知")
    print("- 事件51: 开始数据接收")
    print("- 事件54: 停止接收并后台处理文件(容错处理)")
    print("- 支持原始数据格式兼容")
    print("- 增强的错误处理，不会因处理错误中断程序")
    print("- 多线程后台处理，不影响数据接收")
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