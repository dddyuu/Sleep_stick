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
#include <QKeyEvent>
#include <cmath>
#include <functional>
#include <vector>
#include <algorithm>

MathGame::MathGame(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::MathGame),
    difficulty(0), // 初始化为0，预设实验时会自动设置
    timeLeft(100),
    totalTime(120), // 默认2分钟
    customExperimentTime(120), // 默认自定义时间也是2分钟
    answer(0),
    questionsAnswered(0),
    correctAnswers(0),
    accuracy(0.0),
    currentButtonIndex(0), // 默认选中按钮0
    keyboardNavigationEnabled(false),
    experimentMode(ExperimentMode::Single), // 默认单一模式
    currentPresetStageIndex(0),
    presetStageTimeLeft(0),
    presetDuration(10), // 默认每个难度60秒
    isPresetExperiment(false),
    currentStageQuestions(0),
    currentStageCorrect(0)
{
    ui->setupUi(this);

    // 设置初始状态
    updateTimeDisplay();
    ui->timeBar->setVisible(false);
    ui->equationLabel->setText("选择实验模式并开始实验");
    ui->resultLabel->setText("");
    ui->accuracyLabel->setText("准确率: 0%");
    ui->questionsLabel->setText("已答题: 0");
    ui->currentDifficultyLabel->setVisible(false);

    // 设置时间控件
    setupTimeSettings();

    // 设置模式按钮
    setupModeButtons();

    // 初始化计时器
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &MathGame::updateGameTimer);

    questionTimer = new QTimer(this);
    connect(questionTimer, &QTimer::timeout, this, &MathGame::updateQuestionTimer);

    presetStageTimer = new QTimer(this);
    connect(presetStageTimer, &QTimer::timeout, this, &MathGame::updatePresetStageTimer);

    // 设置难度选择按钮
    difficultyGroup = new QButtonGroup(this);
    difficultyGroup->addButton(ui->difficulty1Btn, 1);
    difficultyGroup->addButton(ui->difficulty2Btn, 2);
    difficultyGroup->addButton(ui->difficulty3Btn, 3);
    connect(difficultyGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
        this, &MathGame::difficultySelected);

    // 设置数字按钮和键盘导航
    numberGroup = new QButtonGroup(this);

    // 按0-9的顺序初始化按钮列表
    numberButtons.clear();
    // 先添加数字0按钮
    QPushButton* btn0 = findChild<QPushButton*>("numBtn0");
    if (btn0) {
        numberGroup->addButton(btn0, 0);
        numberButtons.append(btn0);
        btn0->setEnabled(false);
    }
    // 再添加数字1-9按钮
    for (int i = 1; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) {
            numberGroup->addButton(btn, i);
            numberButtons.append(btn);
            btn->setEnabled(false);
        }
    }

    connect(numberGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
        this, &MathGame::numberClicked);

    // 开始按钮
    connect(ui->startBtn, &QPushButton::clicked, this, &MathGame::startGame);

    // 重置按钮
    connect(ui->resetBtn, &QPushButton::clicked, this, &MathGame::resetGame);

    // 预设模式相关控件连接
    connect(ui->presetDurationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        [this](int value) { presetDuration = value; });

    // 设置焦点策略，使窗口能够接收键盘事件
    setFocusPolicy(Qt::StrongFocus);

    // 初始化UI状态
    updateModeUI();
}

MathGame::~MathGame()
{
    delete ui;
    delete gameTimer;
    delete questionTimer;
    delete presetStageTimer;
    delete difficultyGroup;
    delete numberGroup;
    delete modeGroup;
    delete presetOrderGroup;
}

