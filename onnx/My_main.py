import socket
import struct
import threading
import time
import random
import numpy as np

class IntDataSender:
    """整数数据发送器 - 模拟发送0,1,2给C++"""

    def __init__(self, client_socket):
        self.client_socket = client_socket
        self.sending_active = False
        self.send_thread = None

    def start_sending(self, interval=3):
        """开始发送整数数据"""
        if self.sending_active:
            return

        self.sending_active = True
        self.send_thread = threading.Thread(target=self._send_loop, args=(interval,))
        self.send_thread.daemon = True
        self.send_thread.start()
        print(f"开始每{interval}秒发送一次整数数据...")

    def stop_sending(self):
        """停止发送数据"""
        self.sending_active = False
        if self.send_thread:
            self.send_thread.join(timeout=1)
        print("停止发送整数数据")

    def _send_loop(self, interval):
        """发送循环"""
        while self.sending_active:
            try:
                # 随机生成0,1,2中的一个数
                value = random.randint(0, 2)
                self.send_int_to_cpp(value)
                time.sleep(interval)
            except Exception as e:
                print(f"发送数据时出错: {e}")
                break

    def send_int_to_cpp(self, value):
        """发送单个整数到C++"""
        if not self.client_socket:
            print("No client socket available")
            return False

        try:
            # 使用大端序打包整数（32位）
            packet = struct.pack('>i', value)

            # 发送数据
            bytes_sent = self.client_socket.send(packet)

            if bytes_sent == 4:
                print(f"成功发送整数到C++: {value}")
                return True
            else:
                print(f"发送失败，只发送了{bytes_sent}字节")
                return False

        except Exception as e:
            print(f"发送整数失败: {e}")
            return False


