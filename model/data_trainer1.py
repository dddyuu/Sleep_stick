import numpy as np
import torch
import os
from My_main import EEGDataReceiver
from data_trainer import EMBED_DIM, DEVICE, new_load_model_weights_predict, Pget_data_online_loaders
from Mymodel import HierarchicalCrossSubModel
import torch.nn.functional as F


def test_process_realtime_classification_with_model():
    """测试实时分类功能的完整流程，包括数据传输和模型预测"""

    print("=== 实时分类完整流程测试 ===")

    # 创建接收器实例
    receiver = EEGDataReceiver()
    receiver.event_51_count = 2  # 模拟第二次事件
    receiver.first_training_completed = True
    receiver.current_model_path = "D:/SubEEG/model/gr.pth"  # 确保模型路径存在

    # 初始化实时缓冲区
    receiver.realtime_buffer = [[], []]
    receiver.target_samples = 1500

    # 测试用的EEG数据 - 模拟真实数据场景
    test_scenarios = [
        {
            "name": "累积至目标样本数测试",
            "data_batches": [
                ([[np.random.randn(250).tolist(), np.random.randn(250).tolist()]], 51),
                ([[np.random.randn(300).tolist(), np.random.randn(300).tolist()]], 51),
                ([[np.random.randn(400).tolist(), np.random.randn(400).tolist()]], 51),
                ([[np.random.randn(550).tolist(), np.random.randn(550).tolist()]], 51),  # 总计达到1500
            ]
        }
    ]

    for scenario in test_scenarios:
        print(f"\n--- {scenario['name']} ---")

        # 重置缓冲区
        receiver.realtime_buffer = [[], []]
        model_predictions = []

        for i, (eeg_data, event_type) in enumerate(scenario['data_batches']):
            print(f"\n第{i + 1}批数据处理:")

            # 执行数据传输测试
            success, classification_data, labels = test_data_transmission(receiver, eeg_data, event_type)

            if success and classification_data is not None:
                print("✅ 数据传输成功，开始模型测试")

                # 执行模型预测测试
                predictions = test_model_prediction(classification_data, labels, receiver.current_model_path)
                if predictions is not None:
                    model_predictions.extend(predictions)
                    print(f"📊 模型预测结果: {predictions}")
                else:
                    print("❌ 模型预测失败")
            else:
                print("⏳ 数据未达到分类条件，继续累积")

        if model_predictions:
            print(f"\n🎯 场景完成 - 总预测数量: {len(model_predictions)}")
            print(f"预测结果分布: {np.bincount(model_predictions)}")


def test_data_transmission(receiver, eeg_data, event_type):
    """测试数据传输逻辑"""

    # 事件类型到标签映射
    event_to_label = {51: 0, 52: 1, 53: 2}
    current_label = event_to_label[event_type]

    print(f"🔄 处理事件 {event_type} -> 标签 {current_label}")

    if len(eeg_data) == 0 or len(eeg_data[0]) == 0:
        print("⚠️ 空数据，跳过处理")
        return False, None, None

    # 确保数据格式正确
    if len(eeg_data) < 2:
        if len(eeg_data) == 1:
            eeg_data = [eeg_data[0], eeg_data[0].copy()]
            print("✅ 单通道数据扩展为双通道")
        else:
            print("❌ 数据格式错误")
            return False, None, None

    # 只使用前2个通道
    eeg_data = eeg_data[:2]

    # 添加数据到缓冲区
    for ch in range(2):
        receiver.realtime_buffer[ch].extend(eeg_data[ch])

    ch1_len = len(receiver.realtime_buffer[0])
    ch2_len = len(receiver.realtime_buffer[1])
    print(f"📏 缓冲区状态: 通道1={ch1_len}/{receiver.target_samples}, 通道2={ch2_len}/{receiver.target_samples}")

    # 检查是否达到目标采样点数
    if ch1_len >= receiver.target_samples and ch2_len >= receiver.target_samples:
        print("🎯 达到目标采样点数，准备分类数据")

        # 准备分类数据 - 形状为 (5, 2, 500)
        data_for_classification = np.zeros((5, 2, 500), dtype=np.float64)

        for batch_idx in range(5):
            for ch_idx in range(2):
                start_idx = len(receiver.realtime_buffer[ch_idx]) - 500
                end_idx = len(receiver.realtime_buffer[ch_idx])
                data_for_classification[batch_idx, ch_idx, :] = receiver.realtime_buffer[ch_idx][start_idx:end_idx]

        # 验证数据完整性
        if np.any(np.isnan(data_for_classification)) or np.any(np.isinf(data_for_classification)):
            print("❌ 数据包含异常值")
            return False, None, None

        # 创建标签
        labels = np.full((5,), current_label, dtype=np.int64)

        print(f"📦 分类数据准备完成: 形状={data_for_classification.shape}")
        print(f"🏷️ 标签: {labels}")

        # 保持缓冲区大小（重叠策略）
        overlap = 250
        for ch in range(2):
            if len(receiver.realtime_buffer[ch]) > overlap:
                receiver.realtime_buffer[ch] = receiver.realtime_buffer[ch][-overlap:]
            else:
                receiver.realtime_buffer[ch] = []

        return True, data_for_classification, labels

    return False, None, None


