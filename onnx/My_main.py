import os
import mne
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torch.utils.data import TensorDataset, DataLoader
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score, confusion_matrix, classification_report
from tqdm import tqdm
import matplotlib.pyplot as plt
import warnings
# from cross_subject.cross_subject_dataloader import *
from Index_calculation import *
from DAS_WSM.Mymodel.data_trainer import *

warnings.filterwarnings("ignore")

# 创建模型
n_channels, n_times = train_data.shape[1], train_data.shape[2]   # (61, 500)
model = HierarchicalCrossSubModel(n_channels, n_times, embed_dim=EMBED_DIM).to(DEVICE)

# 优化器和调度器
optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE, weight_decay=WEIGHT_DECAY)
scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='max', factor=0.5, patience=5, verbose=True)

# 训练和验证
best_val_acc = 0.0
early_stop_counter = 0
patience = 10

print("Start training model...")
min_acc = 0.3
for epoch in range(NUM_EPOCHS):
    train_loss, coarse_accuracy_1, fine_accuracy_1, coarse_accuracy_2, fine_accuracy_2, test_accuracy = train_epoch(
        model, train_loader, test_loader, optimizer, DEVICE, epoch, NUM_EPOCHS
    )
    if (test_accuracy) > min_acc:
        min_acc = test_accuracy
        torch.save(model.state_dict(), 'G:/博士成果/认知工作负荷/FP1_FP2模型/gr.pth')

    print(f"Epoch {epoch + 1}/{NUM_EPOCHS}:")
    print(f"  训练总损失: {train_loss:.4f}, 粗分类(低-中高)准确率: {coarse_accuracy_1:.4f}, 细分类（中高）准确率: {fine_accuracy_1:.4f}, 粗分类(低中-高)准确率: {coarse_accuracy_2:.4f}, "
          f"细分类（低中）准确率: {fine_accuracy_2:.4f}, "f"整体准确率：{test_accuracy:.4f}")
print(min_acc)