def enhanced_data_receiver():
    """TCP数据接收器 - 接收数据包和事件通知，同时发送整数"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', 8888))
    server.listen(1)

    print("TCP服务器启动，等待连接...")

    client, addr = server.accept()
    print(f"客户端连接: {addr}")

    # 创建整数发送器
    int_sender = IntDataSender(client)

    # 启动发送器（每3秒发送一次）
    int_sender.start_sending(interval=3)

    packet_count = 0
    event_count = 0
    buffer = b''

    try:
        while True:
            data = client.recv(8192)
            if not data:
                break

            buffer += data

            # 处理缓冲区中的完整数据包
            while len(buffer) >= 4:
                processed = False

                # 查找数据包标识符或事件通知标识符
                for i in range(len(buffer) - 3):
                    # 检查数据包标识符 0x12345678
                    if buffer[i:i + 4] == struct.pack('>I', 0x12345678):
                        if i > 0:
                            buffer = buffer[i:]

                        packet_data = parse_data_packet(buffer)
                        if packet_data is None:
                            break

                        packet_count += 1
                        print_packet_info(packet_count, packet_data)

                        # 收到数据包时，立即发送一个响应整数
                        response_value = packet_count % 3  # 0, 1, 2 循环
                        int_sender.send_int_to_cpp(response_value)

                        buffer = buffer[packet_data['total_size']:]
                        processed = True
                        break

                    # 检查事件通知标识符 0x87654321
                    elif buffer[i:i + 4] == struct.pack('>I', 0x87654321):
                        if i > 0:
                            buffer = buffer[i:]

                        event_data = parse_event_notification(buffer)
                        if event_data is None:
                            break

                        event_count += 1
                        print_event_info(event_count, event_data)

                        # 根据事件类型发送不同的响应
                        if event_data['event_type'] == 51:
                            # 事件51：开始传输，发送1
                            int_sender.send_int_to_cpp(1)
                            print("响应事件51：发送整数1")
                        elif event_data['event_type'] == 54:
                            # 事件54：停止传输，发送0
                            int_sender.send_int_to_cpp(0)
                            print("响应事件54：发送整数0")

                        buffer = buffer[event_data['total_size']:]
                        processed = True
                        break

                if not processed:
                    # 没有找到完整的标识符，保留最后3字节
                    if len(buffer) > 3:
                        buffer = buffer[-3:]
                    break

    except KeyboardInterrupt:
        print("\n接收被中断")
    except Exception as e:
        print(f"接收出错: {e}")
        import traceback
        traceback.print_exc()
    finally:
        # 停止发送器
        int_sender.stop_sending()
        client.close()
        server.close()
        print("连接关闭")

# 接收到Python分类结果: 1 映射标签: 0
# Label appended: Label= 2 Time= 43462 ms Sample Point= 8280
# *** Processing Python integer: 2  ***
# Python sent classification result: Class 2
# DataOperation received Python classification result: 2
# 接收到Python分类结果: 2 映射标签: 0
# Received valid label list from Python: (2, 0)
# Label appended: Label= 2 Time= 43501 ms Sample Point= 8280
# *** Processing Python integer: 2  ***
# Python sent classification result: Class 2
# DataOperation received Python classification result: 2
# 接收到Python分类结果: 2 映射标签: 0
# Label appended: Label= 0 Time= 43532 ms Sample Point= 8280
# *** Processing Python integer: 0  ***
# Python sent classification result: Class 0
# DataOperation received Python classification result: 0
# 接收到Python分类结果: 0 映射标签: 0
# Received valid label list from Python: (0, 0)
# Label appended: Label= 0 Time= 43560 ms Sample Point= 8280
# *** Processing Python integer: 0  ***
# Python sent classification result: Class 0
# DataOperation received Python classification result: 0
# python
def send_label_list_to_cpp(client_socket, label_value):
    """一次发送五个整数到C++（4字节长度=5 + 5字节标签）"""
    try:
        # 统一为列表
        if isinstance(label_value, (list, tuple, np.ndarray)):
            label_list = [int(x) for x in label_value]
        else:
            # 标量：复制成5个
            label_list = [int(label_value)] * 5

        # 将列表按5个为一组发送；最后一组不足5个则用最后一个值填充
        def chunks_of_five(src):
            i = 0
            n = len(src)
            while i < n:
                chunk = src[i:i+5]
                if len(chunk) < 5:
                    pad_val = chunk[-1] if chunk else 0
                    chunk = chunk + [pad_val] * (5 - len(chunk))
                yield chunk
                i += 5

        all_ok = True
        for group in chunks_of_five(label_list):
            # 长度固定为5
            length_packet = struct.pack('>i', 5)
            data_packet = struct.pack('>5B', *group)
            full_packet = length_packet + data_packet

            bytes_sent = client_socket.send(full_packet)
            print(f"★ 发送分类结果到C++: {group} ({bytes_sent} 字节)")
            if bytes_sent != len(full_packet):
                all_ok = False

        return all_ok

    except Exception as e:
        print(f"发送标签失败: {e}")
        return False



def simple_data_receiver_with_sender():
    """简化的TCP数据接收器 - 兼容原始格式，并发送整数列表"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', 8888))
    server.listen(1)

    print("TCP服务器启动（简化模式），等待连接...")

    client, addr = server.accept()
    print(f"客户端连接: {addr}")

    # 注意：不再使用IntDataSender，直接使用新的发送函数
    packet_count = 0

    try:
        while True:
            data = client.recv(4096)
            if not data:
                break

            packet_count += 1
            print(f"\n=== 数据包 #{packet_count} ===")
            print(f"接收到 {len(data)} 字节原始数据")

            # 尝试新格式解析
            try:
                # 检查是否是数据包
                if len(data) >= 4 and data[:4] == struct.pack('>I', 0x12345678):
                    packet_data = parse_data_packet(data)
                    if packet_data:
                        print_packet_info(packet_count, packet_data)

                        # 发送标签列表响应 - 根据包计数生成不同的标签组合
                        if packet_count % 6 == 0:
                            send_label_list_to_cpp(client, [0, 0])
                        elif packet_count % 6 == 1:
                            send_label_list_to_cpp(client, [1, 1])
                        elif packet_count % 6 == 2:
                            send_label_list_to_cpp(client, [2, 2])
                        elif packet_count % 6 == 3:
                            send_label_list_to_cpp(client, [0, 1])
                        elif packet_count % 6 == 4:
                            send_label_list_to_cpp(client, [1, 2])
                        else:
                            send_label_list_to_cpp(client, [2, 0])
                        continue

                # 检查是否是事件通知
                if len(data) >= 4 and data[:4] == struct.pack('>I', 0x87654321):
                    event_data = parse_event_notification(data)
                    if event_data:
                        print_event_info(packet_count, event_data)

                        # 事件通知响应 - 发送特定的标签组合
                        event_type = event_data.get('event_type', 0)
                        if event_type == 51:  # 开始缓存事件
                            send_label_list_to_cpp(client, [1, 0])
                            print("▶ 事件51响应: 发送 [1, 0]")
                        elif event_type == 54:  # 结束缓存事件
                            send_label_list_to_cpp(client, [0, 1])
                            print("▶ 事件54响应: 发送 [0, 1]")
                        else:
                            send_label_list_to_cpp(client, [2, 2])
                            print(f"▶ 事件{event_type}响应: 发送 [2, 2]")
                        continue
            except Exception as parse_error:
                print(f"解析新格式时出错: {parse_error}")

            # 回退到原始格式解析
            num_doubles = len(data) // 8
            if len(data) % 8 != 0:
                print(f"警告: 数据长度不是8的倍数，剩余 {len(data) % 8} 字节")

            if num_doubles > 0:
                doubles = struct.unpack(f'>{num_doubles}d', data[:num_doubles * 8])
                print(f"解析出 {num_doubles} 个double值（原始格式）")
                print(f"前5个值: {[f'{v:.6f}' for v in doubles[:5]]}")

                # 假设2个通道重新组织数据
                channels = 2
                samples_per_channel = num_doubles // channels
                if num_doubles % channels == 0:
                    print(f"按 {channels} 通道重新组织:")
                    for ch in range(channels):
                        channel_data = []
                        for sample in range(samples_per_channel):
                            index = ch * samples_per_channel + sample
                            channel_data.append(doubles[index])

                        print(f"通道 {ch}: {len(channel_data)} 个采样点")
                        if len(channel_data) > 0:
                            print(f"  范围: {min(channel_data):.6f} ~ {max(channel_data):.6f}")

            # 原始格式响应 - 发送随机标签对
            random_labels = [random.randint(0, 2), random.randint(0, 2)]
            send_label_list_to_cpp(client, random_labels)
            print(f"▶ 原始格式响应: 发送 {random_labels}")

    except KeyboardInterrupt:
        print("\n接收被中断")
    except Exception as e:
        print(f"接收出错: {e}")
        import traceback
        traceback.print_exc()
    finally:
        client.close()
        server.close()
        print("连接关闭")