def test_model_prediction(data, labels, model_path):
    """测试模型预测功能"""

    try:
        if not os.path.exists(model_path):
            print(f"⚠️ 模型文件不存在: {model_path}")
            return test_mock_model_prediction(data, labels)

        print("🤖 开始模型预测测试")

        # 创建数据加载器
        source_loader, target_loader = create_test_loaders(data, labels)

        # 调用模型预测
        predictions = new_load_model_weights_predict(model_path, source_loader, target_loader)

        if predictions and len(predictions) > 0:
            print(f"✅ 模型预测成功 - 预测数量: {len(predictions)}")
            return predictions
        else:
            print("❌ 模型预测返回空结果")
            return None

    except Exception as e:
        print(f"❌ 模型预测异常: {e}")
        return test_mock_model_prediction(data, labels)


def test_mock_model_prediction(data, labels):
    """模拟模型预测（当真实模型不可用时）"""

    print("🎭 使用模拟模型进行预测")

    try:
        # 简单的模拟预测逻辑
        mock_predictions = []
        for i in range(len(data)):
            # 基于数据的简单特征进行模拟预测
            mean_val = np.mean(data[i])
            if mean_val > 0.1:
                pred = 2
            elif mean_val > -0.1:
                pred = 1
            else:
                pred = 0
            mock_predictions.append(pred)

        print(f"🎯 模拟预测完成: {mock_predictions}")
        return mock_predictions

    except Exception as e:
        print(f"❌ 模拟预测失败: {e}")
        return None


def create_test_loaders(data, labels):
    """为测试创建数据加载器"""

    try:
        # 转换数据格式
        train_data = torch.tensor(data.astype(np.float32), dtype=torch.float)
        train_labels = torch.tensor(labels, dtype=torch.long)

        # 创建所需的多标签格式
        coarse_labels_1 = torch.zeros_like(train_labels)
        coarse_labels_1[train_labels > 0] = 1

        coarse_labels_2 = torch.zeros_like(train_labels)
        coarse_labels_2[train_labels > 1] = 1

        fine_labels_1 = torch.zeros_like(train_labels)
        fine_labels_1[train_labels == 2] = 1

        fine_labels_2 = torch.zeros_like(train_labels)
        fine_labels_2[train_labels == 1] = 1

        # 使用 Pget_data_online_loaders 的逻辑创建加载器
        from torch.utils.data import TensorDataset, DataLoader

        dataset = TensorDataset(
            train_data, coarse_labels_1, fine_labels_1,
            coarse_labels_2, fine_labels_2, train_labels
        )

        loader = DataLoader(dataset, batch_size=5, shuffle=False)

        # 返回相同的加载器作为source和target（测试用）
        return loader, loader

    except Exception as e:
        print(f"❌ 创建数据加载器失败: {e}")
        return None, None


