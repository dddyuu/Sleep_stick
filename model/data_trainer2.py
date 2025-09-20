from loss_function import *
from sklearn.metrics import accuracy_score
from Mymodel import *
from torch.utils.data import TensorDataset, DataLoader
import torch.optim as optim
import torch.nn as nn  # 添加nn模块导入
from sklearn import preprocessing

from preprocess import save_to_train_npy

# 模型超参数
EMBED_DIM = 64
DROPOUT = 0.5
BATCH_SIZE = 128
NUM_EPOCHS = 1000
LEARNING_RATE = 0.005
WEIGHT_DECAY = 1e-2

# 随机种子
SEED = 3407

# 设备
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

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

def Pget_data_loaders(Train_data_path, Train_label_path,Test_data_path,Test_label_path, batch_size=128):
    try:
        #加载训练数据
        train_data = np.load(Train_data_path)
        train_original_labels = np.load(Train_label_path)

        print(f"原始数据形状: {train_data.shape}")
        print(f"原始标签形状: {train_original_labels.shape}")
        print(f"标签分布: {np.bincount(train_original_labels)}")

        # 验证数据完整性
        if len(train_data) == 0 or len(train_original_labels) == 0:
            raise ValueError("数据或标签为空")

        if len(train_data) != len(train_original_labels):
            raise ValueError(f"数据和标签长度不匹配: {len(train_data)} vs {len(train_original_labels)}")

        # 检查标签的唯一值
        unique_labels = np.unique(train_original_labels)
        print(f"唯一标签: {unique_labels}")

        if len(unique_labels) < 2:
            raise ValueError(f"标签类别不足，只有 {len(unique_labels)} 类")


        # 按类别分组数据
        class_0_indices = np.where(train_original_labels == 0)[0]
        class_1_indices = np.where(train_original_labels == 1)[0]
        class_2_indices = np.where(train_original_labels == 2)[0]

        print(f"类别0样本数: {len(class_0_indices)}")
        print(f"类别1样本数: {len(class_1_indices)}")
        print(f"类别2样本数: {len(class_2_indices)}")
        # 加载验证数据
        Test_data = np.load(Test_data_path)

        Test_original_labels = np.load(Test_label_path)
        print(f"验证原始数据形状: {Test_data.shape}")
        print(f"验证原始标签形状: {Test_original_labels.shape}")
        print(f"验证标签分布: {np.bincount(Test_original_labels)}")

        # 验证数据完整性
        if len(Test_data) == 0 or len(Test_original_labels) == 0:
            raise ValueError("验证数据或标签为空")

        if len(Test_data) != len(Test_original_labels):
            Test_original_labels = Test_original_labels[:len(Test_data)]
            # raise ValueError(f"验证数据和标签长度不匹配: {len(Test_data)} vs {len(Test_original_labels)}")

        # 检查标签的唯一值
        unique_labels = np.unique(Test_original_labels)
        print(f"验证唯一标签: {unique_labels}")

        if len(unique_labels) < 2:
            raise ValueError(f"验证标签类别不足，只有 {len(unique_labels)} 类")

        #验证类别分组数据
        test_class_0_indices = np.where(Test_original_labels == 0)[0]
        test_class_1_indices = np.where(Test_original_labels == 1)[0]
        test_class_2_indices = np.where(Test_original_labels == 2)[0]

        print(f"类别0样本数: {len(class_0_indices)}")
        print(f"类别1样本数: {len(class_1_indices)}")
        print(f"类别2样本数: {len(class_2_indices)}")

        print(f"验证类别0样本数: {len(test_class_0_indices)}")
        print(f"验证类别1样本数: {len(test_class_1_indices)}")
        print(f"验证类别2样本数: {len(test_class_2_indices)}")

        # 检查每个类别是否有足够的样本进行分割
        min_samples_per_class = 10  # 每类至少需要10个样本

        if len(class_0_indices) < min_samples_per_class:
            print(f"警告: 类别0样本数不足 ({len(class_0_indices)})，使用全部数据")
        if len(class_1_indices) < min_samples_per_class:
            print(f"警告: 类别1样本数不足 ({len(class_1_indices)})，使用全部数据")
        if len(class_2_indices) < min_samples_per_class:
            print(f"警告: 类别2样本数不足 ({len(class_2_indices)})，使用全部数据")

        # 合并训练和测试索引
        train_indices = np.concatenate([class_0_indices, class_1_indices, class_2_indices])
        test_indices = np.concatenate([test_class_0_indices, test_class_1_indices, test_class_2_indices])

        print(f"训练集索引数量: {len(train_indices)}")
        print(f"测试集索引数量: {len(test_indices)}")

        # 确保训练集不为空
        if len(train_indices) == 0:
            raise ValueError("训练集为空，无法进行训练")

        # 提取训练和测试数据
        train_data = train_data[train_indices]
        train_original_labels = train_original_labels[train_indices]
        test_data = Test_data[test_indices]
        test_original_labels = Test_original_labels[test_indices]

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
    criterion_domain = nn.BCEWithLogitsLoss()  # 添加域损失函数定义
    total_loss = coarse_loss_total = fine_loss_total = domain_loss_total = alignment_loss_total = 0
    total_train_acc = 0
    target_iter = iter(target_loader)

    for batch_idx, (source_data, source_coarse_labels_1, source_fine_labels_1, source_coarse_labels_2, source_fine_labels_2, source_original_labels) in enumerate(source_loader):
        try:
            target_data, _, _, _, _,_  = next(target_iter)
        except StopIteration:
            target_iter = iter(target_loader)
            target_data, _, _, _, _,_= next(target_iter)

        source_data, source_coarse_labels_1, source_fine_labels_1, source_coarse_labels_2, source_fine_labels_2 = source_data.to(device), source_coarse_labels_1.to(
            device), source_fine_labels_1.to(device), source_coarse_labels_2.to(device), source_fine_labels_2.to(device)
        target_data = target_data.to(device)
        source_domain = torch.zeros(source_data.size(0), 1).to(device)  # 源域标签， 全为0
        target_domain = torch.ones(target_data.size(0), 1).to(device)  # 目标域标签，全为1

        optimizer.zero_grad()
        p = float(batch_idx + epoch * len(source_loader)) / (total_epochs * len(source_loader))  # 计算当前训练进度，范围从0到1
        alpha = 2.0 / (1.0 + np.exp(-10 * p)) - 1.0  # 2sigmoid(10p)-1， 自适应参数，渐进式策略

        source_coarse_output_1, source_fine_output_1, source_coarse_output_2, source_fine_output_2, _, source_domain_output, source_features = model(source_data, alpha)
        target_coarse_output_1, target_fine_output_1, target_coarse_output_2, target_fine_output_2, _, target_domain_output, target_features = model(target_data, alpha)

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


        domain_loss = criterion_domain(source_domain_output.squeeze(), source_domain.squeeze().float()) + criterion_domain(target_domain_output.squeeze(),
                                                                                               target_domain.squeeze().float())
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

