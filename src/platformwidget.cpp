#include "platformwidget.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>

PlatFormWidget::PlatFormWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::PlatFormWidget),
    m_preFcm(25, 2.0),  // 使用25个聚类中心
    m_fcmTrained(false), // 初始化训练状态为false
    m_trainingCounter(0)  // 初始化训练计数器为0
{
    ui->setupUi(this);
    initWidget();
    pre.setSize(512000, 1);
    setConnect();
    initFcmCenters(); // 初始化FCM聚类中心

}

PlatFormWidget::~PlatFormWidget()
{
    delete ui;
}

void PlatFormWidget::initWidget()
{
    this->setWindowFlag(Qt::FramelessWindowHint);
    mainwidget = new MainWidget;
    fatiguereswidget = new FatigueResWidget;
    chart_index = new Chart;
    mathgame = new MathGame;
    // fileStorage = new FileStorage;
    fileStorage = FileStorage::instance(this->parent());
    ui->Indexwidget->addWidget("主页", mainwidget);
    ui->Indexwidget->addWidget("采集", bciia.getMonitorWidget());
    ui->Indexwidget->addWidget("分析", fatiguereswidget);
    ui->Indexwidget->addWidget("范式", mathgame);

    QPalette palette;
    QPixmap pix(":/image/bg.png");
    palette.setBrush(QPalette::Window, pix);
    this->setPalette(palette);
}

void PlatFormWidget::initFcmCenters()
{
    // 尝试从文件加载预训练的聚类中心
    QFile file("fcm_centers.csv");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        std::vector<std::vector<double>> centers;
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList values = line.split(',');
            std::vector<double> center;
            for (const QString& val : values) {
                bool ok;
                double num = val.toDouble(&ok);
                if (ok) {
                    center.push_back(num);
                }
            }
            if (!center.empty()) {
                centers.push_back(center);
            }
        }

        if (centers.size() == 25) {
            try {
                m_preFcm.setCenters(centers);
                m_fcmTrained = true;  // 标记为已训练
                qDebug() << "成功从文件加载聚类中心";
            }
            catch (const std::exception& e) {
                qDebug() << "设置聚类中心失败:" << e.what();
            }
        }
        file.close();
    }
}

void PlatFormWidget::saveFcmCenters()
{
    try {
        auto centers = m_preFcm.getCenters();
        QFile file("fcm_centers.csv");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (const auto& center : centers) {
                for (size_t i = 0; i < center.size(); ++i) {
                    out << center[i];
                    if (i < center.size() - 1) out << ",";
                }
                out << "\n";
            }
            file.close();
            qDebug() << "聚类中心已保存到文件";
        }
    }
    catch (const std::exception& e) {
        qDebug() << "保存聚类中心失败:" << e.what();
    }
}

void PlatFormWidget::setConnect()
{
    // 转发被试名称
    connect(fatiguereswidget, &FatigueResWidget::transSubname, &bciia, &BCIIA::reciveSubName);
    connect(this, &PlatFormWidget::result_send, fatiguereswidget, &FatigueResWidget::sendData);
    //转发计算的label进行保存
    //connect(this, &PlatFormWidget::result_send, fileStorage, &FileStorage::appendLabel);
    // 新增：连接MathGame的标签信号并转发到FileStorage
    connect(mathgame, &MathGame::tagSent, this, [=](const QString& tag) {
        // 将QString标签转换为quint8并转发
        bool ok;
        int tagValue = tag.toUInt(&ok);
        if (ok) {
            qDebug() << "接收到MathGame标签:" << tag << "转换为:" << tagValue;
            fileStorage->appendEvent(tagValue);
        }
        else {
            qDebug() << "标签转换失败:" << tag;
        }
        });
    //connect(mathgame, &MathGame::tagSent, this, [=](const QString& tag) {
    //    // 将QString标签转换为quint8并转发
    //    bool ok;
    //    quint8 tagValue = tag.toUInt(&ok);
    //    if (ok) {
    //        qDebug() << "接收到MathGame标签:" << tag << "转换为:" << tagValue;
    //        fileStorage->appendLabel(tagValue);
    //    }
    //    else {
    //        qDebug() << "标签转换失败:" << tag;
    //    }
    //    });
    //转发本地label
    //connect(&bciia, &BCIIA::locallabelFinished, this, [=](QList<uint8_t> data){
    //    if(!is_ceived){
    //        is_ceived=1;
    //        xx1=data;
    //        localLabelIndex=0;
    //    }
    //});

    connect(&bciia, &BCIIA::chartDataFinished, this, [=](QList<double> data) {
        pre.append(data);
        // qDebug()<<"zhelishixx1"<<xx1.size();
        std::vector<float> input_value = pre.getOutput();

        if (!input_value.empty()) {
            try {
                // 1. 自动训练FCM模型
                qDebug() << m_fcmTrained << "\n";
                if (!m_fcmTrained) {
                    m_trainingData.push_back(input_value);
                    m_trainingCounter++;

                    // 当收集到足够样本后自动训练模型
                    if (m_trainingCounter >= 100) {
                        qDebug() << "自动训练FCM模型，样本数:" << m_trainingData.size();

                        // 自动训练FCM模型
                        m_preFcm.train(m_trainingData);
                        m_fcmTrained = true;

                        // 自动保存训练好的聚类中心
                        saveFcmCenters();

                        // 清空训练数据
                        m_trainingData.clear();
                        m_trainingCounter = 0;
                        qDebug() << "FCM模型训练完成";
                    }
                }

                // 2. 自动计算模糊特征（关键部分）
                std::vector<float> fuzzy_features;

                if (m_fcmTrained) {
                    // 使用FCM计算模糊特征
                    fuzzy_features = pre.zScore(m_preFcm.computeMembership(input_value));
                    qDebug() << "模糊化后进行输入";
                }
                else {
                    // FCM未训练时使用原始特征（临时方案）
                    fuzzy_features = pre.zScore(input_value);
                }

                // 3. 运行疲劳检测模型（使用模糊特征）
                tskfatifue.run(fuzzy_features);

                // 4. 获取疲劳检测结果
                std::vector<std::vector<float>> output_value = tskfatifue.getOutputValue();
                qDebug() << output_value;
                // 5. 处理分类结果（四分类）
                quint8 result = 0;
                quint8 temp = 0;
                for (int i = 1; i < 4; i++) {
                    if (output_value[0][i] > output_value[0][i - 1]) {
                        temp = i;
                    }
                }
                if (output_value[0][temp] > output_value[0][0]) {
                    result = temp;
                }
                if (result < 1) {
                    result = 0;
                }
                else {
                    result = 1;
                }
                //保存结果
                saveResult.append(result);
                // qDebug()<<saveResult.size();
              /*  if (output_value[0][0] >= output_value[0][1]) {
                    result = 0;
                }
                else {
                    result = 1;
                }*/
                // 6. 发送结果
                if(!xx1.isEmpty())
                    emit result_send(result, xx1[localLabelIndex++]);
                else
                    emit result_send(result,-1);
                qDebug() << "时间：" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                    << "疲劳检测结果：" << result;
            }

            catch (const std::exception& e) {
                qDebug() << "处理异常:" << e.what();
            }
        }
        });

    connect(ui->Indexwidget, &IndexWidget::closeSingal, this, [=]() {
        this->close();
        QApplication::quit();
        });

    connect(ui->Indexwidget, &IndexWidget::minSingal, this, [=]() {
        this->showMinimized();
        });

    connect(ui->Indexwidget, &IndexWidget::maxSingal, this, [=]() {
        this->showMaximized();
        });
}