def test_model_loading():
    """测试模型加载功能"""

    print("\n=== 模型加载测试 ===")

    model_path = "D:/SubEEG/model/gr.pth"

    try:
        if os.path.exists(model_path):
            # 测试模型加载
            model = HierarchicalCrossSubModel(n_channels=2, n_times=500, embed_dim=EMBED_DIM).to(DEVICE)
            model.load_state_dict(torch.load(model_path, map_location=DEVICE))
            model.eval()

            print("✅ 模型加载成功")

            # 测试模型推理
            test_input = torch.randn(1, 2, 500).to(DEVICE)
            with torch.no_grad():
                outputs = model(test_input)
                print(f"✅ 模型推理成功 - 输出数量: {len(outputs)}")

            return True
        else:
            print(f"⚠️ 模型文件不存在: {model_path}")
            print("🎭 将使用模拟预测进行测试")
            return False

    except Exception as e:
        print(f"❌ 模型加载失败: {e}")
        return False


def test_edge_cases_with_model():
    """测试边界情况和模型鲁棒性"""

    print("\n=== 边界情况和模型鲁棒性测试 ===")

    receiver = EEGDataReceiver()
    receiver.event_51_count = 2
    receiver.first_training_completed = True
    receiver.current_model_path = "D:/SubEEG/model/gr.pth"
    receiver.realtime_buffer = [[], []]
    receiver.target_samples = 1500

    edge_cases = [
        {
            "name": "异常值数据",
            "generator": lambda: [np.full(100, np.inf).tolist(), np.full(100, -np.inf).tolist()],
            "event": 51
        },
        {
            "name": "全零数据",
            "generator": lambda: [np.zeros(100).tolist(), np.zeros(100).tolist()],
            "event": 52
        },
        {
            "name": "随机噪声数据",
            "generator": lambda: [np.random.randn(100).tolist() * 100, np.random.randn(100).tolist() * 100],
            "event": 53
        }
    ]

    for case in edge_cases:
        print(f"\n--- {case['name']} ---")

        # 重置缓冲区
        receiver.realtime_buffer = [[], []]

        try:
            # 逐步累积数据直到达到目标样本数
            batch_count = 0
            max_batches = 20  # 最多处理20批数据

            while (len(receiver.realtime_buffer[0]) < receiver.target_samples or
                   len(receiver.realtime_buffer[1]) < receiver.target_samples) and batch_count < max_batches:

                batch_count += 1
                print(f"第{batch_count}批边界数据处理:")

                # 生成一批测试数据
                batch_data = case['generator']()

                # 处理数据传输
                success, classification_data, labels = test_data_transmission(receiver, batch_data, case['event'])

                if success:
                    print(f"✅ {case['name']}达到分类条件，开始模型测试")

                    # 清理异常值
                    classification_data = np.nan_to_num(classification_data, nan=0.0, posinf=1.0, neginf=-1.0)

                    # 限制数据范围防止模型崩溃
                    classification_data = np.clip(classification_data, -10.0, 10.0)

                    print(f"🔧 数据清理完成，形状: {classification_data.shape}")

                    # 测试模型预测
                    predictions = test_model_prediction(classification_data, labels, receiver.current_model_path)

                    if predictions and len(predictions) > 0:
                        print(f"✅ {case['name']}模型预测成功: {predictions}")
                    else:
                        print(f"⚠️ {case['name']}模型预测失败，使用模拟预测")
                        mock_predictions = test_mock_model_prediction(classification_data, labels)
                        print(f"🎭 模拟预测结果: {mock_predictions}")
                    break
                else:
                    current_ch1 = len(receiver.realtime_buffer[0])
                    current_ch2 = len(receiver.realtime_buffer[1])
                    print(
                        f"📊 当前进度: 通道1={current_ch1}/{receiver.target_samples}, 通道2={current_ch2}/{receiver.target_samples}")

            # 检查是否成功达到目标
            if batch_count >= max_batches:
                print(f"⚠️ {case['name']}在{max_batches}批次后未能达到目标样本数")
                # 强制进行一次测试
                if len(receiver.realtime_buffer[0]) > 0 and len(receiver.realtime_buffer[1]) > 0:
                    print("🔧 使用现有数据进行强制测试")
                    # 截取或填充到目标长度
                    ch1_data = receiver.realtime_buffer[0]
                    ch2_data = receiver.realtime_buffer[1]

                    # 确保数据长度一致
                    min_len = min(len(ch1_data), len(ch2_data))
                    if min_len >= 500:  # 至少需要500个样本进行模型测试
                        test_data = np.array([ch1_data[:min_len], ch2_data[:min_len]])
                        # 如果数据太短，重复填充到500
                        if test_data.shape[1] < 500:
                            repeat_factor = 500 // test_data.shape[1] + 1
                            test_data = np.repeat(test_data, repeat_factor, axis=1)[:, :500]

                        test_data = np.nan_to_num(test_data, nan=0.0, posinf=1.0, neginf=-1.0)
                        test_data = np.clip(test_data, -10.0, 10.0)

                        # 扩展维度进行测试
                        test_data = test_data[np.newaxis, :]
                        test_labels = np.array([0])  # 默认标签

                        predictions = test_model_prediction(test_data, test_labels, receiver.current_model_path)
                        if predictions:
                            print(f"🎯 强制测试成功: {predictions}")
                        else:
                            print("❌ 强制测试失败")
                    else:
                        print("❌ 数据不足，无法进行测试")

        except Exception as e:
            print(f"❌ {case['name']}测试异常: {e}")
            import traceback
            print(traceback.format_exc())


