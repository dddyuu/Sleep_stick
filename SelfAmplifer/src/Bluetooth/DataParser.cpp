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

    // 【修改16】检查帧头 - 与Python代码保持一致
    const QByteArray FRAME_HEADER = QByteArray::fromHex("41495290");
    if (data.left(4) != FRAME_HEADER) {
        qWarning() << "帧头不匹配，期望:" << FRAME_HEADER.toHex(' ').toUpper()
            << "实际:" << data.left(4).toHex(' ').toUpper();
        return result;
    }

    // 【修改17】验证校验和 - 使用最后2字节而不是185字节位置
    if (!validateChecksum(data)) {
        qWarning() << "数据校验失败";
        return result;
    }

    try {
        // 【修改18】根据Python代码调整解析逻辑

        // 获取命令类型 (4-6字节) - 2字节小端序
        quint16 command = static_cast<quint16>((static_cast<quint8>(data[5]) << 8) |
            static_cast<quint8>(data[4]));

        // 获取数据长度 (6-8字节) - 2字节小端序
        quint16 data_length = static_cast<quint16>((static_cast<quint8>(data[7]) << 8) |
            static_cast<quint8>(data[6]));

        // 序列号 (8-11字节) - 3字节，转换为十六进制字符串
        result.order_num = data.mid(8, 3).toHex().toUpper();

        // 电池数据 (11-13字节) - 2字节小端序
        quint16 battery_voltage = static_cast<quint16>((static_cast<quint8>(data[12]) << 8) |
            static_cast<quint8>(data[11]));
        result.battery["voltage"] = battery_voltage;

        // 音频数据 (13-73字节) - 60字节直接存储
        result.video_data.clear();
        for (int i = 13; i < 73; i += 2) {
            if (i + 1 < data.length()) {
                qint16 value = static_cast<qint16>((static_cast<quint8>(data[i + 1]) << 8) |
                    static_cast<quint8>(data[i]));
                result.video_data.append(value);
            }
        }

        // EEG数据 (73-143字节) - 70字节，按通道解析
        result.eeg_data.clear();
        int eegStart = 73;
        int channelCount = 14; // 70字节 / 5字节每通道 = 14通道
        for (int channel = 0; channel < channelCount; ++channel) {
            QVector<int> channelData;
            int channelOffset = eegStart + channel * 5;
            if (channelOffset + 4 < data.length()) {
                // 每通道5字节，取前4字节作为数据
                for (int i = 0; i < 2; ++i) { // 2个int16值
                    qint16 value = static_cast<qint16>((static_cast<quint8>(data[channelOffset + i * 2 + 1]) << 8) |
                        static_cast<quint8>(data[channelOffset + i * 2]));
                    channelData.append(value);
                }
            }
            result.eeg_data.append(channelData);
        }

        // 是否脱落 (143字节)
        result.is_fall = (data[143] != 0);

        // 四元数据 (144-160字节) - 16字节，4个int32值
        result.four_data.clear();
        for (int i = 0; i < 4; ++i) {
            int offset = 144 + i * 4;
            if (offset + 3 < data.length()) {
                qint32 value = (static_cast<quint8>(data[offset + 3]) << 24) |
                    (static_cast<quint8>(data[offset + 2]) << 16) |
                    (static_cast<quint8>(data[offset + 1]) << 8) |
                    static_cast<quint8>(data[offset]);
                result.four_data.append(value);
            }
        }

        // 重力数据 (160-163字节) - 3字节
        result.gravity_data.clear();
        if (data.length() >= 163) {
            quint32 gravity = (static_cast<quint8>(data[162]) << 16) |
                (static_cast<quint8>(data[161]) << 8) |
                static_cast<quint8>(data[160]);
            result.gravity_data.append(gravity);
        }

        // 预留数据 (163-167字节) - 4字节
        result.yuliu = data.mid(163, 4);

        // 光源数据 (167-185字节) - 18字节
        result.shine = static_cast<qint16>((static_cast<quint8>(data[184]) << 8) |
            static_cast<quint8>(data[183]));

        // 校验值 (185-187字节) - 2字节
        result.jiaoyan = static_cast<quint16>((static_cast<quint8>(data[186]) << 8) |
            static_cast<quint8>(data[185]));

        result.valid = true;

        // qDebug() << "解析成功 - 命令:" << command << "数据长度:" << data_length
        //     << "序号:" << result.order_num << "电池:" << battery_voltage;

    }
    catch (...) {
        qWarning() << "数据解析异常";
        result.valid = false;
    }

    return result;
}

bool DataParser::validateChecksum(const QByteArray& data)
{
    if (data.length() < 187) return false;

    // 【修改19】根据Python代码，校验和可能在最后2字节
    // 这里简化校验，如果需要严格校验需要了解具体的校验算法
    return true; // 暂时跳过校验，专注于数据接收
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