def test_int_sender_only():
    """纯整数发送测试 - 只发送整数，不接收数据"""
    try:
        # 连接到C++服务器
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect(('127.0.0.1', 8888))
        print("连接到C++服务器成功")

        # 创建发送器
        sender = IntDataSender(client)

        print("开始发送测试...")
        print("将每2秒发送一个随机整数(0-2)")
        print("按Ctrl+C停止")

        # 手动发送循环
        counter = 0
        while True:
            value = counter % 3  # 循环发送0,1,2
            success = sender.send_int_to_cpp(value)
            if not success:
                print("发送失败，退出测试")
                break

            counter += 1
            time.sleep(2)

    except KeyboardInterrupt:
        print("\n测试被中断")
    except Exception as e:
        print(f"测试出错: {e}")
    finally:
        try:
            client.close()
        except:
            pass
        print("测试结束")


# 保持原有的解析函数
def parse_data_packet(buffer):
    """解析数据包"""
    offset = 0

    # 检查缓冲区长度
    if len(buffer) < 16:  # 最小包头大小
        return None

    try:
        # 1. 解析标识符 (4字节)
        identifier = struct.unpack('>I', buffer[offset:offset + 4])[0]
        offset += 4

        if identifier != 0x12345678:
            raise ValueError(f"无效的数据包标识符: 0x{identifier:08X}")

        # 2. 解析事件类型 (4字节)
        event_type = struct.unpack('>I', buffer[offset:offset + 4])[0]
        offset += 4

        # 3. 解析文件名 (QString格式: 4字节长度 + UTF-16编码字符串)
        if len(buffer) < offset + 4:
            return None

        filename_length = struct.unpack('>I', buffer[offset:offset + 4])[0]
        offset += 4

        # 检查文件名数据是否完整
        if len(buffer) < offset + filename_length:
            return None

        if filename_length > 0:
            filename_bytes = buffer[offset:offset + filename_length]
            # Qt的QString使用UTF-16编码
            filename = filename_bytes.decode('utf-16be', errors='ignore')
        else:
            filename = ""
        offset += filename_length

        # 4. 解析通道数 (4字节)
        if len(buffer) < offset + 4:
            return None
        channel_count = struct.unpack('>I', buffer[offset:offset + 4])[0]
        offset += 4

        # 5. 解析采样点数 (4字节)
        if len(buffer) < offset + 4:
            return None
        sample_count = struct.unpack('>I', buffer[offset:offset + 4])[0]
        offset += 4

        # 6. 计算数据大小并检查是否完整
        data_size = channel_count * sample_count * 8  # 每个double是8字节
        if len(buffer) < offset + data_size:
            return None

        # 7. 解析实际数据
        eeg_data = []
        if data_size > 0:
            # 读取所有double值
            num_doubles = channel_count * sample_count
            doubles = struct.unpack(f'>{num_doubles}d', buffer[offset:offset + data_size])

            # 重新组织为通道格式
            for ch in range(channel_count):
                channel_data = []
                for sample in range(sample_count):
                    index = ch * sample_count + sample
                    channel_data.append(doubles[index])
                eeg_data.append(channel_data)

        offset += data_size

        return {
            'identifier': identifier,
            'event_type': event_type,
            'filename': filename,
            'channel_count': channel_count,
            'sample_count': sample_count,
            'eeg_data': eeg_data,
            'total_size': offset
        }

    except struct.error as e:
        print(f"数据解析错误: {e}")
        return None
    except UnicodeDecodeError as e:
        print(f"文件名解码错误: {e}")
        return None


