#include "mathgame.h"
#include "ui_mathgame.h"

#include <QRandomGenerator>
#include <QGridLayout>
#include <QTime>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QStack>
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <functional>
#include <vector>
#include <algorithm>

MathGame::MathGame(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::MathGame),
    difficulty(1),
    timeLeft(100),
    totalTime(120), // 2分钟
    answer(0),
    questionsAnswered(0),
    correctAnswers(0),
    accuracy(0.0)
{
    ui->setupUi(this);

    // 设置初始状态
    ui->gameTimeDisplay->display(totalTime);
    ui->timeBar->setVisible(false);
    ui->equationLabel->setText("选择难度并开始实验");
    ui->resultLabel->setText("");
    ui->accuracyLabel->setText("准确率: 0%");
    ui->questionsLabel->setText("已答题: 0");

    // 初始化计时器
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &MathGame::updateGameTimer);

    questionTimer = new QTimer(this);
    connect(questionTimer, &QTimer::timeout, this, &MathGame::updateQuestionTimer);

    // 设置难度选择按钮
    difficultyGroup = new QButtonGroup(this);
    difficultyGroup->addButton(ui->difficulty1Btn, 1);
    difficultyGroup->addButton(ui->difficulty2Btn, 2);
    difficultyGroup->addButton(ui->difficulty3Btn, 3);
    connect(difficultyGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
        this, &MathGame::difficultySelected);

    // 设置数字按钮
    numberGroup = new QButtonGroup(this);
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) {
            numberGroup->addButton(btn, i);
            btn->setEnabled(false);
        }
    }
    connect(numberGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
        this, &MathGame::numberClicked);

    // 开始按钮
    connect(ui->startBtn, &QPushButton::clicked, this, &MathGame::startGame);

    // 重置按钮
    connect(ui->resetBtn, &QPushButton::clicked, this, &MathGame::resetGame);
}

MathGame::~MathGame()
{
    delete ui;
    delete gameTimer;
    delete questionTimer;
    delete difficultyGroup;
    delete numberGroup;
}

void MathGame::difficultySelected(int level)
{
    difficulty = level;
    QString text;
    switch (level) {
    case 1: text = "一级难度: 两个数的加减法"; break;
    case 2: text = "二级难度: 三个数的加减乘除"; break;
    case 3: text = "三级难度: 四个数的加减乘除"; break;
    }
    ui->difficultyLabel->setText(text);
}

void MathGame::startGame()
{
    if (difficulty == 0) {
        QMessageBox::warning(this, "提示", "请先选择难度!");
        return;
    }

    // 发送游戏开始标签
    QString startTag;
    switch (difficulty) {
    case 1: startTag = "11"; break;  // 难度一开始标签
    case 2: startTag = "21"; break;  // 难度二开始标签
    case 3: startTag = "31"; break;  // 难度三开始标签
    default: startTag = "11"; break;
    }
    emit tagSent(startTag);
    qDebug() << "发送游戏开始标签:" << startTag;

    // 重置游戏状态
    resetGame();

    // 记录游戏开始时间
    gameStartTime = QDateTime::currentDateTime();
    questionRecords.clear();

    // 启用数字按钮
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) btn->setEnabled(true);
    }

    // 隐藏难度选择
    ui->difficultyWidget->setVisible(false);
    ui->startBtn->setVisible(false);

    // 显示游戏控件
    ui->timeBar->setVisible(true);
    ui->resetBtn->setVisible(true);

    // 开始计时
    gameTimer->start(1000); // 每秒更新一次
    newQuestion();
}

void MathGame::resetGame()
{
    // 停止所有计时器
    gameTimer->stop();
    questionTimer->stop();

    // 重置游戏状态
    totalTime = 120;
    timeLeft = 100;
    questionsAnswered = 0;
    correctAnswers = 0;
    accuracy = 0.0;

    // 更新显示
    ui->gameTimeDisplay->display(totalTime);
    ui->timeBar->setValue(timeLeft);
    ui->accuracyLabel->setText("准确率: 0%");
    ui->questionsLabel->setText("已答题: 0");
    ui->equationLabel->setText("");
    ui->resultLabel->setText("");

    // 显示难度选择
    ui->difficultyWidget->setVisible(true);
    ui->startBtn->setVisible(true);

    // 禁用数字按钮
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) btn->setEnabled(false);
    }

    // 隐藏游戏控件
    ui->timeBar->setVisible(false);
    ui->resetBtn->setVisible(false);
}