void MathGame::setupModeButtons()
{
    // 设置模式按钮组 - 使用独立的ID
    modeGroup = new QButtonGroup(this);
    modeGroup->addButton(ui->singleModeBtn, 0);  // 单一模式使用ID 0
    modeGroup->addButton(ui->presetModeBtn, 1);  // 预设模式使用ID 1
    connect(modeGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
        this, &MathGame::onModeChanged);

    // 直接连接每个预设顺序按钮的点击信号，不使用按钮组
    connect(ui->lowToHighBtn, &QPushButton::clicked, this, [this]() {
        qDebug() << "低到高按钮被直接点击";
        onPresetOrderSelectedDirect(ExperimentMode::PresetLowToHigh);
        });

    connect(ui->highToLowBtn, &QPushButton::clicked, this, [this]() {
        qDebug() << "高到低按钮被直接点击";
        onPresetOrderSelectedDirect(ExperimentMode::PresetHighToLow);
        });

    // 设置初始状态
    ui->singleModeBtn->setChecked(true);
    ui->lowToHighBtn->setChecked(true);

    // 重要：将 presetOrderGroup 设置为 nullptr，因为我们不再使用它
    presetOrderGroup = nullptr;

    qDebug() << "模式按钮初始化完成，单一模式选中，预设顺序默认为低到高";
}
void MathGame::onPresetOrderSelectedDirect(ExperimentMode mode)
{
    qDebug() << "直接预设顺序选择，模式:" << static_cast<int>(mode);

    // 设置按钮状态
    if (mode == ExperimentMode::PresetLowToHigh) {
        ui->lowToHighBtn->setChecked(true);
        ui->highToLowBtn->setChecked(false);
        qDebug() << "设置为低到高模式";
    }
    else {
        ui->highToLowBtn->setChecked(true);
        ui->lowToHighBtn->setChecked(false);
        qDebug() << "设置为高到低模式";
    }

    // 首先自动切换到预设模式
    ui->presetModeBtn->setChecked(true);
    ui->singleModeBtn->setChecked(false);

    // 更新实验模式
    experimentMode = mode;
    qDebug() << "自动切换到预设模式，实验模式设置为:" << static_cast<int>(experimentMode);

    // 更新按钮样式
    ui->singleModeBtn->setStyleSheet("background-color: #9E9E9E; color: white; border-radius: 10px;");
    ui->presetModeBtn->setStyleSheet("background-color: #2196F3; color: white; border-radius: 10px;");

    // 更新UI
    updateModeUI();
}
void MathGame::onModeChanged()
{
    int buttonId = modeGroup->checkedId();
    qDebug() << "模式按钮点击，buttonId:" << buttonId;

    if (buttonId == 0) {
        // 单一模式
        experimentMode = ExperimentMode::Single;
        qDebug() << "切换到单一模式";
    }
    else if (buttonId == 1) {
        // 预设模式 - 根据当前按钮状态确定顺序
        qDebug() << "切换到预设模式";

        // 检查当前选中的预设顺序按钮
        if (ui->lowToHighBtn->isChecked()) {
            experimentMode = ExperimentMode::PresetLowToHigh;
            qDebug() << "根据按钮状态设置为低到高模式";
        }
        else if (ui->highToLowBtn->isChecked()) {
            experimentMode = ExperimentMode::PresetHighToLow;
            qDebug() << "根据按钮状态设置为高到低模式";
        }
        else {
            // 默认设置为低到高
            experimentMode = ExperimentMode::PresetLowToHigh;
            ui->lowToHighBtn->setChecked(true);
            qDebug() << "默认设置为低到高模式";
        }
    }

    // 更新按钮样式
    ui->singleModeBtn->setStyleSheet(experimentMode == ExperimentMode::Single ?
        "background-color: #2196F3; color: white; border-radius: 10px;" :
        "background-color: #9E9E9E; color: white; border-radius: 10px;");

    ui->presetModeBtn->setStyleSheet(experimentMode != ExperimentMode::Single ?
        "background-color: #2196F3; color: white; border-radius: 10px;" :
        "background-color: #9E9E9E; color: white; border-radius: 10px;");

    updateModeUI();
    qDebug() << "最终实验模式设置为:" << static_cast<int>(experimentMode);
}

void MathGame::onPresetOrderSelected()
{
    // 先获取是哪个按钮被点击了（在按钮状态可能被改变之前）
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    int buttonId = -1;

    if (clickedButton == ui->lowToHighBtn) {
        buttonId = static_cast<int>(ExperimentMode::PresetLowToHigh);
        qDebug() << "检测到点击低到高按钮";
    }
    else if (clickedButton == ui->highToLowBtn) {
        buttonId = static_cast<int>(ExperimentMode::PresetHighToLow);
        qDebug() << "检测到点击高到低按钮";
    }
    else {
        // 备用方案：检查按钮组
        buttonId = presetOrderGroup->checkedId();
        qDebug() << "通过按钮组获取buttonId:" << buttonId;
    }

    qDebug() << "预设顺序按钮点击，确定的buttonId:" << buttonId;

    // 添加调试信息
    qDebug() << "lowToHighBtn是否选中:" << ui->lowToHighBtn->isChecked();
    qDebug() << "highToLowBtn是否选中:" << ui->highToLowBtn->isChecked();

    // 检查buttonId的有效性
    if (buttonId == -1) {
        qDebug() << "错误：无法确定点击的预设顺序按钮，默认使用低到高";
        buttonId = static_cast<int>(ExperimentMode::PresetLowToHigh);
    }

    // 确保正确的按钮被选中
    if (buttonId == static_cast<int>(ExperimentMode::PresetLowToHigh)) {
        ui->lowToHighBtn->setChecked(true);
        ui->highToLowBtn->setChecked(false);
    }
    else {
        ui->highToLowBtn->setChecked(true);
        ui->lowToHighBtn->setChecked(false);
    }

    // 首先自动切换到预设模式
    ui->presetModeBtn->setChecked(true);
    ui->singleModeBtn->setChecked(false);

    // 更新实验模式
    experimentMode = static_cast<ExperimentMode>(buttonId);
    qDebug() << "自动切换到预设模式，实验模式设置为:" << static_cast<int>(experimentMode);

    // 更新按钮样式
    ui->singleModeBtn->setStyleSheet("background-color: #9E9E9E; color: white; border-radius: 10px;");
    ui->presetModeBtn->setStyleSheet("background-color: #2196F3; color: white; border-radius: 10px;");

    // 更新UI
    updateModeUI();
}

