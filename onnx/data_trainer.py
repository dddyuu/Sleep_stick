import os
import numpy as np
import mne
from sklearn.preprocessing import StandardScaler
import torch
from DAS_WSM.loss_function import *
from sklearn.metrics import accuracy_score, confusion_matrix, classification_report
from DAS_WSM.Mymodel.Mymodel import *
from torch.utils.data import TensorDataset, DataLoader
from sklearn.linear_model import LogisticRegression

# 模型超参数
EMBED_DIM = 64
DROPOUT = 0.5
# BATCH_SIZE = 128
NUM_EPOCHS = 1000
LEARNING_RATE = 0.001
WEIGHT_DECAY = 1e-2

# 随机种子
SEED = 3407

# 设备
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

#跨会话
data = np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/data/gr.npy")
original_labels = np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/label/gr.npy")
# 标准化
# scaler = StandardScaler()
# data = np.array([scaler.fit_transform(x.T).T for x in data])

# 创建粗分类标签 (0=低负荷, 1=中高负荷；0=低中负荷，1=高负荷)
coarse_labels_1 = np.zeros_like(original_labels)
coarse_labels_1[original_labels > 0] = 1
coarse_labels_2 = np.zeros_like(original_labels)
coarse_labels_2[original_labels > 1] = 1

# 创建细分类标签 (0=中负荷, 1=高负荷；0=低负荷，1=中负荷)
fine_labels_1 = np.zeros_like(original_labels)
fine_labels_1[original_labels == 2] = 1
fine_labels_2 = np.zeros_like(original_labels)
# fine_labels_2[original_labels == 2] = 1
fine_labels_2[original_labels == 1] = 1

#训练集测试集划分
train_data = np.concatenate([data[0:60,:,:], data[120:180,:,:], data[240:300,:,:]])
train_coarse_labels_1 = np.concatenate([coarse_labels_1[0:60,], coarse_labels_1[120:180,], coarse_labels_1[240:300,]])
train_fine_labels_1 = np.concatenate([fine_labels_1[0:60,], fine_labels_1[120:180,], fine_labels_1[240:300,]])
train_coarse_labels_2 = np.concatenate([coarse_labels_2[0:60,], coarse_labels_2[120:180,], coarse_labels_2[240:300,]])
train_fine_labels_2 = np.concatenate([fine_labels_2[0:60,], fine_labels_2[120:180,], fine_labels_2[240:300,]])
train_original_labels = np.concatenate([original_labels[0:60,], original_labels[120:180,], original_labels[240:300,]])
test_data = np.concatenate([data[60:120,:,:], data[180:240,:,:], data[300:360,:,:]])
test_coarse_labels_1 = np.concatenate([coarse_labels_1[60:120,], coarse_labels_1[180:240,], coarse_labels_1[300:360,]])
test_fine_labels_1 = np.concatenate([fine_labels_1[60:120,], fine_labels_1[180:240,], fine_labels_1[300:360,]])
test_coarse_labels_2 = np.concatenate([coarse_labels_2[60:120,], coarse_labels_2[180:240,], coarse_labels_2[300:360,]])
test_fine_labels_2 = np.concatenate([fine_labels_2[60:120,], fine_labels_2[180:240,], fine_labels_2[300:360,]])
test_original_labels = np.concatenate([original_labels[60:120,], original_labels[180:240,], original_labels[300:360,]])

train_data = torch.tensor(train_data.real.astype(float), dtype=torch.float)
train_coarse_labels_1 = torch.tensor(train_coarse_labels_1, dtype=torch.int64)
train_fine_labels_1 = torch.tensor(train_fine_labels_1, dtype=torch.int64)
train_coarse_labels_2 = torch.tensor(train_coarse_labels_2, dtype=torch.int64)
train_fine_labels_2 = torch.tensor(train_fine_labels_2, dtype=torch.int64)
train_original_labels = torch.tensor(train_original_labels, dtype=torch.int64)
test_data = torch.tensor(test_data.real.astype(float), dtype=torch.float)
test_coarse_labels_1 = torch.tensor(test_coarse_labels_1, dtype=torch.int64)
test_fine_labels_1 = torch.tensor(test_fine_labels_1, dtype=torch.int64)
test_coarse_labels_2 = torch.tensor(test_coarse_labels_2, dtype=torch.int64)
test_fine_labels_2 = torch.tensor(test_fine_labels_2, dtype=torch.int64)
test_original_labels = torch.tensor(test_original_labels, dtype=torch.int64)