#加载权重
def load_model_weights_predict(model_path,data):
    model = HierarchicalCrossSubModel(n_channels=2, n_times=500, embed_dim=EMBED_DIM).to(DEVICE)
    model.load_state_dict(torch.load(model_path))
    model.eval()
    Tdata = torch.Tensor(data[np.newaxis, :]).to(DEVICE)
    coarse_output_1, fine_output_1, coarse_output_2, fine_output_2, _, _ = model(Tdata)
    coarse_predicted_1 = torch.max(coarse_output_1, 1)[1]
    fine_predicted_1 = torch.max(fine_output_1, 1)[1]
    coarse_predicted_2 = torch.max(coarse_output_2, 1)[1]
    fine_predicted_2 = torch.max(fine_output_2, 1)[1]
    original_preds_1 = convert_to_original_labels_1(coarse_predicted_1.cpu().numpy(),
                                                    fine_predicted_1.cpu().numpy())
    original_preds_2 = convert_to_original_labels_2(coarse_predicted_2.cpu().numpy(),
                                                    fine_predicted_2.cpu().numpy())
    # 简单多数投票
    original_preds = np.round((original_preds_1 + original_preds_1) / 2).astype(int)
    return original_preds


def online_train_epoch(model, source_loader, target_loader, optimizer, device, epoch, total_epochs):
    model.train()
    criterion_cls = LabelSmoothingLoss(classes=2, smoothing=0.1)  # 类别数2，平滑参数为0.1
    criterion_alignment = ClassAlignmentLoss()
    criterion_domain = nn.BCEWithLogitsLoss()  # 添加域损失函数定义
    total_loss = coarse_loss_total = fine_loss_total = domain_loss_total = alignment_loss_total = 0
    target_data,_,_,_,_,_ = next(iter(target_loader))

    for batch_idx, (
    source_data, source_coarse_labels_1, source_fine_labels_1, source_coarse_labels_2, source_fine_labels_2,
    source_original_labels) in enumerate(source_loader):

        source_data, source_coarse_labels_1, source_fine_labels_1, source_coarse_labels_2, source_fine_labels_2 = source_data.to(
            device), source_coarse_labels_1.to(
            device), source_fine_labels_1.to(device), source_coarse_labels_2.to(device), source_fine_labels_2.to(device)
        target_data = target_data.to(device)
        source_domain = torch.zeros(source_data.size(0), 1).to(device)  # 源域标签， 全为0
        target_domain = torch.ones(target_data.size(0), 1).to(device)  # 目标域标签，全为1

        optimizer.zero_grad()
        p = float(batch_idx + epoch * len(source_loader)) / (total_epochs * len(source_loader))  # 计算当前训练进度，范围从0到1
        alpha = 2.0 / (1.0 + np.exp(-10 * p)) - 1.0  # 2sigmoid(10p)-1， 自适应参数，渐进式策略

        source_coarse_output_1, source_fine_output_1, source_coarse_output_2, source_fine_output_2, source_combined_feat, source_domain_output, source_features = model(
            source_data, alpha)
        target_coarse_output_1, target_fine_output_1, target_coarse_output_2, target_fine_output_2, target_combined_feat, target_domain_output, target_features = model(
            target_data, alpha)
        # 获取概率而不是硬标签
        target_coarse_probs_1 = F.softmax(target_coarse_output_1, dim=1)
        target_fine_probs_1 = F.softmax(target_fine_output_1, dim=1)
        target_coarse_probs_2 = F.softmax(target_coarse_output_2, dim=1)
        target_fine_probs_2 = F.softmax(target_fine_output_2, dim=1)

        target_coarse_pseudo_labels_1 = torch.argmax(target_coarse_probs_1, dim=1)  # 为目标域数据生成粗粒度伪标签，取最大概率的类别
        target_fine_pseudo_labels_1 = torch.argmax(target_fine_probs_1, dim=1)  # 为目标域数据生成细粒度伪标签，取最大概率的类别
        target_coarse_pseudo_labels_2 = torch.argmax(target_coarse_probs_2, dim=1)  # 为目标域数据生成粗粒度伪标签，取最大概率的类别
        target_fine_pseudo_labels_2 = torch.argmax(target_fine_probs_2, dim=1)  # 为目标域数据生成细粒度伪标签，取最大概率的类别
        coarse_loss_1 = criterion_cls(source_coarse_output_1, source_coarse_labels_1)  # 计算源域数据的粗粒度分类损失
        coarse_loss_2 = criterion_cls(source_coarse_output_2, source_coarse_labels_2)  # 计算源域数据的粗粒度分类损失
        medium_high_mask_1 = source_coarse_labels_1 == 1  # 生成布尔掩码，标识源域中粗粒度标签为1的样本(中高负荷样本)
        fine_loss_1 = criterion_cls(source_fine_output_1[medium_high_mask_1],
                                    source_fine_labels_1[
                                        medium_high_mask_1]) if medium_high_mask_1.sum() > 0 else torch.tensor(
            0.0).to(device)  # 如果有中高负荷样本，计算这些样本的细粒度分类损失；否则设为0
        medium_high_mask_2 = source_coarse_labels_2 == 0  # 生成布尔掩码，标识源域中粗粒度标签为0的样本(低中负荷样本)
        fine_loss_2 = criterion_cls(source_fine_output_2[medium_high_mask_2],
                                    source_fine_labels_2[
                                        medium_high_mask_2]) if medium_high_mask_2.sum() > 0 else torch.tensor(
            0.0).to(device)  # 如果有低中负荷样本，计算这些样本的细粒度分类损失；否则设为0
        domain_loss = criterion_domain(source_domain_output.squeeze(), source_domain.squeeze().float()) + criterion_domain(target_domain_output.squeeze(), target_domain.squeeze().float())
        alignment_loss_1 = criterion_alignment(source_features, target_features,
                                               [source_coarse_labels_1, source_fine_labels_1],
                                               [target_coarse_pseudo_labels_1,
                                                target_fine_pseudo_labels_1])  # 计算源域和目标域特征的对齐损失，使用真实标签和伪标签
        alignment_loss_2 = criterion_alignment(source_features, target_features,
                                               [source_coarse_labels_2, source_fine_labels_2],
                                               [target_coarse_pseudo_labels_2,
                                                target_fine_pseudo_labels_2])  # 计算源域和目标域特征的对齐损失，使用真实标签和伪标签
        lambda_coarse, lambda_fine, lambda_domain, lambda_alignment = 1.0, 1.0, 0.5 * (1 + np.exp(-10 * p)), 0.3
        loss = lambda_coarse * coarse_loss_1 + lambda_fine * fine_loss_1 + lambda_coarse * coarse_loss_2 + lambda_fine * fine_loss_2 + lambda_alignment * alignment_loss_1 + lambda_alignment * alignment_loss_2
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
    all_original_preds, all_original_labels = [], []
    total_val_loss = 0



    with torch.no_grad():
        data, coarse_labels_1, fine_labels_1, coarse_labels_2, fine_labels_2, original_labels = next(iter(target_loader))
        data, coarse_labels_1, fine_labels_1, coarse_labels_2, fine_labels_2 = data.to(device), coarse_labels_1.to(device), fine_labels_1.to(device), coarse_labels_2.to(device), fine_labels_2.to(device)

        coarse_output_1, fine_output_1, coarse_output_2, fine_output_2, _, _, features = model(data)
        #原来的测试方式
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

        original_preds_1 = convert_to_original_labels_1(coarse_predicted_1.cpu().numpy(),
                                                        fine_predicted_1.cpu().numpy())
        original_preds_2 = convert_to_original_labels_2(coarse_predicted_2.cpu().numpy(),
                                                        fine_predicted_2.cpu().numpy())
        # 简单多数投票
        original_preds = np.round((original_preds_1 + original_preds_2) / 2).astype(int)
        all_original_preds.extend(original_preds)
        all_original_labels.extend(original_labels.numpy())

    coarse_accuracy_1 = coarse_correct_1 / total if total > 0 else 0
    fine_accuracy_1 = fine_correct_1 / medium_high_total_1 if medium_high_total_1 > 0 else 0
    coarse_accuracy_2 = coarse_correct_2 / total if total > 0 else 0
    fine_accuracy_2 = fine_correct_2 / low_medium_total_2 if low_medium_total_2 > 0 else 0
    test_accuracy = accuracy_score(all_original_labels, all_original_preds)

    return train_loss, total_val_loss, coarse_accuracy_1, fine_accuracy_1, coarse_accuracy_2, fine_accuracy_2, test_accuracy, all_original_preds