void MathGame::updateGameTimer()
{
    totalTime--;
    ui->gameTimeDisplay->display(totalTime);

    if (totalTime <= 0) {
        // 发送游戏结束标签
        QString endTag;
        switch (difficulty) {
        case 1: endTag = "12"; break;  // 难度一结束标签
        case 2: endTag = "22"; break;  // 难度二结束标签
        case 3: endTag = "32"; break;  // 难度三结束标签
        default: endTag = "12"; break;
        }
        emit tagSent(endTag);
        qDebug() << "发送游戏结束标签:" << endTag;
        gameTimer->stop();
        questionTimer->stop();

        // 保存实验数据
        saveExperimentData();

        showResults();
    }
}

void MathGame::updateQuestionTimer()
{
    timeLeft--;
    ui->timeBar->setValue(timeLeft);

    if (timeLeft <= 0) {
        questionTimer->stop();
        ui->resultLabel->setText("TIMEOUT!");
        ui->resultLabel->setStyleSheet("color: blue; background-color: white;");

        // 记录超时题目
        QuestionRecord record;
        record.timestamp = QDateTime::currentDateTime();
        record.timeSpent = questionElapsedTimer.elapsed();
        record.isCorrect = false;
        record.isTimeout = true;
        record.question = currentQuestion;
        record.answer = answer;
        record.userAnswer = -1; // -1表示超时
        questionRecords.append(record);

        questionsAnswered++;
        updateAccuracy();

        QTimer::singleShot(1500, this, &MathGame::newQuestion);
    }
}

void MathGame::numberClicked(int num)
{
    questionTimer->stop();

    // 记录答题情况
    QuestionRecord record;
    record.timestamp = QDateTime::currentDateTime();
    record.timeSpent = questionElapsedTimer.elapsed();
    record.isCorrect = (num == answer);
    record.isTimeout = false;
    record.question = currentQuestion;
    record.answer = answer;
    record.userAnswer = num;
    questionRecords.append(record);

    questionsAnswered++;

    if (num == answer) {
        ui->resultLabel->setText("TRUE");
        ui->resultLabel->setStyleSheet("color: green; background-color: white;");
        correctAnswers++;
    }
    else {
        ui->resultLabel->setText("FALSE");
        ui->resultLabel->setStyleSheet("color: red; background-color: white;");
    }

    updateAccuracy();

    QTimer::singleShot(1500, this, &MathGame::newQuestion);
}

void MathGame::updateAccuracy()
{
    accuracy = (questionsAnswered > 0) ?
        static_cast<double>(correctAnswers) / questionsAnswered * 100 : 0.0;

    ui->accuracyLabel->setText(QString("准确率: %1%").arg(accuracy, 0, 'f', 1));
    ui->questionsLabel->setText(QString("已答题: %1").arg(questionsAnswered));
}

void MathGame::newQuestion()
{
    if (totalTime <= 0) {
        showResults();
        return;
    }

    timeLeft = 100;
    ui->timeBar->setValue(timeLeft);
    ui->resultLabel->setStyleSheet("");
    ui->resultLabel->clear();

    generateEquation();

    // 开始计时这道题
    questionElapsedTimer.start();

    questionTimer->start(150); // 每150ms更新一次进度条 (15秒总时间)
}

// 新增：根据难度生成数字
int MathGame::generateNumber(int difficulty) {
    switch (difficulty) {
    case 1:
        // 一级难度：只生成1-9的单位数
        return QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
    case 2:
    case 3:
        // 二级和三级难度：生成1-99的数字（包含两位数，不超过两位数）
        // 70%概率生成两位数(10-99)，30%概率生成单位数(1-9)
        if (QRandomGenerator::global()->bounded(static_cast<quint32>(100)) < 70) {
            return QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(100)); // 10-99
        }
        else {
            return QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10)); // 1-9
        }
    default:
        return QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
    }
}

