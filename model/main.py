import os
import struct
import socket
import threading
import time
import logging
import traceback
from typing import Optional, Dict, Any, List

import numpy as np
import pandas as pd
from sklearn import preprocessing  # For StandardScaler

# External training/inference pipeline
from data_trainer2 import *

# Optional modules guarded by MODULES_AVAILABLE
try:
    from data_trainer import get_data_loaders, load_model_weights_predict  # noqa: F401
    from preprocess import preprocess_eeg, save_to_train_npy, preprocess_data, save_to_test_npy  # noqa: F401
    from train import train_and_save_model  # noqa: F401

    MODULES_AVAILABLE = True
except Exception as e:
    MODULES_AVAILABLE = False

# Logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    handlers=[
        logging.FileHandler("tcp_receiver.log"),
        logging.StreamHandler()
    ],
)

# Protocol constants
DATA_HEADER = 0x12345678
EVENT_HEADER = 0x87654321

# Paths (unified)
PATH_CONFIG = {
    "base": "D:/SubEEG/",
    "data": "D:/SubEEG/data/",
    "label": "D:/SubEEG/label/",
    "model": "D:/SubEEG/model/",
}


def ensure_directories(paths: Dict[str, str]) -> None:
    for p in paths.values():
        try:
            os.makedirs(p, exist_ok=True)
        except Exception as e:
            logging.warning(f"创建目录失败 {p}: {e}")


def decode_filename(bytes_data: bytes) -> str:
    if not bytes_data:
        return ""
    for enc in ("utf-16be", "utf-8", "latin1"):
        try:
            return bytes_data.decode(enc, errors="ignore")
        except Exception:
            continue
    return "unknown"


