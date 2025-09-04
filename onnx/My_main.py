import socket
import struct


def simple_data_receiver():
    """TCP数据接收器 - 只接收纯数据"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', 8888))
    server.listen(1)

    print("TCP服务器启动，等待连接...")

    client, addr = server.accept()
    print(f"客户端连接: {addr}")

    packet_count = 0

    try:
        while True:
            # 接收数据 (每次接收一个数据块)
            data = client.recv(4096)  # 调整缓冲区大小
            if not data:
                break

            packet_count += 1
            print(f"\n=== 数据包 #{packet_count} ===")
            print(f"接收到 {len(data)} 字节原始数据")

            # 解析为double数组 (假设是大端序)
            num_doubles = len(data) // 8
            if len(data) % 8 != 0:
                print(f"警告: 数据长度不是8的倍数，剩余 {len(data) % 8} 字节")

            if num_doubles > 0:
                # 解析double数据
                doubles = struct.unpack(f'>{num_doubles}d', data[:num_doubles * 8])

                print(f"解析出 {num_doubles} 个double值")
                print(f"前5个值: {doubles[:5]}")
                if num_doubles > 5:
                    print(f"后5个值: {doubles[-5:]}")

                # 如果知道通道数，可以重新整理数据
                # 假设2个通道
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
    finally:
        client.close()
        server.close()
        print("连接关闭")


if __name__ == "__main__":
    simple_data_receiver()