void MathGame::updateModeUI()
{
    bool isSingleMode = (experimentMode == ExperimentMode::Single);
    ui->singleModeWidget->setVisible(isSingleMode);
    ui->presetModeWidget->setVisible(!isSingleMode);

    // 更新提示文本
    if (isSingleMode) {
        ui->equationLabel->setText("选择难度、设置时间并开始实验");
    }
    else {
        ui->equationLabel->setText("选择预设实验模式并开始实验");
    }

    qDebug() << "UI更新完成，单一模式:" << isSingleMode << "当前实验模式:" << static_cast<int>(experimentMode);
}

void MathGame::setupTimeSettings()
{
    // 连接时间设置控件的信号
    connect(ui->timeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &MathGame::onTimeChanged);

    // 设置初始值
    ui->timeSpinBox->setValue(customExperimentTime);
    ui->presetDurationSpinBox->setValue(presetDuration);
}

void MathGame::onTimeChanged(int value)
{
    customExperimentTime = value;
    updateTimeDisplay();
    qDebug() << "实验时长设置为:" << customExperimentTime << "秒";
}

void MathGame::updateTimeDisplay()
{
    if (isPresetExperiment) {
        ui->gameTimeDisplay->display(presetStageTimeLeft);
    }
    else {
        ui->gameTimeDisplay->display(customExperimentTime);
    }
}

void MathGame::updateCurrentDifficultyDisplay()
{
    QString difficultyText;
    switch (difficulty) {
    case 1: difficultyText = "当前难度: 一级"; break;
    case 2: difficultyText = "当前难度: 二级"; break;
    case 3: difficultyText = "当前难度: 三级"; break;
    default: difficultyText = "当前难度: --"; break;
    }
    ui->currentDifficultyLabel->setText(difficultyText);

    // 根据难度设置不同颜色
    QString styleSheet;
    switch (difficulty) {
    case 1: styleSheet = "background-color: #4CAF50; color: white; border-radius: 5px; padding: 5px;"; break;
    case 2: styleSheet = "background-color: #2196F3; color: white; border-radius: 5px; padding: 5px;"; break;
    case 3: styleSheet = "background-color: #FF9800; color: white; border-radius: 5px; padding: 5px;"; break;
    default: styleSheet = "background-color: #9E9E9E; color: white; border-radius: 5px; padding: 5px;"; break;
    }
    ui->currentDifficultyLabel->setStyleSheet(styleSheet);
}

void MathGame::keyPressEvent(QKeyEvent* event)
{
    // 只在游戏进行中且数字按钮启用时处理键盘事件
    if (!keyboardNavigationEnabled) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_Down:
        moveToNextButton();
        break;
    case Qt::Key_Left:
    case Qt::Key_Up:
        moveToPrevButton();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        selectCurrentButton();
        break;
    case Qt::Key_0:
        currentButtonIndex = 0;
        updateButtonFocus();
        break;
    case Qt::Key_1:
        currentButtonIndex = 1;
        updateButtonFocus();
        break;
    case Qt::Key_2:
        currentButtonIndex = 2;
        updateButtonFocus();
        break;
    case Qt::Key_3:
        currentButtonIndex = 3;
        updateButtonFocus();
        break;
    case Qt::Key_4:
        currentButtonIndex = 4;
        updateButtonFocus();
        break;
    case Qt::Key_5:
        currentButtonIndex = 5;
        updateButtonFocus();
        break;
    case Qt::Key_6:
        currentButtonIndex = 6;
        updateButtonFocus();
        break;
    case Qt::Key_7:
        currentButtonIndex = 7;
        updateButtonFocus();
        break;
    case Qt::Key_8:
        currentButtonIndex = 8;
        updateButtonFocus();
        break;
    case Qt::Key_9:
        currentButtonIndex = 9;
        updateButtonFocus();
        break;
    default:
        QWidget::keyPressEvent(event);
        break;
    }
}

void MathGame::moveToNextButton()
{
    currentButtonIndex++;
    if (currentButtonIndex > 9) { // 0-9循环
        currentButtonIndex = 0;
    }
    updateButtonFocus();
}

void MathGame::moveToPrevButton()
{
    currentButtonIndex--;
    if (currentButtonIndex < 0) {
        currentButtonIndex = 9; // 回到按钮9
    }
    updateButtonFocus();
}

