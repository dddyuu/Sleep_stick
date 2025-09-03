import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np
import warnings

# 忽略警告
warnings.filterwarnings("ignore")
BATCH_SIZE = 128


class AnalyticSignalApprox(nn.Module):
    """使用可微分的近似方法替代FFT的希尔伯特变换"""

    def __init__(self, seq_len=500):
        super().__init__()
        # 使用1D卷积来近似希尔伯特变换
        self.hilbert_conv = nn.Conv1d(1, 1, kernel_size=25, padding=12, bias=False)

        # 初始化希尔伯特核（简化版本）
        with torch.no_grad():
            kernel = torch.zeros(25)
            center = 12
            for i in range(25):
                if i != center:
                    kernel[i] = 2.0 / (np.pi * (i - center)) * (1 - (-1) ** (i - center))
            self.hilbert_conv.weight[0, 0] = kernel

    def forward(self, x):
        """
        近似的希尔伯特变换实现（避免使用FFT）
        输入: (batch, seq_len)
        输出: (batch, seq_len) 复数张量的近似
        """
        # 原始信号作为实部
        real_part = x

        # 使用卷积近似希尔伯特变换作为虚部
        x_expanded = x.unsqueeze(1)  # (batch, 1, seq_len)
        imag_part = self.hilbert_conv(x_expanded).squeeze(1)  # (batch, seq_len)

        # 返回复数（用两个通道表示实部和虚部）
        return torch.complex(real_part, imag_part)


class PhaseLockingMatrix(nn.Module):
    def __init__(self, epsilon=1e-8):
        super().__init__()
        self.hilbert = AnalyticSignalApprox()
        self.epsilon = epsilon

    def forward(self, x):
        """
        输入: (batch, 2, seq_len)
        输出: (batch, 2, 2) 邻接矩阵
        """
        FP1, FP2 = x[:, 0], x[:, 1]  # 各(batch, seq_len)

        # 计算近似解析信号
        analytic1 = self.hilbert(FP1)
        analytic2 = self.hilbert(FP2)

        # 计算相位差（使用atan2的实部和虚部）
        phase1 = torch.atan2(analytic1.imag, analytic1.real)
        phase2 = torch.atan2(analytic2.imag, analytic2.real)
        phase_diff = phase1 - phase2

        # 计算PLV (使用cos和sin避免复数运算)
        cos_phase = torch.cos(phase_diff)
        sin_phase = torch.sin(phase_diff)

        # PLV = |mean(cos + i*sin)|
        mean_cos = torch.mean(cos_phase, dim=-1)
        mean_sin = torch.mean(sin_phase, dim=-1)
        raw_plv = torch.sqrt(mean_cos ** 2 + mean_sin ** 2)

        # 构建动态连接矩阵
        batch_size = x.shape[0]
        device = x.device

        # 创建单位矩阵为基础
        adj = torch.eye(2, device=device).repeat(batch_size, 1, 1)

        # 归一化PLV
        plv_min = raw_plv.min().clamp(min=self.epsilon)
        plv_max = raw_plv.max().clamp(min=plv_min + self.epsilon)
        normalized_plv = (raw_plv - plv_min) / (plv_max - plv_min)

        adj[:, 0, 1] = adj[:, 1, 0] = normalized_plv

        return adj


class GraphConvolution(nn.Module):
    def __init__(self, num_in, num_out, bias=False):
        super(GraphConvolution, self).__init__()
        self.num_in = num_in
        self.num_out = num_out
        self.weight = nn.Parameter(torch.FloatTensor(num_in, num_out))
        nn.init.kaiming_normal_(self.weight, )
        self.bias = None
        if bias:
            self.bias = nn.Parameter(torch.FloatTensor(num_out))
            nn.init.zeros_(self.bias)

    def forward(self, x, adj):
        out = torch.matmul(adj, x)
        out = torch.matmul(out, self.weight)
        if self.bias is not None:
            return out + self.bias
        else:
            return out


def generate_cheby_adj(A, K):
    support = []
    for i in range(K):
        if i == 0:
            support.append(torch.eye(A.shape[1], device=A.device))
        elif i == 1:
            support.append(A)
        else:
            temp = torch.matmul(support[-1], A)
            support.append(temp)
    return support