// 新增：检查树中是否包含两位数
bool MathGame::hasDoubleDigit(Node* node) {
    if (!node) return false;

    if (node->type == Node::Number) {
        return node->value >= 10 && node->value <= 99;
    }

    return hasDoubleDigit(node->left) || hasDoubleDigit(node->right);
}

// 新增：确保树中至少包含一个两位数
void MathGame::ensureDoubleDigit(Node* node) {
    if (!node) return;

    // 收集所有数字节点
    std::vector<Node*> numberNodes;
    std::function<void(Node*)> collectNumbers = [&](Node* n) {
        if (!n) return;
        if (n->type == Node::Number) {
            numberNodes.push_back(n);
        }
        else {
            collectNumbers(n->left);
            collectNumbers(n->right);
        }
        };

    collectNumbers(node);

    if (!numberNodes.empty()) {
        // 随机选择一个数字节点，将其设置为两位数
        size_t randomIndex = QRandomGenerator::global()->bounded(static_cast<quint32>(numberNodes.size()));
        Node* selectedNode = numberNodes[randomIndex];
        selectedNode->value = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(100)); // 10-99
    }
}

Node* MathGame::buildTree(int numCount, int minPrecedence) {
    if (numCount == 1) {
        int val = generateNumber(difficulty);
        return new Node(val);
    }

    int leftCount = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(numCount));
    int rightCount = numCount - leftCount;

    QVector<char> ops;
    if (minPrecedence <= 1) ops += {'+', '-'};
    if (minPrecedence <= 2) ops += {'*', '/'};

    char op = ops[QRandomGenerator::global()->bounded(static_cast<quint32>(ops.size()))];

    Node* left = buildTree(leftCount, Node::precedence(op));
    Node* right = buildTree(rightCount, Node::precedence(op));

    // 使用 std::function 实现递归节点计数
    std::function<int(Node*)> countNodes;
    countNodes = [&countNodes](Node* node) -> int {
        if (!node) return 0;
        if (node->type == Node::Number) return 1;
        return countNodes(node->left) + countNodes(node->right);
        };

    int actualLeftCount = countNodes(left);
    int actualRightCount = countNodes(right);
    int actualTotal = actualLeftCount + actualRightCount;

    // 如果节点数量不足，重新生成子树
    if (actualLeftCount < leftCount || actualRightCount < rightCount) {
        qDebug() << "Node count mismatch. Expected:" << leftCount << "+"
            << rightCount << "=" << numCount << "Actual:"
            << actualLeftCount << "+" << actualRightCount << "=" << actualTotal;

        delete left;
        delete right;
        return buildTree(numCount, minPrecedence);
    }

    // 修复除法、减法合法性（不删除子树）
    if (op == '/') {
        int r_val;
        if (!right->evaluate(r_val)) r_val = 1;

        // 确保除数有效
        if (r_val == 0) {
            adjustTreeValue(right, QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10)));
            right->evaluate(r_val); // 重新获取值
        }

        int l_val;
        if (!left->evaluate(l_val)) l_val = r_val;

        // 调整左子树使其可整除
        if (l_val % r_val != 0) {
            int factor = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(5));
            adjustTreeValue(left, r_val * factor);
        }
    }
    else if (op == '-') {
        int l_val, r_val;
        if (left->evaluate(l_val) && right->evaluate(r_val)) {
            if (l_val < r_val) {
                swapTreeValues(left, right);
            }
        }
    }

    return new Node(op, left, right);
}