def test_predict(model,target_loader):
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE, weight_decay=WEIGHT_DECAY)
    model.eval()
    all_original_preds, all_original_labels = [], []
    total_val_loss = 0


    for batch_data in target_loader:  # 正确遍历 DataLoader
        data,_, _, _, _,original_labels = batch_data
        data = data.to(DEVICE)
        coarse_output_1, fine_output_1, coarse_output_2, fine_output_2, _, _, features = model(data)
        optimizer.zero_grad()

        # 获取概率
        coarse_probs_1 = F.softmax(coarse_output_1, dim=1)
        fine_probs_1 = F.softmax(fine_output_1, dim=1)
        coarse_probs_2 = F.softmax(coarse_output_2, dim=1)
        fine_probs_2 = F.softmax(fine_output_2, dim=1)

        # 计算各类别的置信度
        confidence_coarse_1, _ = torch.max(coarse_probs_1, dim=1)
        confidence_fine_1, _ = torch.max(fine_probs_1, dim=1)
        confidence_coarse_2, _ = torch.max(coarse_probs_2, dim=1)
        confidence_fine_2, _ = torch.max(fine_probs_2, dim=1)

        # 平均置信度作为整体置信度
        avg_confidence = (confidence_coarse_1 + confidence_fine_1 + confidence_coarse_2 + confidence_fine_2) / 4

        # 使用所有样本，但用置信度加权
        pseudo_coarse_loss_1 = F.cross_entropy(coarse_output_1, torch.argmax(coarse_probs_1, dim=1), reduction='none')
        pseudo_coarse_loss_2 = F.cross_entropy(coarse_output_2, torch.argmax(coarse_probs_2, dim=1), reduction='none')

        # 置信度加权的损失
        weighted_loss = (pseudo_coarse_loss_1 * avg_confidence).mean() + \
                        (pseudo_coarse_loss_2 * avg_confidence).mean()

        # 添加特征多样性损失（防止所有预测相同）
        pred_entropy = -torch.sum(coarse_probs_1 * torch.log(coarse_probs_1 + 1e-10), dim=1).mean()
        diversity_loss = -pred_entropy  # 鼓励预测多样性

        # 最终验证损失
        val_loss = weighted_loss #+ 0.1 * diversity_loss

        val_loss.backward()
        optimizer.step()

        total_val_loss += val_loss.item()

        # 使用概率融合
        final_probs = convert_hierarchical_to_original_prob(
            coarse_probs_1, fine_probs_1,
            coarse_probs_2, fine_probs_2,
            model.fusion_weight
        )

        original_preds = torch.argmax(final_probs, dim=1).cpu().numpy()

    all_original_preds.extend(original_preds)

    all_original_labels.extend(original_labels.numpy())
    test_accuracy = accuracy_score(all_original_labels, all_original_preds)

    return test_accuracy,all_original_preds

