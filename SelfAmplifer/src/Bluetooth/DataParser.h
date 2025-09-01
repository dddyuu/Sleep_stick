#ifndef DATAPARSER_H
#define DATAPARSER_H

#include <QObject>
#include <QByteArray>
#include <QVector>
#include <QVariantMap>
#include <QString>

class DataParser : public QObject
{
    Q_OBJECT

public:
    explicit DataParser(QObject* parent = nullptr);

    struct EEGChannelData {
        QVector<int> fp1;  // FP1通道数据
        QVector<int> fp2;  // FP2通道数据
        QVector<int> events; // 事件标记数据

        // 添加size()方法返回通道数量
        int size() const {
            return 2; // 固定返回2个通道（fp1和fp2）
        }

        // 添加数据点数量方法
        int dataPointCount() const {
            return fp1.size(); // 返回每个通道的数据点数量
        }
    };

    struct ParsedData {
        bool valid;
        QString order_num;
        int battery;
        QByteArray video_data;
        QByteArray eeg_data;  // 原始EEG数据（70字节）
        EEGChannelData parsed_eeg_data;  // 解析后的EEG数据
        bool is_fall;
        QByteArray four_data;
        int gravity_data;
        QByteArray yuliu;
        QByteArray shine;
        int jiaoyan;
        QByteArray rawdata;
        QString payload;  // 十六进制字符串表示
    };

    ParsedData parseData(const QByteArray& data);

    // 添加静态方法版本，方便直接调用
    static ParsedData parseDataStatic(const QByteArray& data);

    // 创建单例实例方法
    static DataParser& instance();
    // ADC最大值：2^23 = 8388608
    const int adcMaxVal = 8388608; // 2^23

    // 基准电压和增益
    const double baseVoltage = 4.5; // 基准电压
    const int eegFP1Magnification = 12; // FP1通道放大倍数
    const int eegFP2Magnification = 12; // FP2通道放大倍数
private:
    EEGChannelData parseEEGChannelData(const QByteArray& eegData);
    int parse24BitLittleEndian(const QByteArray& data, int start);
	//补码转原码函数
    int complement2Original(int rawData, int adcMaxVal);
    // 补码转电压函数
    double complementToVoltage(int rawData, int adcMaxVal, double baseVoltage, int gain);



};

#endif // DATAPARSER_H