// 辅助函数：调整树的值而不改变结构
void MathGame::adjustTreeValue(Node* node, int newValue) {
    if (!node) return;

    if (node->type == Node::Number) {
        // 根据难度调整值的范围
        if (difficulty == 1) {
            // 一级难度：限制在1-9
            node->value = qMax(1, qMin(9, newValue));
        }
        else {
            // 二级和三级难度：限制在1-99
            node->value = qMax(1, qMin(99, newValue));
        }
    }
    else {
        // 随机选择调整左子树或右子树
        if (QRandomGenerator::global()->bounded(static_cast<quint32>(2)) == 0 && node->left) {
            adjustTreeValue(node->left, newValue);
        }
        else if (node->right) {
            adjustTreeValue(node->right, newValue);
        }
    }
}

// 辅助函数：交换两棵树的值
void MathGame::swapTreeValues(Node* a, Node* b) {
    if (!a || !b) return;

    if (a->type == Node::Number && b->type == Node::Number) {
        std::swap(a->value, b->value);
    }
    else {
        // 递归交换子树的值
        int a_val, b_val;
        if (a->evaluate(a_val) && b->evaluate(b_val)) {
            adjustTreeValue(a, b_val);
            adjustTreeValue(b, a_val);
        }
    }
}

// 计算树中节点数量
int MathGame::countTreeNodes(Node* node) {
    if (!node) return 0;
    if (node->type == Node::Number) return 1;
    return countTreeNodes(node->left) + countTreeNodes(node->right);
}

QString MathGame::generateExpression(int count) {
    const int MAX_ATTEMPTS = 100;
    Node* root = nullptr;
    int val = -1;

    if (count > 2) {
        for (int i = 0; i < MAX_ATTEMPTS; ++i) {
            delete root;
            root = buildTree(count, 1); // 最顶层只能是加减

            // 对于二级和三级难度，确保至少包含一个两位数
            if (difficulty >= 2 && !hasDoubleDigit(root)) {
                ensureDoubleDigit(root);
            }

            if (root->evaluate(val) && val >= 0 && val <= 9)
                break;
        }

        if (!root || val < 0 || val > 9) {
            // 后备方案：生成简单表达式
            int a, b;
            if (difficulty >= 2) {
                // 确保至少有一个两位数
                a = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(100)); // 10-99
                b = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(a + 1)); // 确保减法结果非负
            }
            else {
                a = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
                b = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(a + 1));
            }
            root = new Node('-', new Node(a), new Node(b));
            root->evaluate(val);
        }
    }
    else {
        int a, b;
        if (difficulty >= 2) {
            // 二级和三级难度：至少有一个两位数
            if (QRandomGenerator::global()->bounded(static_cast<quint32>(2)) == 0) {
                a = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(100)); // 第一个数是两位数
                b = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));   // 第二个数是单位数
            }
            else {
                a = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));   // 第一个数是单位数
                b = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(100)); // 第二个数是两位数
            }
        }
        else {
            // 一级难度：都是单位数
            a = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
            b = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
        }

        // 随机选择 + 或 -
        bool add = QRandomGenerator::global()->bounded(static_cast<quint32>(2)) == 0;
        if (add) {
            // 加法结果 <= 9
            while (a + b > 9) {
                if (difficulty >= 2) {
                    // 重新生成，保持至少一个两位数的要求
                    if (QRandomGenerator::global()->bounded(static_cast<quint32>(2)) == 0) {
                        int maxA = std::min(100, 10 - b);
                        if (maxA > 10) {
                            a = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(maxA));
                        }
                        else {
                            a = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
                        }
                        b = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
                    }
                    else {
                        a = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
                        int maxB = std::min(100, 10 - a);
                        if (maxB > 10) {
                            b = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(maxB));
                        }
                        else {
                            b = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
                        }
                    }
                }
                else {
                    a = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
                    b = QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
                }

                // 如果无法满足条件，切换到减法
                if (a + b > 9) {
                    add = false;
                    break;
                }
            }

            if (add) {
                root = new Node('+', new Node(a), new Node(b));
                val = a + b;
            }
        }

        if (!add) {
            // 减法结果 >= 0
            if (a < b) std::swap(a, b);
            root = new Node('-', new Node(a), new Node(b));
            val = a - b;
        }
    }

    QString expr;
    root->toString(expr);
    answer = val;
    delete root;

    // 调试输出
    qDebug() << "Generated expression:" << expr << "=" << val;

    return expr;
}