# 创建数据加载器
train_dataset = TensorDataset(train_data, train_coarse_labels_1, train_fine_labels_1, train_coarse_labels_2, train_fine_labels_2, train_original_labels)
test_dataset = TensorDataset(test_data, test_coarse_labels_1, test_fine_labels_1, test_coarse_labels_2, test_fine_labels_2, test_original_labels)
train_loader = DataLoader(train_dataset, batch_size=BATCH_SIZE, shuffle=True)
test_loader = DataLoader(test_dataset, batch_size=BATCH_SIZE, shuffle=True)

def train_epoch(model, source_loader, target_loader, optimizer, device, epoch, total_epochs):
    model.train()
    criterion_cls = LabelSmoothingLoss(classes=2, smoothing=0.1)  # 类别数2，平滑参数为0.1
    criterion_alignment = ClassAlignmentLoss()
    total_loss = coarse_loss_total = fine_loss_total = domain_loss_total = alignment_loss_total = 0
    total_train_acc = 0
    target_iter = iter(target_loader)

    for batch_idx, (source_data, source_coarse_labels_1, source_fine_labels_1, source_coarse_labels_2, source_fine_labels_2, source_original_labels) in enumerate(source_loader):
        try:
            target_data, _, _, _, _, _ = next(target_iter)
        except StopIteration:
            target_iter = iter(target_loader)
            target_data, _, _, _, _, _ = next(target_iter)

        source_data, source_coarse_labels_1, source_fine_labels_1, source_coarse_labels_2, source_fine_labels_2 = source_data.to(device), source_coarse_labels_1.to(
            device), source_fine_labels_1.to(device), source_coarse_labels_2.to(device), source_fine_labels_2.to(device)
        target_data = target_data.to(device)
        source_domain = torch.zeros(source_data.size(0), 1).to(device)  # 源域标签， 全为0
        target_domain = torch.ones(target_data.size(0), 1).to(device)  # 目标域标签，全为1

        optimizer.zero_grad()
        p = float(batch_idx + epoch * len(source_loader)) / (total_epochs * len(source_loader))  # 计算当前训练进度，范围从0到1
        alpha = 2.0 / (1.0 + np.exp(-10 * p)) - 1.0  # 2sigmoid(10p)-1， 自适应参数，渐进式策略

        source_coarse_output_1, source_fine_output_1, source_coarse_output_2, source_fine_output_2, source_domain_output, source_features = model(source_data, alpha)
        target_coarse_output_1, target_fine_output_1, target_coarse_output_2, target_fine_output_2, target_domain_output, target_features = model(target_data, alpha)

        target_coarse_pseudo_labels_1 = torch.argmax(target_coarse_output_1, dim=1)  # 为目标域数据生成粗粒度伪标签，取最大概率的类别
        target_fine_pseudo_labels_1 = torch.argmax(target_fine_output_1, dim=1)  # 为目标域数据生成细粒度伪标签，取最大概率的类别
        target_coarse_pseudo_labels_2 = torch.argmax(target_coarse_output_2, dim=1)  # 为目标域数据生成粗粒度伪标签，取最大概率的类别
        target_fine_pseudo_labels_2 = torch.argmax(target_fine_output_2, dim=1)  # 为目标域数据生成细粒度伪标签，取最大概率的类别

        coarse_loss_1 = criterion_cls(source_coarse_output_1, source_coarse_labels_1)  # 计算源域数据的粗粒度分类损失
        coarse_loss_2 = criterion_cls(source_coarse_output_2, source_coarse_labels_2)  # 计算源域数据的粗粒度分类损失

        medium_high_mask_1 = source_coarse_labels_1 == 1  # 生成布尔掩码，标识源域中粗粒度标签为1的样本(中高负荷样本)
        fine_loss_1 = criterion_cls(source_fine_output_1[medium_high_mask_1],
                                  source_fine_labels_1[medium_high_mask_1]) if medium_high_mask_1.sum() > 0 else torch.tensor(
            0.0).to(device)  # 如果有中高负荷样本，计算这些样本的细粒度分类损失；否则设为0
        medium_high_mask_2 = source_coarse_labels_2 == 0  # 生成布尔掩码，标识源域中粗粒度标签为0的样本(低中负荷样本)
        fine_loss_2 = criterion_cls(source_fine_output_2[medium_high_mask_2],
                                    source_fine_labels_2[medium_high_mask_2]) if medium_high_mask_2.sum() > 0 else torch.tensor(
            0.0).to(device)  # 如果有低中负荷样本，计算这些样本的细粒度分类损失；否则设为0


        domain_loss = criterion_domain(source_domain_output, source_domain) + criterion_domain(target_domain_output,
                                                                                               target_domain)
        alignment_loss_1 = criterion_alignment(source_features, target_features,
                                             [source_coarse_labels_1, source_fine_labels_1],
                                             [target_coarse_pseudo_labels_1, target_fine_pseudo_labels_1])  # 计算源域和目标域特征的对齐损失，使用真实标签和伪标签
        alignment_loss_2 = criterion_alignment(source_features, target_features,
                                              [source_coarse_labels_2, source_fine_labels_2],
                                              [target_coarse_pseudo_labels_2,target_fine_pseudo_labels_2])  # 计算源域和目标域特征的对齐损失，使用真实标签和伪标签

        lambda_coarse, lambda_fine, lambda_domain, lambda_alignment = 1.0, 1.0, 0.5 * (1 + np.exp(-10 * p)), 0.3
        loss = lambda_coarse * coarse_loss_1 + lambda_fine * fine_loss_1 + lambda_domain * domain_loss + lambda_alignment * alignment_loss_1 + lambda_coarse * coarse_loss_2 + lambda_fine * fine_loss_2 + lambda_alignment * alignment_loss_2

        loss.backward()
        optimizer.step()

        total_loss += loss.item()
        coarse_loss_total += coarse_loss_1.item() + coarse_loss_2.item()
        fine_loss_total += fine_loss_1.item() if isinstance(fine_loss_1, torch.Tensor) else 0
        fine_loss_total += fine_loss_2.item() if isinstance(fine_loss_2, torch.Tensor) else 0
        domain_loss_total += domain_loss.item()
        alignment_loss_total += alignment_loss_1.item() + alignment_loss_2.item()

        if batch_idx >= len(target_loader):
            break

    n_batches = min(len(source_loader), len(target_loader))
    train_loss, coarse_loss, fine_loss, domain_loss, alignment_loss = total_loss / n_batches, coarse_loss_total / n_batches, fine_loss_total / n_batches, domain_loss_total / n_batches, alignment_loss_total / n_batches

    model.eval()
    coarse_correct_1 = fine_correct_1 = coarse_correct_2 = fine_correct_2 = medium_high_total_1 = low_medium_total_2 = total = 0
    all_coarse_preds, all_coarse_labels, all_fine_preds, all_fine_labels = [], [], [], []
    all_original_preds, all_original_labels = [], []

    with torch.no_grad():
        for data, coarse_labels_1, fine_labels_1, coarse_labels_2, fine_labels_2, original_labels in test_loader:
            data, coarse_labels_1, fine_labels_1, coarse_labels_2, fine_labels_2 = data.to(device), coarse_labels_1.to(device), fine_labels_1.to(device), coarse_labels_2.to(device), fine_labels_2.to(device)
            coarse_output_1, fine_output_1, coarse_output_2, fine_output_2, _, _ = model(data)
            coarse_predicted_1 = torch.max(coarse_output_1, 1)[1]
            fine_predicted_1 = torch.max(fine_output_1, 1)[1]
            coarse_predicted_2 = torch.max(coarse_output_2, 1)[1]
            fine_predicted_2 = torch.max(fine_output_2, 1)[1]

            total += coarse_labels_1.size(0)
            coarse_correct_1 += (coarse_predicted_1 == coarse_labels_1).sum().item()
            medium_high_mask_1 = coarse_labels_1 == 1
            medium_high_total_1 += medium_high_mask_1.sum().item()
            if medium_high_mask_1.sum() > 0:
                fine_correct_1 += (fine_predicted_1[medium_high_mask_1] == fine_labels_1[medium_high_mask_1]).sum().item()

            coarse_correct_2 += (coarse_predicted_2 == coarse_labels_2).sum().item()
            low_medium_mask_2 = coarse_labels_2 == 0
            low_medium_total_2 += low_medium_mask_2.sum().item()
            if low_medium_mask_2.sum() > 0:
                fine_correct_2 += (fine_predicted_2[low_medium_mask_2] == fine_labels_2[low_medium_mask_2]).sum().item()

            original_preds_1 = convert_to_original_labels_1(coarse_predicted_1.cpu().numpy(), fine_predicted_1.cpu().numpy())
            original_preds_2 = convert_to_original_labels_2(coarse_predicted_2.cpu().numpy(), fine_predicted_2.cpu().numpy())

            # 需要真实标签来训练元模型
            X_meta = np.column_stack((original_preds_1, original_preds_2))
            meta_model = LogisticRegression()
            meta_model.fit(X_meta, original_labels)
            original_preds = meta_model.predict(X_meta)

            # 加权平均
            # original_preds = np.round(0.3 * original_preds_1 + 0.7 * original_preds_2).astype(int)


            # all_coarse_preds.extend(coarse_predicted.cpu().numpy())
            # all_coarse_labels.extend(coarse_labels.cpu().numpy())
            # all_fine_preds.extend(fine_predicted.cpu().numpy())
            # all_fine_labels.extend(fine_labels.cpu().numpy())
            all_original_preds.extend(original_preds)
            all_original_labels.extend(original_labels.numpy())

    coarse_accuracy_1 = coarse_correct_1 / total if total > 0 else 0
    fine_accuracy_1 = fine_correct_1 / medium_high_total_1 if medium_high_total_1 > 0 else 0
    coarse_accuracy_2 = coarse_correct_2 / total if total > 0 else 0
    fine_accuracy_2 = fine_correct_2 / low_medium_total_2 if low_medium_total_2 > 0 else 0
    test_accuracy = accuracy_score(all_original_labels, all_original_preds)
    # coarse_conf_matrix = confusion_matrix(all_coarse_labels, all_coarse_preds)
    # original_conf_matrix = confusion_matrix(all_original_labels, all_original_preds)
    # original_report = classification_report(all_original_labels, all_original_preds)

    return train_loss, coarse_accuracy_1, fine_accuracy_1, coarse_accuracy_2, fine_accuracy_2, test_accuracy