void MathGame::updateButtonFocus()
{
    // 清除所有按钮的高亮样式
    for (QPushButton* btn : numberButtons) {
        if (btn && btn->isEnabled()) {
            btn->setStyleSheet("background-color: white; border-radius: 10px;");
        }
    }

    // 高亮当前选中的按钮
    QPushButton* currentBtn = nullptr;
    if (currentButtonIndex >= 0 && currentButtonIndex <= 9) {
        // 按钮0-9，直接使用索引
        currentBtn = numberButtons[currentButtonIndex];
    }

    if (currentBtn && currentBtn->isEnabled()) {
        currentBtn->setStyleSheet("background-color: #2196F3; color: white; border-radius: 10px; border: 3px solid #1976D2;");
    }
}

void MathGame::selectCurrentButton()
{
    int number = currentButtonIndex; // 直接使用currentButtonIndex作为数字值

    // 模拟按钮点击
    numberClicked(number);
}

void MathGame::difficultySelected(int level)
{
    // 只在单一模式下处理难度选择
    if (experimentMode == ExperimentMode::Single) {
        difficulty = level;
        QString text;
        switch (level) {
        case 1: text = "一级难度: 两个数的加减法"; break;
        case 2: text = "二级难度: 三个数的加减乘除"; break;
        case 3: text = "三级难度: 四个数的加减乘除"; break;
        }
        ui->difficultyLabel->setText(text);
        qDebug() << "单一模式选择难度:" << level;
    }
}

void MathGame::initializePresetStages()
{
    presetStages.clear();

    QVector<int> difficulties;
    if (experimentMode == ExperimentMode::PresetLowToHigh) {
        difficulties = { 1, 2, 3 }; // 低到高
    }
    else if (experimentMode == ExperimentMode::PresetHighToLow) {
        difficulties = { 3, 2, 1 }; // 高到低
    }
    else {
        qDebug() << "错误：未知的预设实验模式:" << static_cast<int>(experimentMode);
        return;
    }

    qDebug() << "初始化预设阶段，模式:" << static_cast<int>(experimentMode) << "难度顺序:" << difficulties;

    for (int diff : difficulties) {
        PresetStage stage;
        stage.difficulty = diff;
        stage.duration = presetDuration;
        stage.questionsCount = 0;
        stage.correctCount = 0;
        presetStages.append(stage);
    }

    currentPresetStageIndex = 0;
    if (!presetStages.isEmpty()) {
        difficulty = presetStages[0].difficulty;
        presetStageTimeLeft = presetStages[0].duration;
        qDebug() << "预设第一阶段：难度" << difficulty << "时间" << presetStageTimeLeft;
    }
    else {
        qDebug() << "警告：预设阶段为空！";
    }

    qDebug() << "初始化预设阶段完成，共" << presetStages.size() << "个阶段，第一阶段难度" << difficulty << "时间" << presetStageTimeLeft;
}

void MathGame::setupPresetExperiment()
{
    qDebug() << "开始设置预设实验，当前模式:" << static_cast<int>(experimentMode);

    isPresetExperiment = true;
    presetDuration = ui->presetDurationSpinBox->value();

    qDebug() << "预设实验每阶段时长:" << presetDuration << "秒";

    initializePresetStages();

    // 检查是否成功初始化
    if (presetStages.isEmpty() || difficulty == 0) {
        qDebug() << "预设实验初始化失败！";
        isPresetExperiment = false;
        return;
    }

    updateCurrentDifficultyDisplay();

    // 计算总实验时间
    totalTime = presetStages.size() * presetDuration;
    customExperimentTime = totalTime;

    qDebug() << "预设实验设置完成，总时长:" << totalTime << "秒，每阶段:" << presetDuration << "秒，当前难度:" << difficulty;
}

void MathGame::switchToNextPresetStage()
{
    if (currentPresetStageIndex < presetStages.size()) {
        // 保存当前阶段统计
        presetStages[currentPresetStageIndex].questionsCount = currentStageQuestions;
        presetStages[currentPresetStageIndex].correctCount = currentStageCorrect;
    }

    currentPresetStageIndex++;
    if (currentPresetStageIndex >= presetStages.size()) {
        // 所有阶段完成
        qDebug() << "所有预设阶段完成";

        // 发送预设实验结束标签
        QString endTag;
        if (experimentMode == ExperimentMode::PresetLowToHigh) {
            endTag = "54";
        }
        else {
            endTag = "62";
        }
        emit tagSent(endTag);
        qDebug() << "发送预设实验结束标签:" << endTag;

        gameTimer->stop();
        presetStageTimer->stop();
        questionTimer->stop();

        // 保存预设实验数据
        savePresetExperimentData();

        showResults();
        return;
    }

    // 切换到下一个阶段
    difficulty = presetStages[currentPresetStageIndex].difficulty;
    presetStageTimeLeft = presetStages[currentPresetStageIndex].duration;
    currentStageQuestions = 0;
    currentStageCorrect = 0;

    updateCurrentDifficultyDisplay();

    // 发送难度切换标签
    QString switchTag;
    switch (difficulty) {
    case 1: switchTag = "51"; break;  // 切换到难度一
    case 2: switchTag = "52"; break;  // 切换到难度二
    case 3: switchTag = "53"; break;  // 切换到难度三
    }
    emit tagSent(switchTag);
    qDebug() << "切换到阶段" << currentPresetStageIndex + 1 << "难度" << difficulty << "标签:" << switchTag;

    // 继续下一题
    newQuestion();
}

