#include "DataParser.h"
#include <QDebug>

DataParser::DataParser(QObject* parent)
    : QObject(parent)
{
}

DataParser::ParsedData DataParser::parseData(const QByteArray& data)
{
    ParsedData result;
    result.valid = false;
    result.rawdata = data;

    // 根据Python代码检查数据长度（187字节）
    if (data.length() < 8) {
        qWarning() << "数据长度不足:" << data.length();
        return result;
    }

    // 根据Python代码检查帧头
    QByteArray frameHeader = QByteArray::fromHex("41495290");  // FRAME_HEADER = bytes([0x41, 0x49, 0x52, 0x90])
    if (data.left(4) != frameHeader) {
        qWarning() << "帧头校验失败";
        return result;
    }

    try {
        // 根据Python代码解析各字段：

        // 获取命令类型 (2字节) - Python: struct.unpack('<H', data_bytes[4:6])[0]
        int command = static_cast<quint8>(data[4]) | (static_cast<quint8>(data[5]) << 8);

        // 获取数据长度 (2字节) - Python: struct.unpack('<H', data_bytes[6:8])[0]
        int data_length = static_cast<quint8>(data[6]) | (static_cast<quint8>(data[7]) << 8);

        // 序列号 (3字节) - Python: order_num_bytes = data_bytes[8:11]
        QByteArray order_num_bytes = data.mid(8, 3);
        result.order_num = order_num_bytes.toHex().toUpper();

        // 电池数据 (2字节) - Python: struct.unpack('<H', data_bytes[11:13])[0]
        result.battery = static_cast<quint8>(data[11]) | (static_cast<quint8>(data[12]) << 8);

        // 音频数据 (60字节) - Python: audio_data = data_bytes[13:73]
        result.video_data = data.mid(13, 60);

        // EEG数据 (70字节) - Python: eeg_data = data_bytes[73:143]
        result.eeg_data = data.mid(73, 70);

        // 解析EEG通道数据
        result.parsed_eeg_data = parseEEGChannelData(result.eeg_data);

        // 是否脱落 (1字节) - Python: struct.unpack('<B', data_bytes[143:144])[0]
        result.is_fall = (static_cast<quint8>(data[143]) != 0);

        // 四元数据 (16字节) - Python: four_data = data_bytes[144:160]
        result.four_data = data.mid(144, 16);

        // 重力数据 (3字节) - Python: gravity_bytes = data_bytes[160:163]
        QByteArray gravity_bytes = data.mid(160, 3);
        result.gravity_data = static_cast<quint8>(gravity_bytes[0]) |
            (static_cast<quint8>(gravity_bytes[1]) << 8) |
            (static_cast<quint8>(gravity_bytes[2]) << 16);

        // 预留 (4字节) - Python: yuliu = data_bytes[163:167]
        result.yuliu = data.mid(163, 4);

        // 光源数据 (18字节) - Python: shine = data_bytes[167:185]
        result.shine = data.mid(167, 18);

        // 校验 (2字节) - Python: struct.unpack('<H', data_bytes[185:])[0]
        if (data.length() >= 187) {
            result.jiaoyan = static_cast<quint8>(data[185]) | (static_cast<quint8>(data[186]) << 8);
        }

        // 将EEG数据转换为十六进制字符串表示 - Python: payload = ' '.join(f"{b:02X}" for b in eeg_data)
        QStringList hexList;
        for (int i = 0; i < result.eeg_data.length(); ++i) {
            hexList << QString("%1").arg(static_cast<quint8>(result.eeg_data[i]), 2, 16, QChar('0')).toUpper();
        }
        result.payload = hexList.join(" ");

        result.valid = true;
    }
    catch (...) {
        qWarning() << "数据解析异常";
        result.valid = false;
    }

    return result;
}

DataParser::EEGChannelData DataParser::parseEEGChannelData(const QByteArray& eegData)
{
    EEGChannelData channelData;

    // Python代码中的处理逻辑：
    // window_size = 7, step = 7
    // for i in range(0, len(data_array), step):
    //     data = data_array[i:i + window_size]
    //     fp1.append((data[0] + data[1] * 256 + data[2] * 65536) * 10)
    //     fp2.append((data[3] + data[4] * 256 + data[5] * 65536) * 10)

    int window_size = 7;
    int step = 7;

    for (int i = 0; i < eegData.length(); i += step) {
        if (i + window_size <= eegData.length()) {
            QByteArray window = eegData.mid(i, window_size);

            // FP1通道：前3字节，小端序
            int fp1_value = (static_cast<quint8>(window[0]) +
                static_cast<quint8>(window[1]) * 256 +
                static_cast<quint8>(window[2]) * 65536) * 10;
            channelData.fp1.append(fp1_value);
            qDebug()<<"fp1"<<fp1_value;
            // FP2通道：接下来3字节，小端序
            int fp2_value = (static_cast<quint8>(window[3]) +
                static_cast<quint8>(window[4]) * 256 +
                static_cast<quint8>(window[5]) * 65536) * 10;
            channelData.fp2.append(fp2_value);
            qDebug()<<"fp2"<<fp2_value;
            // 事件标记：第7字节 - data[6]
            int event_value = static_cast<quint8>(window[6]);
            channelData.events.append(event_value);
        }
    }

    return channelData;
}

int DataParser::parse24BitLittleEndian(const QByteArray& data, int start)
{
    if (start + 2 >= data.length()) {
        return 0;
    }

    // 小端序：低字节在前
    int value = static_cast<quint8>(data[start]) +
        static_cast<quint8>(data[start + 1]) * 256 +
        static_cast<quint8>(data[start + 2]) * 65536;

    return value;
}

// 静态方法版本
DataParser::ParsedData DataParser::parseDataStatic(const QByteArray& data)
{
    return instance().parseData(data);
}

// 单例实例
DataParser& DataParser::instance()
{
    static DataParser instance;
    return instance;
}
