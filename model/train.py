import torch
import warnings
import torch.optim as optim
from Mymodel import *
from data_trainer import *

warnings.filterwarnings("ignore")

# 创建模型

def train_and_save_model(train_loader, test_loader, model_path):
    n_channels, n_times = 2, 500
    model = HierarchicalCrossSubModel(n_channels, n_times, embed_dim=EMBED_DIM).to(DEVICE)
    model.fusion_weight = nn.Parameter(torch.tensor([0.5, 0.5]), requires_grad=True).to(DEVICE)
    # 优化器和调度器
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE, weight_decay=WEIGHT_DECAY)
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='max', factor=0.5, patience=5, verbose=True)
    print("Start training model...")
    max_loss = 100
    min_acc = 0
    for epoch in range(NUM_EPOCHS):
        train_loss = train_epoch(
            model, train_loader, test_loader, optimizer, DEVICE, epoch, NUM_EPOCHS
        )

        test_acc,_ = test_predict(model,test_loader)
        if (test_acc) > min_acc:
            max_loss = train_loss
            min_acc = test_acc
            torch.save(model.state_dict(), model_path)

        print(f"Epoch {epoch + 1}/{NUM_EPOCHS}:")
        print(f"  训练总损失: {train_loss:.4f}")
        print(f"验证准确率:{test_acc:.4f}")
    print(f"best准确率：{min_acc:.4f}")

if __name__ == "__main__":
    train_npy_data_path = "D:\\SubEEG\\data\\data/grgg_0.npy"
    test_npy_data_path = "D:\\SubEEG\\data\\data/grdd_0.npy"
    train_npy_label = "D:\\SubEEG\\data\\label/grgg_0.npy"
    test_npy_label = "D:\\SubEEG\\data\\label/grdd_0.npy"
    model_path = "D:\\SubEEG\\model/gr.pth"
    # train_loader, test_loader = Pget_data_loaders(train_npy_data_path, train_npy_label, test_npy_data_path, test_npy_label)
    # print(1)
    # train_and_save_model(train_loader, test_loader, model_path)
    data = np.load("D:\\SubEEG\\data\\data/grgg_0.npy")
    online_data = data[:5]
    print(online_data)
    print(online_data.shape)
    # online_data = np.random.random((5, 2, 500))
    online_label = np.array([0, 0, 0, 0, 0])
    scaler = preprocessing.StandardScaler()
    scaler.fit(online_data.reshape(-1, 2 * 500))

    online_data = scaler.transform(online_data.reshape(-1, 2 * 500)).reshape(5, 2, 500)
    print(f"预处理后数据形状: {online_label.shape}")
    train_loader, test_loader = Pget_data_online_loaders(train_npy_data_path,train_npy_label, data, online_label)
    t = new_load_model_weights_predict(model_path, train_loader,test_loader)
    print("预测结果：", t)