void MathGame::updatePresetStageTimer()
{
    presetStageTimeLeft--;
    updateTimeDisplay();

    if (presetStageTimeLeft <= 0) {
        qDebug() << "当前阶段时间结束，切换到下一阶段";
        switchToNextPresetStage();
    }
}

void MathGame::startGame()
{
    qDebug() << "开始游戏，当前模式:" << static_cast<int>(experimentMode);

    if (experimentMode == ExperimentMode::Single) {
        // 单一难度模式需要检查是否选择了难度
        if (difficulty == 0) {
            QMessageBox::warning(this, "提示", "请先选择难度!");
            return;
        }

        // 单一难度模式
        isPresetExperiment = false;
        customExperimentTime = ui->timeSpinBox->value();

        // 发送游戏开始标签
        QString startTag;
        switch (difficulty) {
        case 1: startTag = "11"; break;  // 难度一开始标签
        case 2: startTag = "21"; break;  // 难度二开始标签
        case 3: startTag = "31"; break;  // 难度三开始标签
        default: startTag = "11"; break;
        }
        emit tagSent(startTag);
        qDebug() << "发送单一模式游戏开始标签:" << startTag << "实验时长:" << customExperimentTime << "秒";

        // 单一模式：先重置状态再设置
        resetGameInternal();

        // 设置单一模式的时间
        totalTime = customExperimentTime;
    }
    else {
        // 预设实验模式：先设置预设数据，再重置其他状态
        setupPresetExperiment();

        // 检查预设实验是否设置成功
        if (!isPresetExperiment || difficulty == 0 || presetStages.isEmpty()) {
            QMessageBox::warning(this, "错误", QString("预设实验初始化失败!\n模式: %1\n难度: %2\n阶段数: %3")
                .arg(static_cast<int>(experimentMode))
                .arg(difficulty)
                .arg(presetStages.size()));
            return;
        }

        // 发送预设实验开始标签
        QString startTag;
        if (experimentMode == ExperimentMode::PresetLowToHigh) {
            startTag = "51"; // 低到高预设实验开始
        }
        else {
            startTag = "61"; // 高到低预设实验开始
        }
        emit tagSent(startTag);
        qDebug() << "发送预设实验开始标签:" << startTag;

        // 预设模式：只重置游戏状态，保留预设数据
        resetGameInternalForPreset();
    }

    // 记录游戏开始时间
    gameStartTime = QDateTime::currentDateTime();
    questionRecords.clear();
    currentStageQuestions = 0;
    currentStageCorrect = 0;

    // 启用数字按钮和键盘导航
    keyboardNavigationEnabled = true;
    currentButtonIndex = 0; // 重置到按钮0
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) btn->setEnabled(true);
    }
    updateButtonFocus(); // 更新按钮焦点显示

    // 隐藏难度选择
    ui->difficultyWidget->setVisible(false);
    ui->startBtn->setVisible(false);

    // 显示游戏控件
    ui->timeBar->setVisible(true);
    ui->resetBtn->setVisible(true);
    ui->currentDifficultyLabel->setVisible(true);
    updateCurrentDifficultyDisplay();

    // 更新显示
    updateTimeDisplay();
    ui->timeBar->setValue(100);
    ui->accuracyLabel->setText("准确率: 0%");
    ui->questionsLabel->setText("已答题: 0");

    // 设置焦点到主窗口以接收键盘事件
    setFocus();

    // 开始计时
    if (isPresetExperiment) {
        presetStageTimer->start(1000); // 预设实验每秒更新阶段计时器
        qDebug() << "预设实验开始，第一阶段时间:" << presetStageTimeLeft;
    }
    else {
        gameTimer->start(1000); // 单一模式每秒更新总计时器
        qDebug() << "单一模式开始，总时间:" << totalTime;
    }

    newQuestion();
}