def pro_load_model_weights_predict(model_path,data,scaler):

    test_data = torch.tensor(data.real.astype(float), dtype=torch.float)


    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    n_channels, n_times = 2, 500
    model = HierarchicalCrossSubModel(n_channels, n_times, embed_dim=EMBED_DIM).to(device)
    model.load_state_dict(torch.load(model_path))

    # 优化器和调度器
    # optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE, weight_decay=WEIGHT_DECAY)
    optimizer = optim.SGD(model.parameters(), lr=LEARNING_RATE, momentum=0.5)
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='max', factor=0.5, patience=5, verbose=True)

    criterion_cls = LabelSmoothingLoss(classes=2, smoothing=0.1)  # 类别数2，平滑参数为0.1
    criterion_alignment = ClassAlignmentLoss()
    # 1. 将模型设置为训练模式（为了启用BN和Dropout，这对于计算正确梯度是必要的）
    model.train()
    # 2. 测试时自适应循环
    for epochs in range(100):  # 在小测试集上迭代少量次数
        inputs = test_data.to(device)  # 这里我们故意不要标签
        # 清零梯度
        optimizer.zero_grad()
        # 前向传播
        coarse_output_1, fine_output_1, coarse_output_2, fine_output_2, _, _, _= model(inputs)
        coarse_predicted_1 = torch.max(coarse_output_1, 1)[1]
        fine_predicted_1 = torch.max(fine_output_1, 1)[1]
        coarse_predicted_2 = torch.max(coarse_output_2, 1)[1]
        fine_predicted_2 = torch.max(fine_output_2, 1)[1]

        original_preds_1 = convert_to_original_labels_1(coarse_predicted_1.cpu().numpy(), fine_predicted_1.cpu().numpy())
        original_preds_2 = convert_to_original_labels_2(coarse_predicted_2.cpu().numpy(), fine_predicted_2.cpu().numpy())

        # 简单多数投票
        original_preds = np.round((original_preds_1 + original_preds_2) / 2).astype(int)
        original_preds = torch.tensor(original_preds.real.astype(float), dtype=torch.float)

        # 计算无监督损失：熵最小化
        coarse_output_1 = torch.softmax(coarse_output_1, dim=1)
        fine_output_1 = torch.softmax(fine_output_1, dim=1)
        coarse_output_2 = torch.softmax(coarse_output_2, dim=1)
        fine_output_2 = torch.softmax(fine_output_2, dim=1)
        loss_coarse1 = 1 / -torch.mean(torch.sum(coarse_output_1 * torch.log(coarse_output_1 + 1e-10), dim=1))  # 加上一个小数防止log(0)
        loss_fine1 = -torch.mean(torch.sum(fine_output_1 * torch.log(fine_output_1 + 1e-10), dim=1))  # 加上一个小数防止log(0)
        loss_coarse2 = -torch.mean(torch.sum(coarse_output_2 * torch.log(coarse_output_2 + 1e-10), dim=1))  # 加上一个小数防止log(0)
        loss_fine2 = -torch.mean(torch.sum(fine_output_2 * torch.log(fine_output_2 + 1e-10), dim=1))  # 加上一个小数防止log(0)
        loss = loss_coarse1 + loss_fine1 + loss_coarse2 + loss_fine2
        print(loss)
        # 反向传播
        loss.backward()
        # 更新参数
        optimizer.step()

    model.eval()
    data = test_data.to(device)
    coarse_output_1, fine_output_1, coarse_output_2, fine_output_2, _, _, _ = model(data)
    coarse_predicted_1 = torch.max(coarse_output_1, 1)[1]
    fine_predicted_1 = torch.max(fine_output_1, 1)[1]
    coarse_predicted_2 = torch.max(coarse_output_2, 1)[1]
    fine_predicted_2 = torch.max(fine_output_2, 1)[1]
    original_preds_1 = convert_to_original_labels_1(coarse_predicted_1.cpu().numpy(), fine_predicted_1.cpu().numpy())
    original_preds_2 = convert_to_original_labels_2(coarse_predicted_2.cpu().numpy(), fine_predicted_2.cpu().numpy())

    # 简单多数投票
    original_preds = np.round((original_preds_1 + original_preds_2) / 2).astype(int)
    return original_preds