# def test(model, test_loader, device):
#     model.eval()
#     coarse_correct = fine_correct = medium_high_total = total = 0
#     all_coarse_preds, all_coarse_labels, all_fine_preds, all_fine_labels = [], [], [], []
#     all_original_preds, all_original_labels = [], []
#
#     with torch.no_grad():
#         for data, coarse_labels, fine_labels, original_labels in test_loader:
#             data, coarse_labels, fine_labels = data.to(device), coarse_labels.to(device), fine_labels.to(device)
#             coarse_output, fine_output, _, _ = model(data)
#             coarse_predicted = torch.max(coarse_output, 1)[1]
#             fine_predicted = torch.max(fine_output, 1)[1]
#
#             total += coarse_labels.size(0)
#             coarse_correct += (coarse_predicted == coarse_labels).sum().item()
#             medium_high_mask = coarse_labels == 1
#             medium_high_total += medium_high_mask.sum().item()
#             if medium_high_mask.sum() > 0:
#                 fine_correct += (fine_predicted[medium_high_mask] == fine_labels[medium_high_mask]).sum().item()
#
#             original_preds = convert_to_original_labels(coarse_predicted.cpu().numpy(), fine_predicted.cpu().numpy())
#             all_coarse_preds.extend(coarse_predicted.cpu().numpy())
#             all_coarse_labels.extend(coarse_labels.cpu().numpy())
#             all_fine_preds.extend(fine_predicted.cpu().numpy())
#             all_fine_labels.extend(fine_labels.cpu().numpy())
#             all_original_preds.extend(original_preds)
#             all_original_labels.extend(original_labels.numpy())
#
#     coarse_accuracy = coarse_correct / total if total > 0 else 0
#     fine_accuracy = fine_correct / medium_high_total if medium_high_total > 0 else 0
#     overall_accuracy = accuracy_score(all_original_labels, all_original_preds)
#     coarse_conf_matrix = confusion_matrix(all_coarse_labels, all_coarse_preds)
#     original_conf_matrix = confusion_matrix(all_original_labels, all_original_preds)
#     original_report = classification_report(all_original_labels, all_original_preds)
#
#     return coarse_accuracy, fine_accuracy, overall_accuracy, coarse_conf_matrix, original_conf_matrix, original_report