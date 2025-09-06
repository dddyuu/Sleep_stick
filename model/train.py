import torch
import warnings
import torch.optim as optim
from model_origin import *
from data_trainer import *

warnings.filterwarnings("ignore")

# 创建模型

def train_and_save_model(train_loader, test_loader, model_path):
    n_channels, n_times = 2, 250
    model = HierarchicalCrossSubModel(n_channels, n_times, embed_dim=EMBED_DIM).to(DEVICE)

    # 优化器和调度器
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE, weight_decay=WEIGHT_DECAY)
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='max', factor=0.5, patience=5, verbose=True)
    print("Start training model...")
    max_loss = 10
    for epoch in range(NUM_EPOCHS):
        train_loss = train_epoch(
            model, train_loader, test_loader, optimizer, DEVICE, epoch, NUM_EPOCHS
        )
        if (train_loss) < max_loss:
            max_loss = train_loss
            torch.save(model.state_dict(), model_path)

        print(f"Epoch {epoch + 1}/{NUM_EPOCHS}:")
        print(f"  训练总损失: {train_loss:.4f}")