def Pget_data_online_loaders(Train_data_path, Train_label_path,Test_data,Test_label, batch_size=5):
    try:
        #加载训练数据
        train_data = np.load(Train_data_path)
        train_original_labels = np.load(Train_label_path)

        print(f"原始数据形状: {train_data.shape}")
        print(f"原始标签形状: {train_original_labels.shape}")
        print(f"标签分布: {np.bincount(train_original_labels)}")

        # 验证数据完整性
        if len(train_data) == 0 or len(train_original_labels) == 0:
            raise ValueError("数据或标签为空")

        if len(train_data) != len(train_original_labels):
            raise ValueError(f"数据和标签长度不匹配: {len(train_data)} vs {len(train_original_labels)}")

        # 检查标签的唯一值
        unique_labels = np.unique(train_original_labels)
        print(f"唯一标签: {unique_labels}")



        # 按类别分组数据
        class_0_indices = np.where(train_original_labels == 0)[0]
        class_1_indices = np.where(train_original_labels == 1)[0]
        class_2_indices = np.where(train_original_labels == 2)[0]

        print(f"类别0样本数: {len(class_0_indices)}")
        print(f"类别1样本数: {len(class_1_indices)}")
        print(f"类别2样本数: {len(class_2_indices)}")
        # 加载验证数据

        Test_original_labels = Test_label
        print(f"验证原始数据形状: {Test_data.shape}")
        print(f"验证原始标签形状: {Test_original_labels.shape}")
        print(f"验证标签分布: {np.bincount(Test_original_labels)}")

        # 验证数据完整性
        if len(Test_data) == 0 or len(Test_original_labels) == 0:
            raise ValueError("验证数据或标签为空")

        if len(Test_data) != len(Test_original_labels):
            Test_original_labels = Test_original_labels[:len(Test_data)]
            # raise ValueError(f"验证数据和标签长度不匹配: {len(Test_data)} vs {len(Test_original_labels)}")

        # 检查标签的唯一值
        unique_labels = np.unique(Test_original_labels)
        print(f"验证唯一标签: {unique_labels}")

        #验证类别分组数据
        test_class_0_indices = np.where(Test_original_labels == 0)[0]
        test_class_1_indices = np.where(Test_original_labels == 1)[0]
        test_class_2_indices = np.where(Test_original_labels == 2)[0]

        print(f"类别0样本数: {len(class_0_indices)}")
        print(f"类别1样本数: {len(class_1_indices)}")
        print(f"类别2样本数: {len(class_2_indices)}")

        print(f"验证类别0样本数: {len(test_class_0_indices)}")
        print(f"验证类别1样本数: {len(test_class_1_indices)}")
        print(f"验证类别2样本数: {len(test_class_2_indices)}")

        # 检查每个类别是否有足够的样本进行分割
        min_samples_per_class = 10  # 每类至少需要10个样本

        if len(class_0_indices) < min_samples_per_class:
            print(f"警告: 类别0样本数不足 ({len(class_0_indices)})，使用全部数据")
        if len(class_1_indices) < min_samples_per_class:
            print(f"警告: 类别1样本数不足 ({len(class_1_indices)})，使用全部数据")
        if len(class_2_indices) < min_samples_per_class:
            print(f"警告: 类别2样本数不足 ({len(class_2_indices)})，使用全部数据")

        # 合并训练和测试索引
        train_indices = np.concatenate([class_0_indices, class_1_indices, class_2_indices])
        test_indices = np.concatenate([test_class_0_indices, test_class_1_indices, test_class_2_indices])

        print(f"训练集索引数量: {len(train_indices)}")
        print(f"测试集索引数量: {len(test_indices)}")

        # 确保训练集不为空
        if len(train_indices) == 0:
            raise ValueError("训练集为空，无法进行训练")

        # 提取训练和测试数据
        train_data = train_data[train_indices]
        train_original_labels = train_original_labels[train_indices]
        test_data = Test_data[test_indices]
        test_original_labels = Test_original_labels[test_indices]

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