// 新增：用于预设实验的内部重置函数，不清空预设数据
void MathGame::resetGameInternalForPreset()
{
    // 停止所有计时器
    gameTimer->stop();
    questionTimer->stop();
    presetStageTimer->stop();

    // 禁用键盘导航
    keyboardNavigationEnabled = false;

    // 重置游戏统计状态，但保留预设实验数据
    timeLeft = 100;
    questionsAnswered = 0;
    correctAnswers = 0;
    accuracy = 0.0;
    currentStageQuestions = 0;
    currentStageCorrect = 0;

    // 更新显示
    ui->timeBar->setValue(timeLeft);
    ui->accuracyLabel->setText("准确率: 0%");
    ui->questionsLabel->setText("已答题: 0");

    // 禁用数字按钮并清除样式
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: white; border-radius: 10px;");
        }
    }

    qDebug() << "预设实验内部重置完成，保留难度:" << difficulty << "时间:" << presetStageTimeLeft;
}

// 新增：用于单一模式的内部重置函数
void MathGame::resetGameInternal()
{
    // 停止所有计时器
    gameTimer->stop();
    questionTimer->stop();
    presetStageTimer->stop();

    // 禁用键盘导航
    keyboardNavigationEnabled = false;

    // 重置游戏状态
    timeLeft = 100;
    questionsAnswered = 0;
    correctAnswers = 0;
    accuracy = 0.0;
    currentStageQuestions = 0;
    currentStageCorrect = 0;

    // 更新显示
    ui->timeBar->setValue(timeLeft);
    ui->accuracyLabel->setText("准确率: 0%");
    ui->questionsLabel->setText("已答题: 0");

    // 禁用数字按钮并清除样式
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: white; border-radius: 10px;");
        }
    }

    // 重置实验模式相关
    isPresetExperiment = false;
    currentPresetStageIndex = 0;
    presetStages.clear();

    qDebug() << "单一模式内部重置完成，难度:" << difficulty;
}

void MathGame::resetGame()
{
    // 停止所有计时器
    gameTimer->stop();
    questionTimer->stop();
    presetStageTimer->stop();

    // 禁用键盘导航
    keyboardNavigationEnabled = false;

    // 重置游戏状态
    timeLeft = 100;
    questionsAnswered = 0;
    correctAnswers = 0;
    accuracy = 0.0;
    currentStageQuestions = 0;
    currentStageCorrect = 0;

    // 重置难度到0（用于单一模式检查）
    if (experimentMode == ExperimentMode::Single) {
        difficulty = 0;
    }

    // 更新显示
    updateTimeDisplay();
    ui->timeBar->setValue(timeLeft);
    ui->accuracyLabel->setText("准确率: 0%");
    ui->questionsLabel->setText("已答题: 0");
    ui->equationLabel->setText("选择实验模式并开始实验");
    ui->resultLabel->setText("");

    // 显示难度选择
    ui->difficultyWidget->setVisible(true);
    ui->startBtn->setVisible(true);
    ui->currentDifficultyLabel->setVisible(false);

    // 禁用数字按钮并清除样式
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: white; border-radius: 10px;");
        }
    }

    // 隐藏游戏控件
    ui->timeBar->setVisible(false);
    ui->resetBtn->setVisible(false);

    // 重置实验模式相关
    isPresetExperiment = false;
    currentPresetStageIndex = 0;
    presetStages.clear();

    // 更新UI
    updateModeUI();

    qDebug() << "完全重置完成";
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

        // 禁用键盘导航
        keyboardNavigationEnabled = false;

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
        record.difficulty = difficulty; // 记录难度
        questionRecords.append(record);

        questionsAnswered++;
        currentStageQuestions++;
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
    record.difficulty = difficulty; // 记录难度
    questionRecords.append(record);

    questionsAnswered++;
    currentStageQuestions++;

    if (num == answer) {
        ui->resultLabel->setText("TRUE");
        ui->resultLabel->setStyleSheet("color: green; background-color: white;");
        correctAnswers++;
        currentStageCorrect++;
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
    // 检查是否为预设实验且需要切换阶段
    if (isPresetExperiment && presetStageTimeLeft <= 0) {
        switchToNextPresetStage();
        return;
    }

    // 检查单一模式时间是否结束
    if (!isPresetExperiment && totalTime <= 0) {
        showResults();
        return;
    }

    timeLeft = 100;
    ui->timeBar->setValue(timeLeft);
    ui->resultLabel->setStyleSheet("");
    ui->resultLabel->clear();

    generateEquation();

    // 重置按钮焦点到0
    currentButtonIndex = 0;
    updateButtonFocus();

    // 确保窗口有焦点以接收键盘事件
    setFocus();

    // 开始计时这道题
    questionElapsedTimer.start();

    questionTimer->start(150); // 每150ms更新一次进度条 (15秒总时间)
}

// 根据难度生成数字
int MathGame::generateNumber(int difficulty) {
    switch (difficulty) {
    case 1:
        // 一级难度：只生成1-9的单位数
        return QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
    case 2:
    case 3:
        // 二级和三级难度：生成1-99的数字（不超过两位数）
        return QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(100));
    default:
        return QRandomGenerator::global()->bounded(static_cast<quint32>(1), static_cast<quint32>(10));
    }
}