class Chebynet(nn.Module):
    def __init__(self, xdim, K, num_out, dropout):
        super(Chebynet, self).__init__()
        self.K = K
        self.gc1 = nn.ModuleList()
        self.dp = nn.Dropout(dropout)
        for i in range(K):
            self.gc1.append(GraphConvolution(xdim[2], num_out))

    def forward(self, x, L):
        adj = generate_cheby_adj(L, self.K)
        for i in range(len(self.gc1)):
            if i == 0:
                result = self.gc1[i](x, adj[i])
            else:
                result += self.gc1[i](x, adj[i])
        result = F.relu(result)
        return result


class GradientReversalFunction(torch.autograd.Function):
    @staticmethod
    def forward(ctx, x, alpha):
        ctx.alpha = alpha
        return x.view_as(x)

    @staticmethod
    def backward(ctx, grad_output):
        output = grad_output.neg() * ctx.alpha
        return output, None


class ChannelAttention(nn.Module):
    def __init__(self, channels, reduction_ratio=16):
        super(ChannelAttention, self).__init__()
        self.avg_pool = nn.AdaptiveAvgPool2d(1)
        self.max_pool = nn.AdaptiveMaxPool2d(1)
        self.mlp = nn.Sequential(
            nn.Conv2d(channels, channels // reduction_ratio, 1, bias=False),
            nn.ReLU(inplace=True),
            nn.Conv2d(channels // reduction_ratio, channels, 1, bias=False)
        )
        self.sigmoid = nn.Sigmoid()

    def forward(self, x):
        avg_out = self.mlp(self.avg_pool(x))
        max_out = self.mlp(self.max_pool(x))
        channel_weights = self.sigmoid(avg_out + max_out)
        return channel_weights


class SpatialAttention(nn.Module):
    def __init__(self, kernel_size=7):
        super(SpatialAttention, self).__init__()
        self.conv = nn.Conv2d(2, 1, kernel_size, padding=kernel_size // 2, bias=False)
        self.sigmoid = nn.Sigmoid()

    def forward(self, x):
        avg_out = torch.mean(x, dim=1, keepdim=True)
        max_out, _ = torch.max(x, dim=1, keepdim=True)
        spatial_features = torch.cat([avg_out, max_out], dim=1)
        spatial_weights = self.sigmoid(self.conv(spatial_features))
        return spatial_weights


class CBAM(nn.Module):
    def __init__(self, channels, reduction_ratio=16, kernel_size=7):
        super(CBAM, self).__init__()
        self.channel_attention = ChannelAttention(channels, reduction_ratio)
        self.spatial_attention = SpatialAttention(kernel_size)

    def forward(self, x):
        x = self.channel_attention(x) * x
        x_out = self.spatial_attention(x) * x
        return x_out + x


class SpectralConv2dSimplified(nn.Module):
    """简化的频谱卷积层 - 使用时域卷积替代STFT"""

    def __init__(self, in_channels, out_channels, kernel_size=64):
        super().__init__()
        # 修复：确保通道数匹配
        # 使用多尺度1D卷积来模拟频域特征
        self.conv1 = nn.Conv2d(in_channels, out_channels // 4, kernel_size=(1, kernel_size // 4),
                               padding=(0, kernel_size // 8))
        self.conv2 = nn.Conv2d(in_channels, out_channels // 4, kernel_size=(1, kernel_size // 2),
                               padding=(0, kernel_size // 4))
        self.conv3 = nn.Conv2d(in_channels, out_channels // 4, kernel_size=(1, kernel_size),
                               padding=(0, kernel_size // 2))
        self.conv4 = nn.Conv2d(in_channels, out_channels // 4, kernel_size=(1, 3), padding=(0, 1))  # 添加第四个分支

        # 确保最终输出通道数正确
        total_channels = (out_channels // 4) * 4
        self.final_conv = nn.Conv2d(total_channels, out_channels, kernel_size=1)

    def forward(self, x):
        # 多尺度特征提取
        feat1 = self.conv1(x)
        feat2 = self.conv2(x)
        feat3 = self.conv3(x)
        feat4 = self.conv4(x)

        # 合并特征
        combined = torch.cat([feat1, feat2, feat3, feat4], dim=1)
        out = self.final_conv(combined)
        return out


class SpatialAttention3D(nn.Module):
    def __init__(self, channels, time_points):
        super().__init__()
        # 确保卷积核大小不超过输入尺寸
        kernel_h = min(channels, 3) if channels > 1 else 1
        self.conv = nn.Sequential(
            nn.Conv2d(1, 1, kernel_size=(kernel_h, 3), padding=(0, 1)),
            nn.BatchNorm2d(1),
            nn.Sigmoid()
        )

    def forward(self, x):
        b, c, h, w = x.shape
        x_pool = x.mean(dim=1, keepdim=True)
        weights = self.conv(x_pool)
        # 确保权重尺寸匹配
        if weights.shape[2] != h or weights.shape[3] != w:
            weights = F.interpolate(weights, size=(h, w), mode='bilinear', align_corners=False)
        return weights


class TemporalAttention3D(nn.Module):
    def __init__(self, channels, time_points):
        super().__init__()
        mid_channels = max(1, channels // 4)
        self.temporal_conv = nn.Sequential(
            nn.Conv1d(channels, mid_channels, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.Conv1d(mid_channels, channels, kernel_size=3, padding=1),
            nn.Sigmoid()
        )

    def forward(self, x):
        b, c, h, w = x.shape
        x_pool = x.mean(dim=2)  # (batch, c, w)
        weights = self.temporal_conv(x_pool)
        return weights.unsqueeze(2)


class ST_SF_Module(nn.Module):
    """简化的时空-频谱多模态特征融合模块（移除FFT）"""

    def __init__(self, channels, time_points, embed_dim, reduction_ratio=16):
        super().__init__()
        self.channels = channels
        self.time_points = time_points

        # 时域分支
        self.temporal_branch = nn.Sequential(
            nn.Conv2d(1, 32, kernel_size=(1, 51), padding=(0, 25)),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.AvgPool2d(kernel_size=(1, 4)),
            CBAM(32)
        )

        # 频域分支（使用简化版本）
        self.freq_branch = nn.Sequential(
            SpectralConv2dSimplified(1, 32),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.AvgPool2d(kernel_size=(1, 4)),
            CBAM(32)
        )

        # 时空注意力
        self.spatial_att = SpatialAttention3D(channels, time_points // 4)
        self.temporal_att = TemporalAttention3D(64, time_points // 4)  # 64 = 32 + 32

        # 特征融合
        self.fusion = nn.Sequential(
            nn.Conv2d(64, 128, kernel_size=3, padding=1),  # 修改输入通道为64
            nn.BatchNorm2d(128),
            nn.ReLU(),
            nn.AdaptiveAvgPool2d(1),
            nn.Flatten(),
            nn.Linear(128, embed_dim)
        )

        # 残差连接
        self.residual = nn.Sequential(
            nn.Conv2d(1, 128, kernel_size=1),
            nn.AvgPool2d(kernel_size=(1, 4)),
            nn.AdaptiveAvgPool2d(1),
            nn.Flatten(),
            nn.Linear(128, embed_dim)
        )

    def forward(self, x):
        x = x.unsqueeze(1)

        # 时域处理
        temporal_feat = self.temporal_branch(x)

        # 频域处理（简化版）
        freq_feat = self.freq_branch(x)

        # 合并特征 - 在width维度上拼接
        # temporal_feat和freq_feat的形状应该是 (batch, 32, height, width)
        min_width = min(temporal_feat.shape[3], freq_feat.shape[3])
        temporal_feat = temporal_feat[:, :, :, :min_width]
        freq_feat = freq_feat[:, :, :, :min_width]

        combined = torch.cat([temporal_feat, freq_feat], dim=1)  # (batch, 64, height, width)

        # 时空注意力
        spatial_weights = self.spatial_att(combined)
        temporal_weights = self.temporal_att(combined)
        attended = combined * spatial_weights * temporal_weights

        # 特征融合
        fused = self.fusion(attended)

        # 残差连接
        residual = self.residual(x)

        return fused + residual


class CoarseFeatureExtractor(nn.Module):
    def __init__(self, n_channels, n_times, embed_dim=64, dropout=0.5):
        super(CoarseFeatureExtractor, self).__init__()
        self.temporal_conv = nn.Sequential(
            nn.Conv2d(1, 32, kernel_size=(1, 51), stride=1, padding=(0, 25), bias=False),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.AvgPool2d(kernel_size=(1, 4), stride=(1, 4)),
            nn.Dropout(dropout)
        )
        self.spatial_conv = nn.Sequential(
            nn.Conv2d(32, 64, kernel_size=(n_channels, 1), stride=1, bias=False),
            nn.BatchNorm2d(64),
            nn.ReLU(),
            nn.AvgPool2d(kernel_size=(1, 4), stride=(1, 4)),
            nn.Dropout(dropout)
        )
        self.attention = CBAM(64)
        self.extra_conv = nn.Sequential(
            nn.Conv2d(64, 128, kernel_size=(1, 25), stride=1, padding=(0, 12), bias=False),
            nn.BatchNorm2d(128),
            nn.ReLU(),
            nn.AvgPool2d(kernel_size=(1, 2), stride=(1, 2)),
            nn.Dropout(dropout)
        )
        self._feature_size = self._get_feature_size(n_times)
        self.feature_fusion = nn.Sequential(
            nn.Linear(self._feature_size, 256),
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(256, embed_dim)
        )

    def _get_feature_size(self, n_times):
        times_after_conv1 = (n_times // 4)
        times_after_conv2 = (times_after_conv1 // 4)
        times_after_conv3 = (times_after_conv2 // 2)
        return 128 * times_after_conv3

    def forward(self, x):
        x = x.unsqueeze(1)
        x = self.temporal_conv(x)
        x = self.spatial_conv(x)
        x = self.extra_conv(x)
        x = x.view(x.size(0), -1)
        x = self.feature_fusion(x)
        return x


class FineFeatureExtractor(nn.Module):
    def __init__(self, n_channels, n_times, embed_dim=64, dropout=0.5):
        super(FineFeatureExtractor, self).__init__()
        self.temporal_conv1 = nn.Conv2d(1, 16, kernel_size=(1, 25), stride=1, padding=(0, 12), bias=False)
        self.temporal_conv2 = nn.Conv2d(1, 16, kernel_size=(1, 51), stride=1, padding=(0, 25), bias=False)
        self.temporal_conv3 = nn.Conv2d(1, 16, kernel_size=(1, 101), stride=1, padding=(0, 50), bias=False)
        self.bn_temporal = nn.BatchNorm2d(48)
        self.relu = nn.ReLU()
        self._feature_size = self._get_feature_size(n_times)
        self.feature_fusion = nn.Sequential(
            nn.Linear(self._feature_size, 256),
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(256, embed_dim)
        )
        self.plm = PhaseLockingMatrix()
        self.SGCN1 = Chebynet(xdim=[BATCH_SIZE, 2, 500], K=2, num_out=250, dropout=0.)
        self.SGCN2 = Chebynet(xdim=[BATCH_SIZE, 2, 500], K=2, num_out=200, dropout=0.)
        self.SGCN3 = Chebynet(xdim=[BATCH_SIZE, 2, 500], K=2, num_out=50, dropout=0.)

    def _get_feature_size(self, n_times):
        return 48 * 2 * 500

    def forward(self, x):
        # 动态相位矩阵构建
        adj = self.plm(x)

        # 图卷积处理
        x1 = self.SGCN1(x, adj)
        x2 = self.SGCN2(x, adj)
        x3 = self.SGCN3(x, adj)
        x = torch.cat([x1, x2, x3], dim=2)

        x = x.unsqueeze(1)
        x1 = self.temporal_conv1(x)
        x2 = self.temporal_conv2(x)
        x3 = self.temporal_conv3(x)
        x = torch.cat([x1, x2, x3], dim=1)
        x = self.bn_temporal(x)
        x = self.relu(x)
        x = x.view(x.size(0), -1)
        x = self.feature_fusion(x)
        return x


class DomainClassifier(nn.Module):
    def __init__(self, embed_dim=128, hidden_dim=64):
        super(DomainClassifier, self).__init__()
        self.domain_classifier = nn.Sequential(
            nn.Linear(128, hidden_dim),
            nn.ReLU(),
            nn.Dropout(0.3),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Dropout(0.3),
            nn.Linear(hidden_dim, 1)
        )

    def forward(self, x, alpha=1.0):
        x = GradientReversalFunction.apply(x, alpha)
        return torch.sigmoid(self.domain_classifier(x))


class CoarseClassifier_1(nn.Module):
    def __init__(self, embed_dim=128, hidden_dim=64):
        super(CoarseClassifier_1, self).__init__()
        self.classifier = nn.Sequential(
            nn.Linear(128, hidden_dim),
            nn.BatchNorm1d(hidden_dim),
            nn.ReLU(),
            nn.Dropout(0.3),
            nn.Linear(hidden_dim, 2)
        )

    def forward(self, x):
        return self.classifier(x)


class FineClassifier_1(nn.Module):
    def __init__(self, embed_dim=128, hidden_dim=64):
        super(FineClassifier_1, self).__init__()
        self.classifier = nn.Sequential(
            nn.Linear(128, hidden_dim),
            nn.BatchNorm1d(hidden_dim),
            nn.ReLU(),
            nn.Dropout(0.3),
            nn.Linear(hidden_dim, 2)
        )

    def forward(self, x):
        return self.classifier(x)


class CoarseClassifier_2(nn.Module):
    def __init__(self, embed_dim=128, hidden_dim=64):
        super(CoarseClassifier_2, self).__init__()
        self.classifier = nn.Sequential(
            nn.Linear(128, hidden_dim),
            nn.BatchNorm1d(hidden_dim),
            nn.ReLU(),
            nn.Dropout(0.3),
            nn.Linear(hidden_dim, 2)
        )

    def forward(self, x):
        return self.classifier(x)


class FineClassifier_2(nn.Module):
    def __init__(self, embed_dim=128, hidden_dim=64):
        super(FineClassifier_2, self).__init__()
        self.classifier = nn.Sequential(
            nn.Linear(128, hidden_dim),
            nn.BatchNorm1d(hidden_dim),
            nn.ReLU(),
            nn.Dropout(0.3),
            nn.Linear(hidden_dim, 2)
        )

    def forward(self, x):
        return self.classifier(x)


class HierarchicalCrossSubModel(nn.Module):
    def __init__(self, n_channels, n_times, embed_dim=64):
        super(HierarchicalCrossSubModel, self).__init__()
        self.coarse_feature_extractor = CoarseFeatureExtractor(n_channels, n_times, embed_dim)
        self.fine_feature_extractor = FineFeatureExtractor(n_channels, n_times, embed_dim)
        self.domain_classifier = DomainClassifier(embed_dim)
        self.coarse_classifier_1 = CoarseClassifier_1(embed_dim)
        self.fine_classifier_1 = FineClassifier_1(embed_dim)
        self.coarse_classifier_2 = CoarseClassifier_2(embed_dim)
        self.fine_classifier_2 = FineClassifier_2(embed_dim)
        self.st_sf_module = ST_SF_Module(n_channels, n_times, embed_dim)
        self.alpha = nn.Parameter(torch.tensor(0.5))

    def forward(self, x, alpha=1.0):
        coarse_feat = self.coarse_feature_extractor(x)
        fine_feat = self.fine_feature_extractor(x)
        traditional_feat = self.alpha * coarse_feat + (1 - self.alpha) * fine_feat
        st_sf_feat = self.st_sf_module(x)
        combined_feat = torch.cat([traditional_feat, st_sf_feat], dim=1)
        domain_out = self.domain_classifier(combined_feat, alpha)
        coarse_out_1 = self.coarse_classifier_1(combined_feat)
        fine_out_1 = self.fine_classifier_1(combined_feat)
        coarse_out_2 = self.coarse_classifier_2(combined_feat)
        fine_out_2 = self.fine_classifier_2(combined_feat)
        return coarse_out_1, fine_out_1, coarse_out_2, fine_out_2, domain_out, fine_feat


def main():
    """主函数 - 完整的转换流程（修复维度问题）"""
    print("=" * 60)
    print("PyTorch模型转ONNX工具 (修复版本)")
    print("=" * 60)

    # 配置
    model_path = 'gr.pth'
    output_path = 'hierarchical_model.onnx'
    n_channels = 2
    n_times = 500
    embed_dim = 64

    print(f"使用参数:")
    print(f"  - n_channels: {n_channels}")
    print(f"  - n_times: {n_times}")
    print(f"  - embed_dim: {embed_dim}")

    # 创建模型
    print("\n正在创建模型...")
    try:
        model = HierarchicalCrossSubModel(n_channels=n_channels, n_times=n_times, embed_dim=embed_dim)
        print("✓ 模型创建成功")
    except Exception as e:
        print(f"✗ 模型创建失败: {e}")
        return False

    # 加载权重
    print("\n正在加载模型权重...")
    try:
        state_dict = torch.load(model_path, map_location='cpu')
        model_dict = model.state_dict()
        filtered_dict = {}
        mismatched_layers = []

        for k, v in state_dict.items():
            if k in model_dict:
                if model_dict[k].shape == v.shape:
                    filtered_dict[k] = v
                else:
                    mismatched_layers.append(f"{k}: saved {v.shape} vs model {model_dict[k].shape}")

        if mismatched_layers:
            print("⚠ 以下层将使用随机初始化（形状不匹配）:")
            for layer in mismatched_layers[:3]:
                print(f"  {layer}")
            if len(mismatched_layers) > 3:
                print(f"  ... 还有 {len(mismatched_layers) - 3} 个不匹配的层")

        model_dict.update(filtered_dict)
        model.load_state_dict(model_dict)
        print(f"✓ 成功加载 {len(filtered_dict)} 层，{len(mismatched_layers)} 层不匹配")

    except Exception as e:
        print(f"✗ 权重加载失败: {e}")
        return False

    model.eval()

    # 测试前向传播
    print("\n正在测试模型前向传播...")
    try:
        sample_input = torch.randn(1, n_channels, n_times)
        print(f"测试输入形状: {sample_input.shape}")

        with torch.no_grad():
            outputs = model(sample_input)

        print("✓ 前向传播成功!")
        print(f"输出数量: {len(outputs)}")
        for i, output in enumerate(outputs):
            print(f"  输出 {i} 形状: {output.shape}")

    except Exception as e:
        print(f"✗ 前向传播失败: {e}")
        print("错误详情:", str(e))
        return False

    # 导出ONNX
    print(f"\n正在导出ONNX模型到: {output_path}")
    try:
        input_names = ["eeg_input"]
        output_names = ["coarse_out_1", "fine_out_1", "coarse_out_2", "fine_out_2", "domain_out", "fine_feat"]

        torch.onnx.export(
            model,
            sample_input,
            output_path,
            export_params=True,
            opset_version=12,
            input_names=input_names,
            output_names=output_names,
            dynamic_axes={
                'eeg_input': {0: 'batch_size'},
                'coarse_out_1': {0: 'batch_size'},
                'fine_out_1': {0: 'batch_size'},
                'coarse_out_2': {0: 'batch_size'},
                'fine_out_2': {0: 'batch_size'},
                'domain_out': {0: 'batch_size'},
                'fine_feat': {0: 'batch_size'}
            },
            verbose=False
        )

        print("✓ ONNX导出成功!")

        # 验证ONNX模型
        try:
            import onnx
            onnx_model = onnx.load(output_path)
            onnx.checker.check_model(onnx_model)
            print("✓ ONNX模型验证通过!")
        except ImportError:
            print("⚠ 无法验证ONNX模型 (需要安装onnx库)")
        except Exception as e:
            print(f"⚠ ONNX模型验证失败: {e}")

    except Exception as e:
        print(f"✗ ONNX导出失败: {e}")
        return False

    print("=" * 60)
    print("转换完成!")
    print(f"✓ 输入模型: {model_path}")
    print(f"✓ 输出模型: {output_path}")
    print(f"✓ 输入数据格式: (batch_size, {n_channels}, {n_times})")
    print("=" * 60)

    return True


if __name__ == "__main__":
    success = main()
    if not success:
        print("转换失败，请检查错误信息")
        exit(1)