class EEGDataReceiver:
    def __init__(self, host: str = "127.0.0.1", port: int = 8888):
        self.host = host
        self.port = port

        self.socket: Optional[socket.socket] = None
        self.client_socket: Optional[socket.socket] = None
        self._proc_lock = threading.Lock()  # 分类互斥锁（非重入）
        self.receiving_data = False
        self.running = True

        self.packet_count = 0
        self.buffer = b""
        self.processing_threads: List[threading.Thread] = []

        self.send_batches = 0
        self.total_received_samples = 0
        self.filename = ""
        # Event tracking: 分为三个阶段
        self.event_51_count = 0

        # Model path and training flags
        # self.current_model_path = os.path.join(PATH_CONFIG["model"], f"{self.filename}_1.pth")
        self.current_model_path = None
        self.first_training_completed = False
        # Online classification buffers and parameters (fixed 2 channels)
        self.realtime_buffer = [[], []]  # 2 channels
        self.archive_buffer = [[], []]
        self.window_len = 500
        self.overlap = 250
        self.num_windows = 5
        self.target_samples = self.window_len + self.overlap * (self.num_windows - 1)  # 1500
        self.stride = self.target_samples - self.overlap  # 1250
        self.prc_call_count = 0
        ensure_directories(PATH_CONFIG)

    # ---------------------------
    # Socket sending helpers
    # ---------------------------
    def send_label_to_cpp(self, label_value: int) -> bool:
        try:
            if not self.client_socket:
                return False
            label_packet = struct.pack(">i", int(label_value))
            self.client_socket.sendall(label_packet)
            logging.info(f"★ 发送分类结果到C++: {label_value}")
            return True
        except Exception as e:
            logging.error(f"发送标签失败: {e}")
            return False

    def send_2label_to_cpp(self, labels_input) -> bool:
        try:
            if not self.client_socket:
                logging.error("客户端socket未连接")
                return False

            if isinstance(labels_input, (list, tuple, np.ndarray)):
                arr = np.array(labels_input).reshape(-1)
                labels = [int(x) for x in arr[-5:]]  # send last 5
            else:
                labels = [int(labels_input)]

            length_packet = struct.pack(">I", len(labels))
            data_packet = struct.pack(">" + "B" * len(labels), *labels)
            self.client_socket.sendall(length_packet + data_packet)
            logging.info(f"★ 发送分类结果到C++(N={len(labels)}): {labels}")
            return True
        except Exception as e:
            logging.error(f"发送标签异常: {e}")
            return False

    def save_archive_to_excel(self, filename: str = "") -> None:
        """将整段会话的全部采样点导出为Excel。"""
        try:
            max_len = max(len(self.archive_buffer[0]), len(self.archive_buffer[1]))
            if max_len == 0:
                logging.info("无采样点可保存，跳过导出Excel")
                return

            # 对齐两通道长度（短通道用NaN补齐）
            ch0 = self.archive_buffer[0] + [np.nan] * (max_len - len(self.archive_buffer[0]))
            ch1 = self.archive_buffer[1] + [np.nan] * (max_len - len(self.archive_buffer[1]))

            base_filename = os.path.basename(filename) if filename else "session"
            filename_without_ext = os.path.splitext(base_filename)[0] if base_filename else "session"
            ts = time.strftime("%Y%m%d_%H%M%S")
            out_path = os.path.join(PATH_CONFIG["data"], f"{filename_without_ext}_{ts}_all_samples.xlsx")

            df = pd.DataFrame({"ch0": ch0, "ch1": ch1})
            df.to_excel(out_path, index=False)
            logging.info(f"✓ 已导出全量采样到Excel: {out_path}")

            # 导出后清空
            self.archive_buffer = [[], []]
        except Exception as e:
            logging.error(f"导出Excel失败: {e}", exc_info=True)

    # ---------------------------
    # Packet parsing
    # ---------------------------
    def parse_data_packet(self, buffer: bytes) -> Optional[Dict[str, Any]]:
        try:
            offset = 0
            if len(buffer) < 16:
                return None

            # 解析包头标识符
            identifier = struct.unpack(">I", buffer[offset:offset + 4])[0]
            if identifier != DATA_HEADER:
                return None
            offset += 4

            # 解析事件类型
            event_type = struct.unpack(">I", buffer[offset:offset + 4])[0]
            offset += 4

            # 解析文件名长度和内容
            if len(buffer) < offset + 4:
                return None
            filename_length = struct.unpack(">I", buffer[offset:offset + 4])[0]
            offset += 4

            if filename_length > 1024 or len(buffer) < offset + filename_length:
                return None

            # 特别注意：QString在QDataStream中默认使用UTF-16编码
            filename_bytes = buffer[offset:offset + filename_length]
            try:
                # 尝试UTF-16解码（QString的默认编码）
                filename = filename_bytes.decode('utf-16be')
            except UnicodeDecodeError:
                # 如果失败，尝试其他编码
                filename = decode_filename(filename_bytes)
            offset += filename_length

            # 解析通道数和采样点数
            if len(buffer) < offset + 8:
                return None
            channel_count = struct.unpack(">I", buffer[offset:offset + 4])[0]
            sample_count = struct.unpack(">I", buffer[offset + 4:offset + 8])[0]
            offset += 8

            # 验证数据维度
            if channel_count <= 0 or sample_count <= 0:
                return None
            if channel_count > 100 or sample_count > 100000:
                logging.warning(f"数据维度异常: channels={channel_count}, samples={sample_count}")
                return None

            # 解析数据部分
            data_size = channel_count * sample_count * 8
            if len(buffer) < offset + data_size:
                return None

            # 解析double数据
            num_doubles = channel_count * sample_count
            try:
                doubles = struct.unpack(f">{num_doubles}d", buffer[offset:offset + data_size])
            except struct.error as e:
                logging.debug(f"EEG数据解析失败: {e}")
                return None
            offset += data_size

            # 重组为通道数据
            eeg_data = []
            for ch in range(2):
                eeg_data.append([doubles[i] for i in range(ch, len(doubles), 2)])

            # 确保有2个通道
            while len(eeg_data) < 2:
                eeg_data.append(eeg_data[0].copy() if eeg_data else [0.0] * sample_count)
            eeg_data = eeg_data[:2]

            return {
                "type": "data",
                "identifier": identifier,
                "event_type": event_type,
                "filename": filename,
                "channel_count": 2,
                "sample_count": len(eeg_data[0]),
                "eeg_data": eeg_data,
                "total_size": offset,
            }
        except Exception as e:
            logging.debug(f"数据包解析异常: {e}")
            return None

    def parse_event_notification(self, buffer: bytes) -> Optional[Dict[str, Any]]:
        try:
            offset = 0
            if len(buffer) < 16:
                return None

            identifier = struct.unpack(">I", buffer[offset:offset + 4])[0]
            if identifier != EVENT_HEADER:
                return None
            offset += 4

            event_type = struct.unpack(">I", buffer[offset:offset + 4])[0]
            offset += 4

            if len(buffer) < offset + 4:
                return None
            filename_length = struct.unpack(">I", buffer[offset:offset + 4])[0]
            offset += 4

            if filename_length > 1024 or len(buffer) < offset + filename_length:
                return None
            filename = decode_filename(buffer[offset:offset + filename_length])
            offset += filename_length

            if len(buffer) < offset + 16:
                return None
            sample_count = struct.unpack(">Q", buffer[offset:offset + 8])[0]
            timestamp = struct.unpack(">q", buffer[offset + 8:offset + 16])[0]
            offset += 16

            return {
                "type": "event",
                "identifier": identifier,
                "event_type": event_type,
                "filename": filename,
                "sample_count": sample_count,
                "timestamp": timestamp,
                "total_size": offset,
            }
        except Exception as e:
            logging.debug(f"事件通知解析异常: {e}")
            return None

    # ---------------------------
    # Core handling - 修改后的处理逻辑
    # ---------------------------
    def process_realtime_classification(self, eeg_data: List[List[float]], event_type: int) -> None:
        """只在第2轮及以上进行实时分类"""
        try:
            # 只在第2轮及以上进行实时分类
            if self.event_51_count < 3:
                return
            if not self.first_training_completed or not self.current_model_path:
                return
            if not os.path.exists(self.current_model_path):
                logging.warning(f"模型文件不存在: {self.current_model_path}")
                return

            # Map event types to labels
            event_to_label = {51: 0, 52: 1, 53: 2}
            current_label = event_to_label.get(event_type, -1)
            if current_label == -1:
                return

            # Ensure 2 channels
            if len(eeg_data) == 1:
                eeg_data = [eeg_data[0], eeg_data[0].copy()]
            eeg_data = eeg_data[:2]

            # 记录接收到的数据
            samples_in_batch = len(eeg_data[0]) if eeg_data and eeg_data[0] else 0
            self.total_received_samples += samples_in_batch

            # 追加数据到缓冲区
            for ch in range(2):
                self.realtime_buffer[ch].extend(eeg_data[ch])

            if not self._proc_lock.acquire(blocking=False):
                return

            try:
                # Trigger classification when enough samples (with overlap windows)
                while min(len(self.realtime_buffer[0]), len(self.realtime_buffer[1])) >= self.target_samples:
                    data_for_classification = np.zeros((self.num_windows, 2, self.window_len), dtype=np.float64)
                    for w in range(self.num_windows):
                        start = w * self.overlap
                        end = start + self.window_len
                        for ch in range(2):
                            seg = self.realtime_buffer[ch][start:end]
                            if seg:
                                n = min(self.window_len, len(seg))
                                data_for_classification[w, ch, :n] = seg[:n]
                            data_for_classification[w, ch] = np.nan_to_num(
                                data_for_classification[w, ch], nan=0.0, posinf=0.0, neginf=0.0
                            )

                    try:
                        # Standardize online batch
                        scaler = preprocessing.StandardScaler()
                        flat = data_for_classification.reshape(-1, 2 * self.window_len)
                        scaler.fit(flat)
                        online_data = scaler.transform(flat).reshape(self.num_windows, 2, self.window_len)

                        # Online loaders and inference (from data_trainer2)
                        test_label = np.full((self.num_windows,), current_label, dtype=np.int64)
                        train_npy_data_path = os.path.join(PATH_CONFIG["data"], f"{self.filename}_1.npy")

                        train_npy_label = os.path.join(PATH_CONFIG["label"], f"{self.filename}_1.npy")

                        train_loader, test_loader = Pget_data_online_loaders(
                            train_npy_data_path, train_npy_label, online_data, test_label
                        )
                        predictions = new_load_model_weights_predict(
                            self.current_model_path, train_loader, test_loader
                        )

                        if predictions is not None and len(np.array(predictions).reshape(-1)) > 0:
                            if self.send_2label_to_cpp(predictions):
                                self.send_batches += 1
                                logging.info(f"发送批次#{self.send_batches}: N={self.num_windows}")
                        else:
                            logging.warning("分类失败，未获得预测结果")
                    except Exception as e:
                        logging.error(f"实时分类处理失败: {e}", exc_info=True)

                    # Slide buffer: keep overlap, drop stride
                    for ch in range(2):
                        del self.realtime_buffer[ch][:self.stride]
            finally:
                self._proc_lock.release()
        except Exception as e:
            logging.error(f"实时分类异常: {e}", exc_info=True)

    def process_event_54(self, filename: str) -> None:
        """根据轮次执行不同的处理流程"""

        def safe_background_process():
            thread_id = threading.current_thread().ident


            try:
                if not filename:
                    logging.warning(f"[线程{thread_id}] 事件54: 文件名为空，跳过处理")
                    return

                base_filename = os.path.basename(filename)
                filename_without_ext = os.path.splitext(base_filename)[0]
                logging.info(f"[线程{thread_id}] 开始处理文件: {filename_without_ext} (第{self.event_51_count}轮)")

                input_mat_file = os.path.join(PATH_CONFIG["base"], f"{filename_without_ext}_{self.event_51_count-1}.mat")
                output_mat_file = os.path.join(PATH_CONFIG["base"], f"{filename_without_ext}_{self.event_51_count-1}_process.mat")
                output_npy_data = os.path.join(PATH_CONFIG["data"], f"{filename_without_ext}_{self.event_51_count-1}.npy")
                output_npy_label = os.path.join(PATH_CONFIG["label"], f"{filename_without_ext}_{self.event_51_count-1}.npy")
                output_model_file = os.path.join(PATH_CONFIG["model"], f"{filename_without_ext}_{self.event_51_count-1}.pth")

                if not MODULES_AVAILABLE:
                    logging.error(f"[线程{thread_id}] 处理模块不可用，跳过文件处理")
                    return

                # Wait for input file if needed
                if not os.path.exists(input_mat_file):
                    logging.warning(f"[线程{thread_id}] 输入文件不存在: {input_mat_file}")
                    for _ in range(5):
                        time.sleep(1)
                        if os.path.exists(input_mat_file):
                            logging.info(f"[线程{thread_id}] 文件出现，继续处理")
                            break
                    else:
                        logging.error(f"[线程{thread_id}] 等待超时，文件仍不存在: {input_mat_file}")
                        return

                # 第0轮：只进行预处理
                if self.event_51_count == 1:
                    logging.info(f"[线程{thread_id}] 第0轮: 只进行预处理")
                    try:
                        preprocess_eeg(input_mat_file, output_mat_file, downsample_freq=250)
                        logging.info(f"[线程{thread_id}] ✓ 第0轮预处理完成")
                    except Exception as e:
                        logging.error(f"[线程{thread_id}] 第0轮预处理失败: {e}")
                        return
                        # Step 2: 转换为numpy格式
                    try:

                        save_to_test_npy(output_mat_file, output_npy_data, output_npy_label)
                        logging.info(f"[线程{thread_id}] 步骤2完成: numpy转换")
                    except Exception as e:
                        logging.error(f"[线程{thread_id}] 步骤2失败 - numpy转换错误: {e}")
                        return

                # 第1轮：进行训练
                elif self.event_51_count == 2:
                    logging.info(f"[线程{thread_id}] 第1轮: 开始训练流程")

                    # Step 1: 预处理EEG
                    try:
                        preprocess_eeg(input_mat_file, output_mat_file, downsample_freq=250)
                        logging.info(f"[线程{thread_id}] 步骤1完成: EEG预处理")
                    except Exception as e:
                        logging.error(f"[线程{thread_id}] 步骤1失败 - EEG预处理错误: {e}")
                        return

                    if not os.path.exists(output_mat_file):
                        logging.error(f"[线程{thread_id}] 预处理输出文件不存在: {output_mat_file}")
                        return

                    # Step 2: 转换为numpy格式
                    try:
                        logging.info(f"[线程{thread_id}] 步骤2: 转换为numpy格式")


                        # Fit scaler on train set and save both train and test npy
                        # scaler = save_to_train_npy(train_npy_mat, train_npy_data_path, train_npy_label, scaler)
                        save_to_test_npy(output_mat_file, output_npy_data, output_npy_label)
                        logging.info(f"[线程{thread_id}] 步骤2完成: numpy转换")
                    except Exception as e:
                        logging.error(f"[线程{thread_id}] 步骤2失败 - numpy转换错误: {e}")
                        return

                    # Step 3: 训练模型
                    try:
                        logging.info(f"[线程{thread_id}] 步骤3: 训练模型")
                        train_npy_data_path = os.path.join(PATH_CONFIG["data"], f"{self.filename}_0.npy")
                        train_npy_label = os.path.join(PATH_CONFIG["label"], f"{self.filename}_0.npy")
                        # train_npy_mat = os.path.join(PATH_CONFIG["base"], "aaa_0_process.mat")
                        train_loader, test_loader = Pget_data_loaders(
                            train_npy_data_path, train_npy_label, output_npy_data, output_npy_label
                        )
                        train_and_save_model(train_loader, test_loader, output_model_file)
                        logging.info(f"[线程{thread_id}] 步骤3完成: 模型训练")

                        self.current_model_path = output_model_file
                        self.first_training_completed = True
                        logging.info(f"[线程{thread_id}] ★ 第1轮训练完成，模型可用: {self.current_model_path}")
                    except Exception as e:
                        logging.error(f"[线程{thread_id}] 步骤3失败 - 模型训练错误: {e}")
                        return

                # 第2轮及以上：进行测试（实时分类已在数据包处理中实现）
                elif self.event_51_count > 2:
                    logging.info(f"[线程{thread_id}] 第{self.event_51_count}轮: 测试模式，跳过事件54处理")

                logging.info(f"[线程{thread_id}] ✓ 文件 {filename_without_ext} 处理完成 (第{self.event_51_count}轮)")

            except Exception as e:
                logging.error(f"[线程{thread_id}] 事件54处理发生未预期错误: {e}")
            finally:
                try:
                    if threading.current_thread() in self.processing_threads:
                        self.processing_threads.remove(threading.current_thread())
                except Exception:
                    pass
                logging.debug(f"[线程{thread_id}] 后台处理线程结束")

        try:
            thread = threading.Thread(target=safe_background_process, daemon=True)
            thread.name = f"Event54-Round{self.event_51_count}"
            self.processing_threads.append(thread)
            thread.start()
            logging.info(f"事件54后台处理线程已启动: {thread.name}")
        except Exception as e:
            logging.error(f"启动事件54处理线程失败: {e}")

    def handle_packet(self, packet_data: Dict[str, Any]) -> None:
        try:
            ptype = packet_data.get("type")
            if ptype == "data":
                if not self.receiving_data:
                    return

                eeg_data = packet_data.get("eeg_data", [])
                event_type = packet_data.get("event_type", 0)
                if event_type == 0:
                    logging.warning("接收到无效的事件类型: 0，跳过处理")
                    return
                if eeg_data:
                    # 确保2通道
                    if len(eeg_data) == 1:
                        eeg_data = [eeg_data[0], eeg_data[0].copy()]
                    eeg_data = eeg_data[:2]
                    for ch in range(2):
                        self.archive_buffer[ch].extend(eeg_data[ch])

                # 只在第2轮及以上进行实时分类
                if eeg_data and self.event_51_count > 2:
                    self.prc_call_count += 1
                    self.process_realtime_classification(eeg_data, event_type)

            elif ptype == "event":
                event_type = packet_data["event_type"]
                filename = packet_data.get("filename", "")
                base_filename = os.path.basename(filename)
                filename_without_ext = os.path.splitext(base_filename)[0]
                self.filename = filename_without_ext
                logging.info(f"接收到事件通知: 类型={event_type}, 文件={filename or 'N/A'}")

                if event_type == 51:
                    self.event_51_count += 1
                    self.receiving_data = True
                    logging.info(f"*** 事件51 (第{self.event_51_count}轮): 开始数据接收 ***")

                    # 第2轮及以上时重置实时分类缓冲区
                    if self.event_51_count > 2:
                        self.realtime_buffer = [[], []]
                        logging.info("开启实时分类模式 - 2通道缓冲区已重置")

                elif event_type == 54:
                    self.receiving_data = False
                    logging.info(f"*** 事件54 (第{self.event_51_count}轮): 停止数据接收，开始后台处理 ***")
                    self.save_archive_to_excel(filename)
                    self.process_event_54(filename)

                else:
                    logging.info(f"*** 其他事件类型: {event_type} ***")

        except Exception as e:
            logging.error(f"处理数据包时出错: {e}")

    def process_buffer(self) -> None:
        try:
            while self.buffer and self.running:
                processed = False
                original_size = len(self.buffer)

                if len(self.buffer) >= 4:
                    if self.buffer[:4] == struct.pack(">I", DATA_HEADER):
                        packet = self.parse_data_packet(self.buffer)
                    elif self.buffer[:4] == struct.pack(">I", EVENT_HEADER):
                        packet = self.parse_event_notification(self.buffer)
                    else:
                        packet = None

                    if packet:
                        self.handle_packet(packet)
                        self.buffer = self.buffer[packet["total_size"]:]
                        processed = True

                if not processed:
                    # 无匹配则滚动一字节
                    if len(self.buffer) == original_size:
                        self.buffer = self.buffer[1:]
                    else:
                        break
        except Exception as e:
            logging.error(f"缓冲区处理异常: {e}")
            self.buffer = b""

    def handle_client(self, client_socket: socket.socket) -> None:
        self.client_socket = client_socket
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
            except Exception:
                pass
            self.client_socket = None
            logging.info(f"[线程{thread_id}] 客户端连接已关闭")

    def start_server(self) -> None:
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

                    t = threading.Thread(target=self.handle_client, args=(client_socket,), daemon=True)
                    t.name = f"Client-{address[0]}:{address[1]}"
                    t.start()
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

    def stop_server(self) -> None:
        self.running = False
        if self.socket:
            try:
                self.socket.close()
            except Exception:
                pass

        if self.processing_threads:
            logging.info(f"等待 {len(self.processing_threads)} 个后台处理线程完成...")
            for thread in self.processing_threads[:]:
                try:
                    thread.join(timeout=2)
                    if thread.is_alive():
                        logging.warning(f"线程 {thread.name} 未能及时结束")
                except Exception:
                    pass

        logging.info("TCP服务器已停止")


def main():
    print("=== EEG数据接收器（三阶段流程） ===")
    print("- 第0轮事件51/54: 只进行预处理")
    print("- 第1轮事件51/54: 进行训练")
    print("- 第2轮及以上事件51: 实时分类（2通道，5×500窗口，250重叠）")
    print("- 自动发送分类结果到C++")
    print("-" * 50)

    receiver = EEGDataReceiver()
    try:
        receiver.start_server()
    except KeyboardInterrupt:
        print("\n用户中断服务器")
    except Exception as e:
        logging.error(f"主程序严重错误: {e}")
    finally:
        try:
            receiver.stop_server()
        except Exception:
            pass


if __name__ == "__main__":
    main()