def parse_event_notification(buffer):
    """解析事件通知包"""
    offset = 0

    if len(buffer) < 16:
        return None

    try:
        # 1. 解析事件通知标识符
        identifier = struct.unpack('>I', buffer[offset:offset + 4])[0]
        offset += 4

        if identifier != 0x87654321:
            raise ValueError(f"无效的事件通知标识符: 0x{identifier:08X}")

        # 2. 解析事件类型
        event_type = struct.unpack('>I', buffer[offset:offset + 4])[0]
        offset += 4

        # 3. 解析文件名
        if len(buffer) < offset + 4:
            return None

        filename_length = struct.unpack('>I', buffer[offset:offset + 4])[0]
        offset += 4

        if len(buffer) < offset + filename_length:
            return None

        if filename_length > 0:
            filename_bytes = buffer[offset:offset + filename_length]
            filename = filename_bytes.decode('utf-16be', errors='ignore')
        else:
            filename = ""
        offset += filename_length

        # 4. 解析总采样点数
        if len(buffer) < offset + 8:
            return None
        sample_count = struct.unpack('>Q', buffer[offset:offset + 8])[0]
        offset += 8

        # 5. 解析时间戳
        if len(buffer) < offset + 8:
            return None
        timestamp = struct.unpack('>q', buffer[offset:offset + 8])[0]
        offset += 8

        return {
            'identifier': identifier,
            'event_type': event_type,
            'filename': filename,
            'sample_count': sample_count,
            'timestamp': timestamp,
            'total_size': offset
        }

    except struct.error as e:
        print(f"事件通知解析错误: {e}")
        return None
    except UnicodeDecodeError as e:
        print(f"文件名解码错误: {e}")
        return None


def print_packet_info(packet_num, packet_data):
    """打印数据包信息"""
    print(f"\n=== 数据包 #{packet_num} ===")
    print(f"标识符: 0x{packet_data['identifier']:08X}")
    print(f"事件类型: {packet_data['event_type']}")
    print(f"文件名: '{packet_data['filename']}'")
    print(f"通道数: {packet_data['channel_count']}")
    print(f"采样点数: {packet_data['sample_count']}")
    print(f"数据包总大小: {packet_data['total_size']} 字节")

    # 显示脑电数据信息
    eeg_data = packet_data['eeg_data']
    if eeg_data:
        print(f"脑电数据:")
        for ch_idx, channel_data in enumerate(eeg_data):
            if len(channel_data) > 0:
                min_val = min(channel_data)
                max_val = max(channel_data)
                avg_val = sum(channel_data) / len(channel_data)
                print(f"  通道 {ch_idx}: {len(channel_data)} 个采样点")
                print(f"    范围: {min_val:.6f} ~ {max_val:.6f}")
                print(f"    平均: {avg_val:.6f}")
                if len(channel_data) >= 5:
                    print(f"    前5个值: {[f'{v:.6f}' for v in channel_data[:5]]}")
    else:
        print("无脑电数据")


def print_event_info(event_num, event_data):
    """打印事件通知信息"""
    print(f"\n!!! 事件通知 #{event_num} !!!")
    print(f"标识符: 0x{event_data['identifier']:08X}")
    print(f"事件类型: {event_data['event_type']}")
    print(f"文件名: '{event_data['filename']}'")
    print(f"总采样点数: {event_data['sample_count']}")
    print(f"时间戳: {event_data['timestamp']} ms ({event_data['timestamp'] / 1000:.3f}s)")

    if event_data['event_type'] == 51:
        print("*** 事件51: 开始数据缓存和TCP传输 ***")
    elif event_data['event_type'] == 54:
        print("*** 事件54: 停止数据传输，连接保持 ***")
    else:
        print(f"*** 其他事件类型: {event_data['event_type']} ***")


if __name__ == "__main__":
    print("选择运行模式:")
    print("1. 增强模式 (接收数据并发送整数)")
    print("2. 简化模式 (兼容模式并发送整数)")
    print("3. 纯整数发送测试 (只发送整数)")

    choice = input("请选择 (1/2/3): ").strip()

    if choice == "1":
        enhanced_data_receiver()
    elif choice == "2":
        simple_data_receiver_with_sender()
    elif choice == "3":
        test_int_sender_only()
    else:
        print("无效选择，使用增强模式")
        enhanced_data_receiver()