// 检查树中是否包含两位数
bool MathGame::hasDoubleDigit(Node* node) {
    if (!node) return false;

    if (node->type == Node::Number) {
        return node->value >= 10 && node->value <= 99;
    }

    return hasDoubleDigit(node->left) || hasDoubleDigit(node->right);
}

// 统计树中两位数的数量
int MathGame::countDoubleDigits(Node* node) {
    if (!node) return 0;

    if (node->type == Node::Number) {
        return (node->value >= 10 && node->value <= 99) ? 1 : 0;
    }

    return countDoubleDigits(node->left) + countDoubleDigits(node->right);
}

// 检查树中是否包含三位数
bool MathGame::hasThreeDigit(Node* node) {
    if (!node) return false;

    if (node->type == Node::Number) {
        return node->value >= 100;
    }

    return hasThreeDigit(node->left) || hasThreeDigit(node->right);
}

// 确保树中至少包含一个两位数
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

// 确保树中至少包含两个两位数
void MathGame::ensureTwoDoubleDigits(Node* node) {
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

    if (numberNodes.size() >= 2) {
        // 统计当前两位数的数量
        int currentDoubleDigits = countDoubleDigits(node);
        int needed = 2 - currentDoubleDigits;

        if (needed > 0) {
            // 收集所有非两位数的数字节点
            std::vector<Node*> singleDigitNodes;
            for (Node* n : numberNodes) {
                if (n->value < 10 || n->value > 99) {
                    singleDigitNodes.push_back(n);
                }
            }

            // 随机选择节点转换为两位数
            std::shuffle(singleDigitNodes.begin(), singleDigitNodes.end(),
                std::default_random_engine(QRandomGenerator::global()->generate()));

            for (int i = 0; i < needed && i < static_cast<int>(singleDigitNodes.size()); i++) {
                singleDigitNodes[i]->value = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(100)); // 10-99
            }
        }
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

        // 确保除数有效且不等于被除数
        if (r_val == 0) {
            adjustTreeValue(right, QRandomGenerator::global()->bounded(static_cast<quint32>(2), static_cast<quint32>(10)));
            right->evaluate(r_val); // 重新获取值
        }

        int l_val;
        if (!left->evaluate(l_val)) l_val = r_val * 2;

        // 调整左子树使其可整除，并确保不等于除数
        if (l_val % r_val != 0 || l_val == r_val) {
            int factor;
            do {
                factor = QRandomGenerator::global()->bounded(static_cast<quint32>(2), static_cast<quint32>(10));
            } while (factor == 1); // 确保因子不为1，这样结果就不会是1
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
    const int MAX_ATTEMPTS = 500; // 进一步增加尝试次数
    Node* root = nullptr;
    int val = -1;

    if (count > 2) {
        for (int i = 0; i < MAX_ATTEMPTS; ++i) {
            delete root;
            root = buildTree(count, 1); // 最顶层只能是加减

            // 对于二级和三级难度，确保至少包含两个两位数且没有三位数
            if (difficulty >= 2) {
                if (!hasThreeDigit(root) && countDoubleDigits(root) >= 2) {
                    // 已经满足条件
                }
                else if (!hasThreeDigit(root)) {
                    // 没有三位数但两位数不够，补充两位数
                    ensureTwoDoubleDigits(root);
                }
                else {
                    // 有三位数，重新生成
                    continue;
                }
            }

            if (root->evaluate(val) && val >= 0 && val <= 9)
                break;
        }

        // 如果超过最大尝试次数仍无法生成有效表达式，继续尝试
        while (!root || val < 0 || val > 9) {
            delete root;
            root = buildTree(count, 1);

            // 对于二级和三级难度，确保至少包含两个两位数且没有三位数
            if (difficulty >= 2) {
                if (!hasThreeDigit(root) && countDoubleDigits(root) >= 2) {
                    // 已经满足条件
                }
                else if (!hasThreeDigit(root)) {
                    // 没有三位数但两位数不够，补充两位数
                    ensureTwoDoubleDigits(root);
                }
                else {
                    // 有三位数，重新生成
                    continue;
                }
            }

            if (root && root->evaluate(val) && val >= 0 && val <= 9) {
                break;
            }
        }
    }
    else {
        // 两个数的情况
        int a, b;
        do {
            if (difficulty >= 2) {
                // 二级和三级难度：至少有两个两位数
                a = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(100)); // 第一个数是两位数
                b = QRandomGenerator::global()->bounded(static_cast<quint32>(10), static_cast<quint32>(100)); // 第二个数是两位数
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
                if (a + b <= 9) {
                    root = new Node('+', new Node(a), new Node(b));
                    val = a + b;
                    break;
                }
            }
            else {
                // 减法结果 >= 0 且 <= 9
                if (a >= b && a - b <= 9) {
                    root = new Node('-', new Node(a), new Node(b));
                    val = a - b;
                    break;
                }
                else if (b > a && b - a <= 9) {
                    root = new Node('-', new Node(b), new Node(a));
                    val = b - a;
                    break;
                }
            }
        } while (true); // 持续尝试直到生成有效表达式
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
    // 禁用数字按钮和键盘导航
    keyboardNavigationEnabled = false;
    for (int i = 0; i <= 9; i++) {
        QPushButton* btn = findChild<QPushButton*>(QString("numBtn%1").arg(i));
        if (btn) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: white; border-radius: 10px;");
        }
    }

    // 计算正确答题的平均时间
    double avgCorrectTime = calculateAverageCorrectTime();

    QString resultText;
    if (isPresetExperiment) {
        resultText = QString("预设实验结束!\n"
            "实验模式: %1\n"
            "每阶段时长: %2秒\n"
            "总答题数: %3\n"
            "正确答题数: %4\n"
            "准确率: %5%\n"
            "正确答题平均时间: %6秒\n"
            "数据已保存到Excel文件\n"
            "点击重置按钮重新开始")
            .arg(experimentMode == ExperimentMode::PresetLowToHigh ? "低→高难度" : "高→低难度")
            .arg(presetDuration)
            .arg(questionsAnswered)
            .arg(correctAnswers)
            .arg(accuracy, 0, 'f', 1)
            .arg(avgCorrectTime, 0, 'f', 2);
    }
    else {
        resultText = QString("实验结束!\n"
            "实验时长: %1秒\n"
            "总答题数: %2\n"
            "正确答题数: %3\n"
            "准确率: %4%\n"
            "正确答题平均时间: %5秒\n"
            "数据已保存到Excel文件\n"
            "点击重置按钮重新开始")
            .arg(customExperimentTime)
            .arg(questionsAnswered)
            .arg(correctAnswers)
            .arg(accuracy, 0, 'f', 1)
            .arg(avgCorrectTime, 0, 'f', 2);
    }

    ui->equationLabel->setText(resultText);
    ui->resultLabel->setText("");
    ui->timeBar->setVisible(false);
    ui->currentDifficultyLabel->setVisible(false);
}