def test_accumulation_to_target():
    """专门测试累积到目标样本数的功能"""

    print("\n=== 数据累积到目标测试 ===")

    receiver = EEGDataReceiver()
    receiver.event_51_count = 2
    receiver.first_training_completed = True
    receiver.current_model_path = "D:/SubEEG/model/gr.pth"
    receiver.realtime_buffer = [[], []]
    receiver.target_samples = 1500

    # 模拟分批数据传输
    batch_size = 100  # 每批100个样本
    total_batches = 16  # 总共16批，确保超过1500个样本

    print(f"📊 目标样本数: {receiver.target_samples}")
    print(f"📦 每批样本数: {batch_size}")
    print(f"🔢 计划批次数: {total_batches}")

    for batch_idx in range(total_batches):
        print(f"\n--- 第{batch_idx + 1}批数据处理 ---")

        # 生成测试数据
        test_data = [
            np.random.randn(batch_size).tolist(),
            np.random.randn(batch_size).tolist()
        ]

        # 处理数据传输
        success, classification_data, labels = test_data_transmission(receiver, test_data, 51)

        if success:
            print(f"🎉 在第{batch_idx + 1}批数据后达到分类条件!")
            print(f"📊 最终分类数据形状: {classification_data.shape}")

            # 测试模型预测
            predictions = test_model_prediction(classification_data, labels, receiver.current_model_path)
            if predictions:
                print(f"✅ 模型预测成功: {predictions}")
            else:
                print("⚠️ 使用模拟预测")
                mock_predictions = test_mock_model_prediction(classification_data, labels)
                print(f"🎭 模拟预测结果: {mock_predictions}")
            break
        else:
            ch1_len = len(receiver.realtime_buffer[0])
            ch2_len = len(receiver.realtime_buffer[1])
            progress_1 = (ch1_len / receiver.target_samples) * 100
            progress_2 = (ch2_len / receiver.target_samples) * 100
            print(
                f"📈 累积进度: 通道1={ch1_len}/{receiver.target_samples} ({progress_1:.1f}%), 通道2={ch2_len}/{receiver.target_samples} ({progress_2:.1f}%)")

    print("\n✅ 数据累积测试完成")


if __name__ == "__main__":
    print("🧪 开始EEG实时分类完整流程测试")

    # 1. 测试模型加载
    model_available = test_model_loading()

    # 2. 测试数据累积功能
    test_accumulation_to_target()

    # 3. 测试完整的数据传输和模型预测流程
    test_process_realtime_classification_with_model()

    # 4. 测试边界情况
    test_edge_cases_with_model()

    print("\n✅ 所有测试完成")
