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

    // 检查数据长度
    if (data.length() != 187) {
        qWarning() << "数据长度不正确:" << data.length() << "期望长度: 187";
        return result;
    }

    // 验证校验和
    if (!validateChecksum(data)) {
        qWarning() << "数据校验失败";
        return result;
    }

    try {
        // 解析序号 (前3字节) - 根据文档修正
        result.order_num = data.mid(0, 3).toHex().toUpper();

        // 解析电池数据 (3-5字节) - 根据文档修正
        result.battery = parseBatteryData(data, 3);

        // 解析音频数据 (5-65字节, 20个采样点，每个3字节) - 根据文档修正
        result.video_data.clear();
        for (int i = 0; i < 20; ++i) {
            int audioValue = parse24BitValue(data, 5 + i * 3);
            result.video_data.append(audioValue);
        }

        // 解析脑电数据 (65-135字节, 70字节) - 这是关键部分
        result.eeg_data = parseEEGData(data, 65, 70);

        // 解析跌倒标志 (135字节)
        result.is_fall = (data[135] != 0);

        // 解析四元数数据 (136-152字节, 4个float值)
        result.four_data.clear();
        for (int i = 0; i < 4; ++i) {
            // 按照文档说明，四元数是float类型，大端格式
            union {
                uint32_t intVal;
                float floatVal;
            } converter;

            converter.intVal = (static_cast<quint8>(data[136 + i * 4]) << 24) |
                (static_cast<quint8>(data[136 + i * 4 + 1]) << 16) |
                (static_cast<quint8>(data[136 + i * 4 + 2]) << 8) |
                static_cast<quint8>(data[136 + i * 4 + 3]);

            // 将float转换为int存储（可能需要根据实际需求调整）
            result.four_data.append(static_cast<int>(converter.floatVal * 1000)); // 放大1000倍保持精度
        }

        // 解析重力数据 (152-155字节, 3个字节)
        result.gravity_data.clear();
        for (int i = 0; i < 3; ++i) {
            int gravityValue = static_cast<int8_t>(data[152 + i]); // 有符号字节
            result.gravity_data.append(gravityValue);
        }

        // 预留数据 (155-159字节, 4字节)
        result.yuliu = data.mid(155, 4);

        // 光源数据 (159-177字节, 18字节，6个采样点)
        // 根据文档，每3字节一个采样点，这里暂时解析第一个采样点作为shine值
        result.shine = parse24BitValue(data, 159);

        // 校验值 (177字节)
        result.jiaoyan = static_cast<quint8>(data[177]);

        result.valid = true;
    }
    catch (...) {
        qWarning() << "数据解析异常";
        result.valid = false;
    }

    return result;
}

bool DataParser::validateChecksum(const QByteArray& data)
{
    if (data.length() < 2) return false;

    quint8 calculatedChecksum = 0;
    // 计算前177字节的校验和（根据文档，PSG_Data为179字节，但这里是177+8=185的格式）
    for (int i = 0; i < 177; ++i) {
        calculatedChecksum ^= static_cast<quint8>(data[i]);
    }

    quint8 receivedChecksum = static_cast<quint8>(data[177]);
    return calculatedChecksum == receivedChecksum;
}

QVector<int> DataParser::bytesToIntVector(const QByteArray& data, int start, int length)
{
    QVector<int> result;
    for (int i = 0; i < length; i += 2) {
        if (start + i + 1 < data.length()) {
            qint16 value = static_cast<qint16>((static_cast<quint8>(data[start + i + 1]) << 8) |
                static_cast<quint8>(data[start + i]));
            result.append(value);
        }
    }
    return result;
}

QVariantMap DataParser::parseBatteryData(const QByteArray& data, int start)
{
    QVariantMap battery;
    if (start + 2 <= data.length()) {
        // 解析电池数据（2字节）
        uint16_t batteryValue = (static_cast<quint8>(data[start + 1]) << 8) |
            static_cast<quint8>(data[start]);

        // 根据文档示例：80.25%（0X50 0X19）
        // 计算电池百分比
        float batteryPercent = batteryValue / 100.0f;
        battery["percentage"] = batteryPercent;
        battery["raw_value"] = batteryValue;
    }
    return battery;
}

DataParser::EEGChannelData DataParser::parseEEGData(const QByteArray& data, int start, int length)
{
    EEGChannelData eegData;

    // 根据文档和Python代码：
    // 脑电数据70字节，每7字节一组，共10组
    // 每组包含：fp1(3字节) + fp2(3字节) + event(1字节)

    int groupCount = length / 7; // 应该是10组

    for (int i = 0; i < groupCount; ++i) {
        int groupStart = start + i * 7;

        if (groupStart + 6 < data.length()) {
            // 解析FP1通道数据 (前3字节，小端序：L+M*256+H*65536)
            int fp1Value = parse24BitValue(data, groupStart);
            eegData.fp1.append(fp1Value);

            // 解析FP2通道数据 (中间3字节，小端序：L+M*256+H*65536)
            int fp2Value = parse24BitValue(data, groupStart + 3);
            eegData.fp2.append(fp2Value);

            // 解析事件标记 (第7字节)
            int eventValue = static_cast<quint8>(data[groupStart + 6]);
            eegData.events.append(eventValue);
        }
    }

    return eegData;
}

int DataParser::parse24BitValue(const QByteArray& data, int start)
{
    if (start + 2 >= data.length()) {
        return 0;
    }

    // 小端序：L + M*256 + H*65536
    int value = static_cast<quint8>(data[start]) |
        (static_cast<quint8>(data[start + 1]) << 8) |
        (static_cast<quint8>(data[start + 2]) << 16);

    // 处理24位有符号数（如果最高位为1，则为负数）
    if (value & 0x800000) {
        value |= 0xFF000000; // 符号扩展
    }

    return value;
}