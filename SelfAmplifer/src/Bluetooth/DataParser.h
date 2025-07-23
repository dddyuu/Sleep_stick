#ifndef DATAPARSER_H
#define DATAPARSER_H

#include <QObject>
#include <QByteArray>
#include <QVariantMap>
#include <QVector>
#include <QDebug>

/**
 * @brief 数据解析器类 - 解析来自蓝牙设备的原始数据
 */
class DataParser : public QObject
{
    Q_OBJECT

public:
    explicit DataParser(QObject* parent = nullptr);

    // 解析数据结构
    struct ParsedData {
        QString order_num;          // 序号
        QVariantMap battery;        // 电池数据
        QVector<int> video_data;    // 音频数据
        QVector<QVector<int>> eeg_data;  // 脑电数据
        bool is_fall;              // 跌倒标志
        QVector<int> four_data;    // 四路数据
        QVector<int> gravity_data; // 重力数据
        QByteArray yuliu;          // 预留数据
        int shine;                 // 闪烁值
        quint8 jiaoyan;            // 校验值
        QByteArray rawdata;        // 原始数据
        bool valid;                // 数据有效标志
    };

public slots:
    /**
     * @brief 解析接收到的数据
     * @param data 原始数据
     * @return 解析后的数据结构
     */
    static ParsedData parseData(const QByteArray& data);

private:
    /**
     * @brief 验证数据校验和
     * @param data 数据
     * @return 校验是否通过
     */
    static bool validateChecksum(const QByteArray& data);

    /**
     * @brief 字节数组转换为整数向量
     * @param data 字节数据
     * @param start 起始位置
     * @param length 长度
     * @return 整数向量
     */
    static QVector<int> bytesToIntVector(const QByteArray& data, int start, int length);

    /**
     * @brief 解析电池数据
     * @param data 字节数据
     * @param start 起始位置
     * @return 电池数据映射
     */
    static QVariantMap parseBatteryData(const QByteArray& data, int start);
};

#endif // DATAPARSER_H