void MathGame::generateEquation()
{
    int expressionCount = 0;

    switch (difficulty) {
    case 1: expressionCount = 2; break;
    case 2: expressionCount = 3; break;
    case 3: expressionCount = 4; break;
    default: expressionCount = 2; break;
    }

    QString expression = generateExpression(expressionCount);
    currentQuestion = expression + " = ?";
    ui->equationLabel->setText(currentQuestion);
}

void MathGame::showResults()
{
    // 禁用数字按钮
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) btn->setEnabled(false);
    }

    // 计算正确答题的平均时间
    double avgCorrectTime = calculateAverageCorrectTime();

    // 显示结果
    QString resultText = QString("实验结束!\n"
        "总答题数: %1\n"
        "正确答题数: %2\n"
        "准确率: %3%\n"
        "正确答题平均时间: %4秒\n"
        "数据已保存到Excel文件\n"
        "点击重置按钮重新开始")
        .arg(questionsAnswered)
        .arg(correctAnswers)
        .arg(accuracy, 0, 'f', 1)
        .arg(avgCorrectTime, 0, 'f', 2);

    ui->equationLabel->setText(resultText);
    ui->resultLabel->setText("");
    ui->timeBar->setVisible(false);
}

// 新增：保存实验数据到Excel(CSV)文件
void MathGame::saveExperimentData()
{
    QString filePath = getExcelFilePath();
    QFile file(filePath);

    bool isNewFile = !file.exists();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "无法打开文件进行写入:" << filePath;
        QMessageBox::warning(this, "保存失败", "无法保存实验数据到文件");
        return;
    }

    QTextStream stream(&file);
    // 修复UTF-8编码问题
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
    // 添加UTF-8 BOM头，确保Excel正确识别中文
    if (isNewFile) {
        stream.setGenerateByteOrderMark(true);
    }

    // 如果是新文件，写入标题行
    if (isNewFile) {
        stream << QString::fromUtf8("实验时间,难度,总答题数,正确答题数,准确率(%),正确答题平均时间(秒)\n");
    }

    // 计算数据
    double avgCorrectTime = calculateAverageCorrectTime();

    // 写入实验数据，使用fromUtf8确保编码正确
    stream << gameStartTime.toString("yyyy-MM-dd hh:mm:ss") << ","
        << difficulty << ","
        << questionsAnswered << ","
        << correctAnswers << ","
        << QString::number(accuracy, 'f', 1) << ","
        << QString::number(avgCorrectTime, 'f', 2) << "\n";

    file.close();

    qDebug() << "实验数据已保存到:" << filePath;

    // 显示保存成功的消息
    QMessageBox::information(this, "保存成功",
        QString("实验数据已成功保存到:\n%1").arg(filePath));
}

// 新增：计算正确答题的平均时间（排除错误和超时）
double MathGame::calculateAverageCorrectTime()
{
    if (correctAnswers == 0) {
        return 0.0;
    }

    int totalCorrectTime = 0;
    int correctCount = 0;

    for (const auto& record : questionRecords) {
        if (record.isCorrect && !record.isTimeout) {
            totalCorrectTime += record.timeSpent;
            correctCount++;
        }
    }

    if (correctCount == 0) {
        return 0.0;
    }

    // 转换为秒
    return static_cast<double>(totalCorrectTime) / correctCount / 1000.0;
}

// 新增：获取Excel文件路径
QString MathGame::getExcelFilePath()
{
    // 使用自定义路径 D:\SubEEG
    QDir dir("D:/SubEEG");

    // 创建目录（如果不存在）
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "无法创建目录: D:/SubEEG";
            // 如果无法创建目录，退回到文档目录
            QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            QDir fallbackDir(documentsPath);
            if (!fallbackDir.exists("MathGameData")) {
                fallbackDir.mkpath("MathGameData");
            }
            return fallbackDir.absoluteFilePath("MathGameData/math_experiment_data.csv");
        }
    }

    return dir.absoluteFilePath("math_experiment_data.csv");
}