def new_load_model_weights_predict(model_path,source_loader,target_loader):
    BATCH_SIZE = 5
    NUM_EPOCHS = 10
    LEARNING_RATE = 0.005
    WEIGHT_DECAY = 5e-2
    model = HierarchicalCrossSubModel(n_channels=2, n_times=500, embed_dim=EMBED_DIM).to(DEVICE)
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE, weight_decay=WEIGHT_DECAY)
    best_best_preds = []
    model.load_state_dict(torch.load(model_path))
    best_preds = []
    min_acc = -1
    for epoch in range(NUM_EPOCHS):
        train_loss, test_loss, coarse_accuracy_1, fine_accuracy_1, coarse_accuracy_2, fine_accuracy_2, test_accuracy,all_original_preds = online_train_epoch(
            model, source_loader, target_loader, optimizer, DEVICE, epoch, NUM_EPOCHS
        )
        if (test_accuracy) > min_acc:
            min_acc = test_accuracy
            best_preds = all_original_preds
    best_best_preds.extend(best_preds)
    return best_best_preds



if __name__ == "__main__":
    # preprocessed_data = preprocess_data(data)
    model_path = "gr_1.pth"
    Train_data_path = "data/dddxl.npy"
    train_npy_data_path = "data/dddxl.npy"
    train_npy_label = "label/dddxl.npy"
    npy_mat = "dddxl_0.mat"

    online_data = np.random.random((5, 2, 500))
    online_label = np.array([0, 0, 0, 0, 0])
    scaler = preprocessing.StandardScaler()
    scaler = save_to_train_npy(npy_mat,train_npy_data_path,train_npy_label,scaler)
    t = pro_load_model_weights_predict(model_path, data,scaler)

    print(t)

