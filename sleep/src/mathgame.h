#ifndef MATHGAME_H
#define MATHGAME_H

#include <QWidget>
#include <QTimer>
#include <QPushButton>
#include <QButtonGroup>
#include <QLCDNumber>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QRandomGenerator>
#include <QStack>
#include <QDateTime>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QVector>
#include <cmath>

namespace Ui {
    class MathGame;
}

// 题目回答记录结构
struct QuestionRecord {
    QDateTime timestamp;    // 题目开始时间
    int timeSpent;         // 答题用时(毫秒)
    bool isCorrect;        // 是否正确
    bool isTimeout;        // 是否超时
    QString question;      // 题目内容
    int answer;           // 正确答案
    int userAnswer;       // 用户答案(-1表示超时)
    int difficulty;       // 新增：题目难度
};

// 预设实验模式枚举
enum class ExperimentMode {
    Single,         // 单一难度模式
    PresetLowToHigh, // 预设模式：低到高难度
    PresetHighToLow  // 预设模式：高到低难度
};

// 预设实验阶段信息
struct PresetStage {
    int difficulty;     // 难度等级
    int duration;       // 持续时间(秒)
    int questionsCount; // 该阶段答题数量
    int correctCount;   // 该阶段正确数量
};

struct Node {
    enum Type { Number, Op } type;
    int value = 0;
    char op = 0;
    Node* left = nullptr;
    Node* right = nullptr;

    explicit Node(int v) : type(Number), value(v) {}
    explicit Node(char o, Node* l, Node* r) : type(Op), op(o), left(l), right(r) {}
    ~Node() { delete left; delete right; }

    static int precedence(char op) {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        return 0;
    }

    bool evaluate(int& out) const {
        if (type == Number) {
            if (value > 99)          //数字超限
                return false;
            out = value;
            return true;
        }

        int l_val, r_val;
        if (!left->evaluate(l_val) || !right->evaluate(r_val))
            return false;

        switch (op) {
        case '+': out = l_val + r_val; return true;
        case '-':
            if (l_val < r_val) return false;
            out = l_val - r_val;
            return true;
        case '*': out = l_val * r_val; return true;
        case '/':
            if (r_val == 0 || l_val % r_val != 0 || (l_val == r_val)) return false;
            out = l_val / r_val;
            return true;
        }
        return false;
    }

    void toString(QString& out) const {
        if (type == Number) {
            out += QString::number(value);
            return;
        }

        auto print = [](Node* child, QString& out, char parentOp, bool isRight) {
            bool needParen = false;
            if (child->type == Op) {
                int childPrec = Node::precedence(child->op);
                int parentPrec = Node::precedence(parentOp);

                // 规则1: 子节点优先级低于父节点时需要括号
                if (childPrec < parentPrec) {
                    needParen = true;
                }
                // 规则2: 相同优先级时处理结合性
                else if (childPrec == parentPrec) {
                    // 对于右子树，除法或减法的右操作数需要括号
                    if (isRight && (parentOp == '/' || parentOp == '-')) {
                        needParen = true;
                    }
                    // 对于左子树，除法或减法的左操作数通常不需要括号
                    // 但乘法的左操作数是加法时需要括号
                    else if (!isRight && (parentOp == '*' || parentOp == '/') &&
                        (child->op == '+' || child->op == '-')) {
                        needParen = true;
                    }
                }
            }

            if (needParen) out += "(";
            child->toString(out);
            if (needParen) out += ")";
            };

        // 处理左子树，传入当前运算符和false表示不是右子树
        print(left, out, op, false);

        out += " ";
        out += op;
        out += " ";

        // 处理右子树，传入当前运算符和true表示是右子树
        print(right, out, op, true);
    }
};

class MathGame : public QWidget
{
    Q_OBJECT

public:
    explicit MathGame(QWidget* parent = nullptr);
    ~MathGame();

signals:
    void tagSent(const QString& tag);  // 发送标签信号

protected:
    void keyPressEvent(QKeyEvent* event) override;  // 键盘事件处理

private slots:
    void updateGameTimer();
    void updateQuestionTimer();
    void updatePresetStageTimer();  // 新增：预设实验阶段计时器
    void numberClicked(int num);
    void startGame();
    void difficultySelected(int level);
    void newQuestion();
    void showResults();
    void onTimeChanged(int value);  // 时间设置改变的槽函数
    void onModeChanged();          // 新增：实验模式改变的槽函数
    void onPresetOrderSelected();   // 新增：预设顺序选择的槽函数

