import numpy as np
import torch
from loss_function import *
from sklearn.metrics import accuracy_score
from model_origin import *
from torch.utils.data import TensorDataset, DataLoader
from sklearn.linear_model import LogisticRegression

# 模型超参数
EMBED_DIM = 64
DROPOUT = 0.5
BATCH_SIZE = 128
NUM_EPOCHS = 1000
LEARNING_RATE = 0.001
WEIGHT_DECAY = 1e-2

# 随机种子
SEED = 3407

# 设备
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

#跨会话

# def get_data_loaders(data_path,label_path,batch_size=128):
#     data = np.load(data_path)
#     original_labels = np.load(label_path)
#
#     # 创建粗分类标签 (0=低负荷, 1=中高负荷；0=低中负荷，1=高负荷)
#     coarse_labels_1 = np.zeros_like(original_labels)
#     coarse_labels_1[original_labels > 0] = 1
#     coarse_labels_2 = np.zeros_like(original_labels)
#     coarse_labels_2[original_labels > 1] = 1
#
#     # 创建细分类标签 (0=中负荷, 1=高负荷；0=低负荷，1=中负荷)
#     fine_labels_1 = np.zeros_like(original_labels)
#     fine_labels_1[original_labels == 2] = 1
#     fine_labels_2 = np.zeros_like(original_labels)
#     # fine_labels_2[original_labels == 2] = 1
#     fine_labels_2[original_labels == 1] = 1
#
#     #训练集测试集划分
#     train_data = np.concatenate([data[0:60,:,:], data[120:180,:,:], data[240:300,:,:]])
#     train_coarse_labels_1 = np.concatenate([coarse_labels_1[0:60,], coarse_labels_1[120:180,], coarse_labels_1[240:300,]])
#     train_fine_labels_1 = np.concatenate([fine_labels_1[0:60,], fine_labels_1[120:180,], fine_labels_1[240:300,]])
#     train_coarse_labels_2 = np.concatenate([coarse_labels_2[0:60,], coarse_labels_2[120:180,], coarse_labels_2[240:300,]])
#     train_fine_labels_2 = np.concatenate([fine_labels_2[0:60,], fine_labels_2[120:180,], fine_labels_2[240:300,]])
#     train_original_labels = np.concatenate([original_labels[0:60,], original_labels[120:180,], original_labels[240:300,]])
#     test_data = np.concatenate([data[60:120,:,:], data[180:240,:,:], data[300:360,:,:]])
#     test_coarse_labels_1 = np.concatenate([coarse_labels_1[60:120,], coarse_labels_1[180:240,], coarse_labels_1[300:360,]])
#     test_fine_labels_1 = np.concatenate([fine_labels_1[60:120,], fine_labels_1[180:240,], fine_labels_1[300:360,]])
#     test_coarse_labels_2 = np.concatenate([coarse_labels_2[60:120,], coarse_labels_2[180:240,], coarse_labels_2[300:360,]])
#     test_fine_labels_2 = np.concatenate([fine_labels_2[60:120,], fine_labels_2[180:240,], fine_labels_2[300:360,]])
#     test_original_labels = np.concatenate([original_labels[60:120,], original_labels[180:240,], original_labels[300:360,]])
#
#     train_data = torch.tensor(train_data.real.astype(float), dtype=torch.float)
#     train_coarse_labels_1 = torch.tensor(train_coarse_labels_1, dtype=torch.int64)
#     train_fine_labels_1 = torch.tensor(train_fine_labels_1, dtype=torch.int64)
#     train_coarse_labels_2 = torch.tensor(train_coarse_labels_2, dtype=torch.int64)
#     train_fine_labels_2 = torch.tensor(train_fine_labels_2, dtype=torch.int64)
#     train_original_labels = torch.tensor(train_original_labels, dtype=torch.int64)
#     test_data = torch.tensor(test_data.real.astype(float), dtype=torch.float)
#     test_coarse_labels_1 = torch.tensor(test_coarse_labels_1, dtype=torch.int64)
#     test_fine_labels_1 = torch.tensor(test_fine_labels_1, dtype=torch.int64)
#     test_coarse_labels_2 = torch.tensor(test_coarse_labels_2, dtype=torch.int64)
#     test_fine_labels_2 = torch.tensor(test_fine_labels_2, dtype=torch.int64)
#     test_original_labels = torch.tensor(test_original_labels, dtype=torch.int64)
#
#     # 创建数据加载器
#     train_dataset = TensorDataset(train_data, train_coarse_labels_1, train_fine_labels_1, train_coarse_labels_2, train_fine_labels_2, train_original_labels)
#     test_dataset = TensorDataset(test_data, test_coarse_labels_1, test_fine_labels_1, test_coarse_labels_2, test_fine_labels_2, test_original_labels)
#     train_loader = DataLoader(train_dataset, batch_size=BATCH_SIZE, shuffle=True)
#     test_loader = DataLoader(test_dataset, batch_size=BATCH_SIZE, shuffle=True)
#     return train_loader, test_loader
def get_data_loaders(data_path, label_path, batch_size=128):
    try:
        data = np.load(data_path)
        original_labels = np.load(label_path)

        print(f"原始数据形状: {data.shape}")
        print(f"原始标签形状: {original_labels.shape}")
        print(f"标签分布: {np.bincount(original_labels)}")

        # 验证数据完整性
        if len(data) == 0 or len(original_labels) == 0:
            raise ValueError("数据或标签为空")

        if len(data) != len(original_labels):
            raise ValueError(f"数据和标签长度不匹配: {len(data)} vs {len(original_labels)}")

        # 检查标签的唯一值
        unique_labels = np.unique(original_labels)
        print(f"唯一标签: {unique_labels}")

        if len(unique_labels) < 2:
            raise ValueError(f"标签类别不足，只有 {len(unique_labels)} 类")

        # 按类别分组数据
        class_0_indices = np.where(original_labels == 0)[0]
        class_1_indices = np.where(original_labels == 1)[0]
        class_2_indices = np.where(original_labels == 2)[0]

        print(f"类别0样本数: {len(class_0_indices)}")
        print(f"类别1样本数: {len(class_1_indices)}")
        print(f"类别2样本数: {len(class_2_indices)}")

        # 检查每个类别是否有足够的样本进行分割
        min_samples_per_class = 10  # 每类至少需要10个样本

        if len(class_0_indices) < min_samples_per_class:
            print(f"警告: 类别0样本数不足 ({len(class_0_indices)})，使用全部数据")
        if len(class_1_indices) < min_samples_per_class:
            print(f"警告: 类别1样本数不足 ({len(class_1_indices)})，使用全部数据")
        if len(class_2_indices) < min_samples_per_class:
            print(f"警告: 类别2样本数不足 ({len(class_2_indices)})，使用全部数据")

        # 动态分割数据
        def split_class_data(indices, train_ratio=0.7):
            if len(indices) == 0:
                return [], []

            np.random.shuffle(indices)  # 随机打乱
            split_point = int(len(indices) * train_ratio)
            split_point = max(1, min(split_point, len(indices) - 1))  # 确保训练集和测试集都有数据

            return indices[:split_point], indices[split_point:]

        # 分割每个类别的数据
        if len(class_0_indices) >= min_samples_per_class:
            train_indices_0, test_indices_0 = split_class_data(class_0_indices)
        else:
            # 如果样本太少，全部用于训练
            train_indices_0, test_indices_0 = class_0_indices, []

        if len(class_1_indices) >= min_samples_per_class:
            train_indices_1, test_indices_1 = split_class_data(class_1_indices)
        else:
            train_indices_1, test_indices_1 = class_1_indices, []

        if len(class_2_indices) >= min_samples_per_class:
            train_indices_2, test_indices_2 = split_class_data(class_2_indices)
        else:
            train_indices_2, test_indices_2 = class_2_indices, []

        # 合并训练和测试索引
        train_indices = np.concatenate([train_indices_0, train_indices_1, train_indices_2])
        test_indices = np.concatenate([test_indices_0, test_indices_1, test_indices_2])

        print(f"训练集索引数量: {len(train_indices)}")
        print(f"测试集索引数量: {len(test_indices)}")

        # 如果测试集为空，从训练集中分出一部分作为测试集
        if len(test_indices) == 0:
            print("测试集为空，从训练集中分出一部分")
            np.random.shuffle(train_indices)
            split_point = max(1, len(train_indices) // 5)  # 20%作为测试集
            test_indices = train_indices[:split_point]
            train_indices = train_indices[split_point:]
            print(f"重新分割后 - 训练集: {len(train_indices)}, 测试集: {len(test_indices)}")

        # 确保训练集不为空
        if len(train_indices) == 0:
            raise ValueError("训练集为空，无法进行训练")

        # 提取训练和测试数据
        train_data = data[train_indices]
        train_original_labels = original_labels[train_indices]
        test_data = data[test_indices] if len(test_indices) > 0 else data[train_indices[:min(10, len(train_indices))]]
        test_original_labels = original_labels[test_indices] if len(test_indices) > 0 else original_labels[
            train_indices[:min(10, len(train_indices))]]

        print(f"最终训练数据形状: {train_data.shape}")
        print(f"最终测试数据形状: {test_data.shape}")
        print(f"训练标签分布: {np.bincount(train_original_labels)}")
        print(f"测试标签分布: {np.bincount(test_original_labels)}")

        # 创建粗分类标签
        train_coarse_labels_1 = np.zeros_like(train_original_labels)
        train_coarse_labels_1[train_original_labels > 0] = 1
        train_coarse_labels_2 = np.zeros_like(train_original_labels)
        train_coarse_labels_2[train_original_labels > 1] = 1

        test_coarse_labels_1 = np.zeros_like(test_original_labels)
        test_coarse_labels_1[test_original_labels > 0] = 1
        test_coarse_labels_2 = np.zeros_like(test_original_labels)
        test_coarse_labels_2[test_original_labels > 1] = 1

        # 创建细分类标签
        train_fine_labels_1 = np.zeros_like(train_original_labels)
        train_fine_labels_1[train_original_labels == 2] = 1
        train_fine_labels_2 = np.zeros_like(train_original_labels)
        train_fine_labels_2[train_original_labels == 1] = 1

        test_fine_labels_1 = np.zeros_like(test_original_labels)
        test_fine_labels_1[test_original_labels == 2] = 1
        test_fine_labels_2 = np.zeros_like(test_original_labels)
        test_fine_labels_2[test_original_labels == 1] = 1

        # 转换为PyTorch张量
        train_data = torch.tensor(train_data.astype(np.float32), dtype=torch.float)
        train_coarse_labels_1 = torch.tensor(train_coarse_labels_1, dtype=torch.long)
        train_fine_labels_1 = torch.tensor(train_fine_labels_1, dtype=torch.long)
        train_coarse_labels_2 = torch.tensor(train_coarse_labels_2, dtype=torch.long)
        train_fine_labels_2 = torch.tensor(train_fine_labels_2, dtype=torch.long)
        train_original_labels = torch.tensor(train_original_labels, dtype=torch.long)

        test_data = torch.tensor(test_data.astype(np.float32), dtype=torch.float)
        test_coarse_labels_1 = torch.tensor(test_coarse_labels_1, dtype=torch.long)
        test_fine_labels_1 = torch.tensor(test_fine_labels_1, dtype=torch.long)
        test_coarse_labels_2 = torch.tensor(test_coarse_labels_2, dtype=torch.long)
        test_fine_labels_2 = torch.tensor(test_fine_labels_2, dtype=torch.long)
        test_original_labels = torch.tensor(test_original_labels, dtype=torch.long)

        # 动态调整批次大小
        actual_batch_size = min(batch_size, len(train_data))
        if actual_batch_size != batch_size:
            print(f"调整批次大小从 {batch_size} 到 {actual_batch_size}")

        # 创建数据加载器
        train_dataset = TensorDataset(train_data, train_coarse_labels_1, train_fine_labels_1,
                                      train_coarse_labels_2, train_fine_labels_2, train_original_labels)
        test_dataset = TensorDataset(test_data, test_coarse_labels_1, test_fine_labels_1,
                                     test_coarse_labels_2, test_fine_labels_2, test_original_labels)

        train_loader = DataLoader(train_dataset, batch_size=actual_batch_size, shuffle=True)
        test_loader = DataLoader(test_dataset, batch_size=actual_batch_size, shuffle=False)

        print(f"数据加载器创建成功 - 训练批次: {len(train_loader)}, 测试批次: {len(test_loader)}")

        return train_loader, test_loader

    except Exception as e:
        print(f"get_data_loaders 错误: {e}")
        import traceback
        traceback.print_exc()
        raise

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
    return train_loss

def evaluate_model(model, test_loader, device):
    """
    评估模型并返回预测标签
    """
    model.eval()
    coarse_correct_1 = fine_correct_1 = coarse_correct_2 = fine_correct_2 = medium_high_total_1 = low_medium_total_2 = total = 0
    all_coarse_preds_1, all_coarse_labels_1 = [], []
    all_fine_preds_1, all_fine_labels_1 = [], []
    all_coarse_preds_2, all_coarse_labels_2 = [], []
    all_fine_preds_2, all_fine_labels_2 = [], []
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

            # 收集预测结果
            all_coarse_preds_1.extend(coarse_predicted_1.cpu().numpy())
            all_coarse_labels_1.extend(coarse_labels_1.cpu().numpy())
            all_fine_preds_1.extend(fine_predicted_1.cpu().numpy())
            all_fine_labels_1.extend(fine_labels_1.cpu().numpy())
            all_coarse_preds_2.extend(coarse_predicted_2.cpu().numpy())
            all_coarse_labels_2.extend(coarse_labels_2.cpu().numpy())
            all_fine_preds_2.extend(fine_predicted_2.cpu().numpy())
            all_fine_labels_2.extend(fine_labels_2.cpu().numpy())

            original_preds_1 = convert_to_original_labels_1(coarse_predicted_1.cpu().numpy(), fine_predicted_1.cpu().numpy())
            original_preds_2 = convert_to_original_labels_2(coarse_predicted_2.cpu().numpy(), fine_predicted_2.cpu().numpy())

            # 使用元模型进行最终预测
            X_meta = np.column_stack((original_preds_1, original_preds_2))
            meta_model = LogisticRegression()
            meta_model.fit(X_meta, original_labels.numpy())
            original_preds = meta_model.predict(X_meta)
            all_original_preds.extend(original_preds)
            all_original_labels.extend(original_labels.numpy())

    # 计算准确率
    coarse_accuracy_1 = coarse_correct_1 / total if total > 0 else 0
    fine_accuracy_1 = fine_correct_1 / medium_high_total_1 if medium_high_total_1 > 0 else 0
    coarse_accuracy_2 = coarse_correct_2 / total if total > 0 else 0
    fine_accuracy_2 = fine_correct_2 / low_medium_total_2 if low_medium_total_2 > 0 else 0
    test_accuracy = accuracy_score(all_original_labels, all_original_preds)

    # 返回预测结果和准确率
    predictions = {
        'coarse_preds_1': np.array(all_coarse_preds_1),
        'coarse_labels_1': np.array(all_coarse_labels_1),
        'fine_preds_1': np.array(all_fine_preds_1),
        'fine_labels_1': np.array(all_fine_labels_1),
        'coarse_preds_2': np.array(all_coarse_preds_2),
        'coarse_labels_2': np.array(all_coarse_labels_2),
        'fine_preds_2': np.array(all_fine_preds_2),
        'fine_labels_2': np.array(all_fine_labels_2),
        'original_preds': np.array(all_original_preds),
        'original_labels': np.array(all_original_labels)
    }
    
    accuracies = {
        'coarse_accuracy_1': coarse_accuracy_1,
        'fine_accuracy_1': fine_accuracy_1,
        'coarse_accuracy_2': coarse_accuracy_2,
        'fine_accuracy_2': fine_accuracy_2,
        'test_accuracy': test_accuracy
    }

    return predictions, accuracies

#加载权重
def load_model_weights_predict(model_path,data):
    model = HierarchicalCrossSubModel(n_channels=2, n_times=250, embed_dim=EMBED_DIM).to(DEVICE)
    model.load_state_dict(torch.load(model_path))
    model.eval()
    Tdata = torch.Tensor(data[np.newaxis, :]).to(DEVICE)
    coarse_output_1, fine_output_1, coarse_output_2, fine_output_2, _, _ = model(Tdata)
    coarse_predicted_1 = torch.max(coarse_output_1, 1)[1]
    fine_predicted_1 = torch.max(fine_output_1, 1)[1]
    coarse_predicted_2 = torch.max(coarse_output_2, 1)[1]
    fine_predicted_2 = torch.max(fine_output_2, 1)[1]

    original_preds_1 = convert_to_original_labels_1(coarse_predicted_1.cpu().numpy(), fine_predicted_1.cpu().numpy())
    original_preds_2 = convert_to_original_labels_2(coarse_predicted_2.cpu().numpy(), fine_predicted_2.cpu().numpy())
    # 加权平均
    original_preds = np.round(0.3 * original_preds_1 + 0.7 * original_preds_2).astype(int)
    return original_preds
