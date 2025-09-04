import socket
import struct
import os

from data_trainer import get_data_loaders
from preprocess import preprocess_eeg, save_to_npy
from train import train_and_save_model

path = "D:/subEEG/"
data_pth = "D:/subEEG/data/"
label_pth = "D:/subEEG/label/"
model_pth = "D:/subEEG/model/"

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
        Tfilename = event_data['filename']
        base_filename = os.path.basename(Tfilename)
        filename = os.path.splitext(base_filename)[0]
        input_mat_file = path + filename + ".mat"  
        output_mat_file = path + filename + "_process.mat"
        output_npy_data = data_pth + filename + ".npy"
        output_npy_label = label_pth + filename + ".npy"
        output_model_file = model_pth + filename + ".pth"
        # 运行预处理
        preprocess_eeg(input_mat_file, output_mat_file, downsample_freq=250)
        save_to_npy(output_mat_file,output_npy_data, output_npy_label)

        # 训练并保存模型
        train_loader, val_loader = get_data_loaders(output_npy_data, output_npy_label, batch_size=128)
        train_and_save_model(train_loader, val_loader, output_model_file)

    else:
        print(f"*** 其他事件类型: {event_data['event_type']} ***")


def simple_data_receiver():
    """简化的TCP数据接收器 - 兼容原始格式"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', 8888))
    server.listen(1)

    print("TCP服务器启动（简化模式），等待连接...")

    client, addr = server.accept()
    print(f"客户端连接: {addr}")

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
                        continue

                # 检查是否是事件通知
                if len(data) >= 4 and data[:4] == struct.pack('>I', 0x87654321):
                    event_data = parse_event_notification(data)
                    if event_data:
                        print_event_info(packet_count, event_data)
                        continue
            except:
                pass

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


if __name__ == "__main__":
    simple_data_receiver()