void MathGame::savePresetExperimentData()
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
        stream << QString::fromUtf8("实验时间,实验模式,每阶段时长(秒),难度,阶段答题数,阶段正确数,阶段准确率(%),总答题数,总正确数,总准确率(%),正确答题平均时间(秒)\n");
    }

    // 计算数据
    double avgCorrectTime = calculateAverageCorrectTime();
    QString modeText = (experimentMode == ExperimentMode::PresetLowToHigh) ? "低到高" : "高到低";

    // 写入每个阶段的数据
    for (int i = 0; i < presetStages.size(); i++) {
        const PresetStage& stage = presetStages[i];
        double stageAccuracy = (stage.questionsCount > 0) ?
            static_cast<double>(stage.correctCount) / stage.questionsCount * 100 : 0.0;

        stream << gameStartTime.toString("yyyy-MM-dd hh:mm:ss") << ","
            << modeText << ","
            << presetDuration << ","
            << stage.difficulty << ","
            << stage.questionsCount << ","
            << stage.correctCount << ","
            << QString::number(stageAccuracy, 'f', 1) << ","
            << questionsAnswered << ","
            << correctAnswers << ","
            << QString::number(accuracy, 'f', 1) << ","
            << QString::number(avgCorrectTime, 'f', 2) << "\n";
    }

    file.close();
    qDebug() << "预设实验数据已保存到:" << filePath;

    // 显示保存成功的消息
    QMessageBox::information(this, "保存成功",
        QString("预设实验数据已成功保存到:\n%1").arg(filePath));
}

// 保存实验数据到Excel(CSV)文件
void MathGame::saveExperimentData()
{
    if (isPresetExperiment) {
        savePresetExperimentData();
        return;
    }

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
        stream << QString::fromUtf8("实验时间,实验模式,难度,实验时长(秒),总答题数,正确答题数,准确率(%),正确答题平均时间(秒)\n");
    }

    // 计算数据
    double avgCorrectTime = calculateAverageCorrectTime();

    // 写入实验数据，使用fromUtf8确保编码正确
    stream << gameStartTime.toString("yyyy-MM-dd hh:mm:ss") << ","
        << "单一难度" << ","
        << difficulty << ","
        << customExperimentTime << ","
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

// 计算正确答题的平均时间（排除错误和超时）
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

// 获取Excel文件路径
QString MathGame::getExcelFilePath()
{
    // 自定义路径 D:\SubEEG
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