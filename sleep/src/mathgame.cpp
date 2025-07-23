#include "mathgame.h"
#include "ui_mathgame.h"

#include <QRandomGenerator>
#include <QGridLayout>
#include <QTime>
#include <QMessageBox>
#include <QString>
#include <QStringList>

#include <QStack>
#include <cmath>
#include <QDebug>

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

    // 重置游戏状态
    resetGame();

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
    //ui->gameTimeWidget->setVisible(true);
    //ui->statsWidget->setVisible(true);
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
    //ui->gameTimeWidget->setVisible(false);
    //ui->statsWidget->setVisible(false);
    ui->resetBtn->setVisible(false);
}

void MathGame::updateGameTimer()
{
    totalTime--;
    ui->gameTimeDisplay->display(totalTime);

    if (totalTime <= 0) {
        gameTimer->stop();
        questionTimer->stop();
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

        questionsAnswered++;
        updateAccuracy();

        QTimer::singleShot(1500, this, &MathGame::newQuestion);
    }
}

void MathGame::numberClicked(int num)
{
    questionTimer->stop();

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

    questionTimer->start(150); // 每150ms更新一次进度条 (15秒总时间)
}
Node* MathGame::buildTree(int numCount, int minPrecedence) {
    if (numCount == 1) {
        int val = QRandomGenerator::global()->bounded(1, 10); // 1-9
        return new Node(val);
    }

    int leftCount = QRandomGenerator::global()->bounded(1, numCount);
    int rightCount = numCount - leftCount;

    QVector<char> ops;
    if (minPrecedence <= 1) ops += {'+', '-'};
    if (minPrecedence <= 2) ops += {'*', '/'};

    char op = ops[QRandomGenerator::global()->bounded(ops.size())];

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
            adjustTreeValue(right, QRandomGenerator::global()->bounded(1, 10));
            right->evaluate(r_val); // 重新获取值
        }

        int l_val;
        if (!left->evaluate(l_val)) l_val = r_val;

        // 调整左子树使其可整除
        if (l_val % r_val != 0) {
            int factor = QRandomGenerator::global()->bounded(1, 5);
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
        node->value = newValue;
    }
    else {
        // 随机选择调整左子树或右子树
        if (QRandomGenerator::global()->bounded(2) == 0 && node->left) {
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
    if(count>2){
        for (int i = 0; i < MAX_ATTEMPTS; ++i) {
            delete root;
            root = buildTree(count, 1); // 最顶层只能是加减

            if (root->evaluate(val) && val >= 0 && val <= 9)
                break;
        }

        if (!root || val < 0 || val > 9) {
            // 后备方案：生成简单表达式
            int a = QRandomGenerator::global()->bounded(1, 10);
            int b = QRandomGenerator::global()->bounded(1, a+1); // 确保减法结果非负
            root = new Node('-', new Node(a), new Node(b));
            root->evaluate(val);
        }
    }
    else{
        int a = QRandomGenerator::global()->bounded(1, 10); // 1-9
        int b = QRandomGenerator::global()->bounded(1, 10); // 1-9

        // 随机选择 + 或 -
        bool add = QRandomGenerator::global()->bounded(2) == 0;
        if (add) {
            // 加法结果 <= 9
            while (a + b > 9) {
                a = QRandomGenerator::global()->bounded(1, 10);
                b = QRandomGenerator::global()->bounded(1, 10);
            }
            root = new Node('+', new Node(a), new Node(b));
            val = a + b;
        } else {
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
    ui->equationLabel->setText(expression + " = ?");
}

void MathGame::showResults()
{
    // 禁用数字按钮
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) btn->setEnabled(false);
    }

    // 显示结果
    QString resultText = QString("实验结束!\n"
        "总答题数: %1\n"
        "正确答题数: %2\n"
        "准确率: %3%\n"
        "点击重置按钮重新开始")
        .arg(questionsAnswered)
        .arg(correctAnswers)
        .arg(accuracy, 0, 'f', 1);

    ui->equationLabel->setText(resultText);
    ui->resultLabel->setText("");
    ui->timeBar->setVisible(false);
}