    void onPresetOrderSelectedDirect(ExperimentMode mode);   // 新增：直接预设顺序选择的槽函数
private:
    void generateEquation();
    QString generateExpression(int count);
    void updateAccuracy();
    void resetGame();
    void resetGameInternal();          // 新增：单一模式的内部重置函数
    void resetGameInternalForPreset(); // 新增：预设模式的内部重置函数
    void adjustTreeValue(Node* node, int newValue);
    void swapTreeValues(Node* a, Node* b);
    int countTreeNodes(Node* node);
    bool hasDoubleDigit(Node* node);  // 检查是否包含两位数
    void ensureDoubleDigit(Node* node);  // 确保包含两位数
    int countDoubleDigits(Node* node);  // 统计两位数的数量
    void ensureTwoDoubleDigits(Node* node);  // 确保至少有两个两位数
    bool hasThreeDigit(Node* node);  // 检查是否包含三位数
    int generateNumber(int difficulty);  // 根据难度生成数字

    // 键盘导航相关函数
    void updateButtonFocus();  // 更新按钮焦点显示
    void moveToNextButton();   // 移动到下一个按钮
    void moveToPrevButton();   // 移动到上一个按钮
    void selectCurrentButton(); // 选择当前按钮

    // 数据保存相关函数
    void saveExperimentData();
    double calculateAverageCorrectTime();
    QString getExcelFilePath();

    // 时间设置相关函数
    void setupTimeSettings();  // 设置时间控件
    void updateTimeDisplay();  // 更新时间显示

    // 新增：预设实验相关函数
    void setupPresetExperiment();     // 设置预设实验
    void initializePresetStages();    // 初始化预设实验阶段
    void switchToNextPresetStage();   // 切换到下一个预设阶段
    void updateCurrentDifficultyDisplay(); // 更新当前难度显示
    void savePresetExperimentData();  // 保存预设实验数据
    void setupModeButtons();          // 设置模式按钮
    void updateModeUI();             // 更新模式界面

    Ui::MathGame* ui;
    QTimer* gameTimer;              // 总游戏时间计时器
    QTimer* questionTimer;          // 单个问题计时器
    QTimer* presetStageTimer;       // 新增：预设实验阶段计时器
    QButtonGroup* difficultyGroup;
    QButtonGroup* numberGroup;
    QButtonGroup* modeGroup;        // 新增：模式按钮组
    QButtonGroup* presetOrderGroup; // 新增：预设顺序按钮组

    int difficulty;         // 1-3级难度
    int timeLeft;           // 当前问题剩余时间
    int totalTime;          // 总游戏剩余时间(秒)
    int customExperimentTime; // 用户自定义的实验时长(秒)
    int answer;             // 当前问题的答案
    int questionsAnswered;  // 已答题数量
    int correctAnswers;     // 正确答案数量
    double accuracy;        // 准确率
    QVector<int> userAnswers; // 用户答案列表

    // 键盘导航相关变量
    int currentButtonIndex; // 当前选中的按钮索引 (1-9对应1-9，0对应数字0)
    QList<QPushButton*> numberButtons; // 数字按钮列表，按1-9-0顺序排列
    bool keyboardNavigationEnabled; // 键盘导航是否启用

    // 题目记录相关
    QList<QuestionRecord> questionRecords;  // 所有题目记录
    QElapsedTimer questionElapsedTimer;     // 单题计时器
    QDateTime gameStartTime;                // 游戏开始时间
    QString currentQuestion;                // 当前题目内容

    // 新增：预设实验相关变量
    ExperimentMode experimentMode;          // 实验模式
    QVector<PresetStage> presetStages;     // 预设实验阶段列表
    int currentPresetStageIndex;           // 当前预设阶段索引
    int presetStageTimeLeft;               // 当前阶段剩余时间
    int presetDuration;                    // 每个难度的持续时间
    bool isPresetExperiment;               // 是否为预设实验
    int currentStageQuestions;             // 当前阶段答题数
    int currentStageCorrect;               // 当前阶段正确数

    Node* buildTree(int numCount, int depth);
    Node* randomNode(bool allowHighPriority);

    bool gameEnded = false; // 游戏是否结束
};

#endif // MATHGAME_H