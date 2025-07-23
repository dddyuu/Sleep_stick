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
        // 解析序号 (前4字节)
        result.order_num = data.mid(0, 4).toHex().toUpper();

        // 解析电池数据 (4-8字节)
        result.battery = parseBatteryData(data, 4);

        // 解析音频数据 (8-28字节, 10个int16值)
        result.video_data = bytesToIntVector(data, 8, 20);

        // 解析脑电数据 (28-148字节, 15通道×4字节)
        int eegStart = 28;
        for (int channel = 0; channel < 15; ++channel) {
            QVector<int> channelData;
            for (int i = 0; i < 2; ++i) { // 每通道2个int16值
                int value = static_cast<qint16>((static_cast<quint8>(data[eegStart + channel * 8 + i * 2 + 1]) << 8) |
                    static_cast<quint8>(data[eegStart + channel * 8 + i * 2]));
                channelData.append(value);
            }
            result.eeg_data.append(channelData);
        }

        // 解析跌倒标志 (148字节)
        result.is_fall = (data[148] != 0);

        // 解析四路数据 (149-165字节, 4个int32值)
        result.four_data.clear();
        for (int i = 0; i < 4; ++i) {
            int value = (static_cast<quint8>(data[149 + i * 4 + 3]) << 24) |
                (static_cast<quint8>(data[149 + i * 4 + 2]) << 16) |
                (static_cast<quint8>(data[149 + i * 4 + 1]) << 8) |
                static_cast<quint8>(data[149 + i * 4]);
            result.four_data.append(value);
        }

        // 解析重力数据 (165-171字节, 3个int16值)
        result.gravity_data = bytesToIntVector(data, 165, 6);

        // 预留数据 (171-183字节)
        result.yuliu = data.mid(171, 12);

        // 闪烁值 (183-185字节)
        result.shine = static_cast<qint16>((static_cast<quint8>(data[184]) << 8) |
            static_cast<quint8>(data[183]));

        // 校验值 (185字节)
        result.jiaoyan = static_cast<quint8>(data[185]);

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
    // 计算前185字节的校验和
    for (int i = 0; i < 185; ++i) {
        calculatedChecksum ^= static_cast<quint8>(data[i]);
    }

    quint8 receivedChecksum = static_cast<quint8>(data[185]);
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
    if (start + 4 <= data.length()) {
        // 解析电池相关数据（根据具体协议调整）
        battery["voltage"] = static_cast<quint16>((static_cast<quint8>(data[start + 1]) << 8) |
            static_cast<quint8>(data[start]));
        battery["current"] = static_cast<quint16>((static_cast<quint8>(data[start + 3]) << 8) |
            static_cast<quint8>(data[start + 2]));
    }
    return battery;
}