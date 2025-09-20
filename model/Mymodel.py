import torch
import torch.nn as nn
import numpy as np
import warnings
import torch.nn.functional as F

# 忽略警告
warnings.filterwarnings("ignore")
BATCH_SIZE = 128

class AnalyticSignal(nn.Module):
    def __init__(self):
        super().__init__()

    def forward(self, x):
        """
        正确的希尔伯特变换实现
        输入: (batch, seq_len)
        输出: (batch, seq_len) 复数张量
        """
        N = x.shape[-1]

        # FFT
        X = torch.fft.fft(x, dim=-1)

        # 创建希尔伯特滤波器
        h = torch.zeros_like(x)
        if N % 2 == 0:
            h[..., 0] = h[..., N // 2] = 1
            h[..., 1:N // 2] = 2
        else:
            h[..., 0] = 1
            h[..., 1:(N + 1) // 2] = 2

        # 频域滤波
        X = X * h

        # IFFT
        return torch.fft.ifft(X, dim=-1)

class PhaseLockingMatrix(nn.Module):
    def __init__(self, epsilon=1e-8):
        super().__init__()
        self.hilbert = AnalyticSignal()
        self.epsilon = epsilon  # 防止除以零的小量

    def forward(self, x):
        """
        输入: (batch, 2, seq_len)
        输出: (batch, 2, 2) 邻接矩阵
        """
        FP1, FP2 = x[:, 0], x[:, 1]  # 各(batch, seq_len)

        # 计算解析信号
        analytic1 = self.hilbert(FP1)
        analytic2 = self.hilbert(FP2)

        # 计算相位差
        phase_diff = torch.atan2(analytic1.imag, analytic1.real) - torch.atan2(analytic2.imag, analytic2.real)
        # print((torch.exp(1j * phase_diff)).shape)

        # 计算PLV (Phase Locking Value)
        raw_plv = torch.abs(torch.mean(torch.exp(phase_diff), dim=-1))  # (batch,)
        # print(plv.shape)

        # 构建动态连接矩阵
        batch_size = x.shape[0]
        device = x.device

        # 创建单位矩阵为基础
        adj = torch.eye(2, device=device).repeat(batch_size, 1, 1)  # (batch, 2, 2)

        # 设置PLV值
        adj[:, 0, 1] = raw_plv
        adj[:, 1, 0] = raw_plv

        # 归一化方案（保持对角线为1）
        # 方法1: 最大最小值归一化 (推荐)
        plv_min = raw_plv.min().clamp(min=self.epsilon)
        plv_max = raw_plv.max().clamp(min=plv_min + self.epsilon)
        normalized_plv = (raw_plv - plv_min) / (plv_max - plv_min)

        adj[:, 0, 1] = adj[:, 1, 0] = normalized_plv

        # # 强制确保数值范围
        # adj[:, 0, 1] = adj[:, 0, 1].clamp(min=self.epsilon, max=1 - self.epsilon)
        # adj[:, 1, 0] = adj[:, 1, 0].clamp(min=self.epsilon, max=1 - self.epsilon)

        return adj
class GraphConvolution(nn.Module):
    def __init__(self, num_in, num_out, bias=False):
        super(GraphConvolution, self).__init__()
        self.num_in = num_in
        self.num_out = num_out
        # 移除硬编码的.cuda()，让框架自动处理设备
        self.weight = nn.Parameter(torch.FloatTensor(num_in, num_out))
        nn.init.kaiming_normal_(self.weight)
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
    device = A.device  # 从输入张量获取设备信息
    for i in range(K):
        if i == 0:
            support.append(torch.eye(A.shape[1], device=device))  # 使用相同设备
        elif i == 1:
            support.append(A)
        else:
            temp = torch.matmul(support[-1], A)
            support.append(temp)
    return support
# class GraphConvolution(nn.Module):
#
#     def __init__(self, num_in, num_out, bias=False):
#
#         super(GraphConvolution, self).__init__()
#
#         self.num_in = num_in
#         self.num_out = num_out
#         self.weight = nn.Parameter(torch.FloatTensor(num_in, num_out).cuda())
#         nn.init.kaiming_normal_(self.weight, )
#         self.bias = None
#         if bias:
#             self.bias = nn.Parameter(torch.FloatTensor(num_out).cuda())
#             nn.init.zeros_(self.bias)
#
#     def forward(self, x, adj):
#         out = torch.matmul(adj, x)
#         out = torch.matmul(out, self.weight)
#         if self.bias is not None:
#             return out + self.bias
#         else:
#             return out
#
# def generate_cheby_adj(A, K):
#     support = []
#     for i in range(K):
#         if i == 0:
#             support.append(torch.eye(A.shape[1]).cuda())
#         elif i == 1:
#             support.append(A)
#         else:
#             temp = torch.matmul(support[-1], A)
#             support.append(temp)
#     return support

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

# 梯度反转层
class GradientReversalFunction(torch.autograd.Function):
    @staticmethod
    def forward(ctx, x, alpha):
        ctx.alpha = alpha
        return x.view_as(x)

    @staticmethod
    def backward(ctx, grad_output):
        output = grad_output.neg() * ctx.alpha
        return output, None

class CBAM(nn.Module):
    def __init__(self, channels, reduction_ratio=16, kernel_size=7):
        super(CBAM, self).__init__()
        self.channel_attention = ChannelAttention(channels, reduction_ratio)
        self.spatial_attention = SpatialAttention(kernel_size)

    def forward(self, x):
        x = self.channel_attention(x) * x
        x_out = self.spatial_attention(x) * x
        return x_out + x

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
        self.conv = nn.Conv2d(2, 1, kernel_size, padding=kernel_size//2, bias=False)
        self.sigmoid = nn.Sigmoid()

    def forward(self, x):
        avg_out = torch.mean(x, dim=1, keepdim=True)
        max_out, _ = torch.max(x, dim=1, keepdim=True)
        spatial_features = torch.cat([avg_out, max_out], dim=1)
        spatial_weights = self.sigmoid(self.conv(spatial_features))
        return spatial_weights


#addition
class ST_SF_Module(nn.Module):
    """时空-频谱多模态特征融合模块"""

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

        # 频域分支 (使用短时傅里叶变换)
        self.freq_branch = nn.Sequential(
            SpectralConv2d(1, 32, n_fft=64, hop_length=16),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.AvgPool2d(kernel_size=(1, 4)),
            CBAM(32)
        )

        # 时空注意力
        self.spatial_att = SpatialAttention3D(channels, time_points // 4)
        self.temporal_att = TemporalAttention3D(channels, time_points // 4)

        # 特征融合
        self.fusion = nn.Sequential(
            nn.Conv2d(32, 128, kernel_size=3, padding=1),
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
        # 输入形状: (batch, channels, time_points)
        x = x.unsqueeze(1)  # (batch, 1, channels, time_points)

        # 时域处理
        temporal_feat = self.temporal_branch(x)

        # 频域处理
        freq_feat = self.freq_branch(x)

        # 合并特征
        combined = torch.cat([temporal_feat, freq_feat], dim=3)

        # 时空注意力
        spatial_weights = self.spatial_att(combined)
        temporal_weights = self.temporal_att(combined)
        attended = combined * spatial_weights * temporal_weights

        # 特征融合
        fused = self.fusion(attended)

        # 残差连接
        residual = self.residual(x)

        return fused + residual

class SpectralConv2d(nn.Module):
    """频谱卷积层 - 在频域进行卷积操作"""

    def __init__(self, in_channels, out_channels, n_fft=64, hop_length=16):
        super().__init__()
        self.n_fft = n_fft
        self.hop_length = hop_length
        self.conv = nn.Conv2d(in_channels * 66, out_channels, kernel_size=3, padding=1)

    def forward(self, x):
        # x形状: (batch, 1, channels, time_points)
        batch, _, channels, _ = x.shape

        # 对每个通道进行STFT
        stft_real = []
        stft_imag = []
        for i in range(channels):
            # 计算单通道STFT
            stft = torch.stft(x[:, 0, i, :], n_fft=self.n_fft, hop_length=self.hop_length,
                              return_complex=True)
            stft_real.append(stft.real)
            stft_imag.append(stft.imag)

        # 合并所有通道
        stft_real = torch.stack(stft_real, dim=1).unsqueeze(1)  # (batch, 1, channels, freq, time)
        stft_imag = torch.stack(stft_imag, dim=1).unsqueeze(1)

        # 合并实部和虚部
        stft_combined = torch.cat([stft_real, stft_imag], dim=1)  # (batch, 2, channels, freq, time)

        # 调整维度用于2D卷积
        stft_combined = stft_combined.permute(0, 1, 3, 2, 4)  # (batch, 2, freq, channels, time)
        b, _, f, c, t = stft_combined.shape
        stft_combined = stft_combined.reshape(b, -1, c, t)  # (batch, 2*freq, channels, time)

        # 频域卷积
        out = self.conv(stft_combined)
        return out

class SpatialAttention3D(nn.Module):
    """3D空间注意力(通道×空间)"""

    def __init__(self, channels, time_points):
        super().__init__()
        self.conv = nn.Sequential(
            nn.Conv2d(1, 1, kernel_size=(channels, 3), padding=(0, 1)),
            nn.BatchNorm2d(1),
            nn.Sigmoid()
        )
        self.time_points = time_points

    def forward(self, x):
        # x形状: (batch, c, h, w)
        b, c, h, w = x.shape
        x_pool = x.mean(dim=1, keepdim=True)  # (batch, 1, h, w)
        weights = self.conv(x_pool)  # (batch, 1, 1, w)
        weights = weights.view(b, 1, 1, w)
        return weights

class TemporalAttention3D(nn.Module):
    """3D时间注意力"""

    def __init__(self, channels, time_points):
        super().__init__()
        self.temporal_conv = nn.Sequential(
            nn.Conv1d(channels*16, channels, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.Conv1d(channels, channels*16, kernel_size=3, padding=1),
            nn.Sigmoid()
        )

    def forward(self, x):
        # x形状: (batch, c, h, w)
        b, c, h, w = x.shape
        x_pool = x.mean(dim=2)  # (batch, c, w)
        weights = self.temporal_conv(x_pool)  # (batch, c, w)
        return weights.unsqueeze(2)  # (batch, c, 1, w)



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
        # self.attention = nn.Sequential(
        #     nn.AdaptiveAvgPool2d(1),
        #     nn.Conv2d(64, 64 // 16, kernel_size=1),
        #     nn.ReLU(),
        #     nn.Conv2d(64 // 16, 64, kernel_size=1),
        #     nn.Sigmoid()
        # )
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
        self.plm = PhaseLockingMatrix()
        self.SGCN1 = Chebynet(xdim=[BATCH_SIZE, 2, 500], K=2, num_out=500, dropout=0.)

    def _get_feature_size(self, n_times):
        times_after_conv1 = (n_times // 4)
        times_after_conv2 = (times_after_conv1 // 4)
        times_after_conv3 = (times_after_conv2 // 2)
        return 128 * times_after_conv3

    def forward(self, x):
        # # 动态相位矩阵构建
        # adj = self.plm(x)
        #
        # # 图卷积处理
        # # x = self.graph_conv(x, adj)
        # x = self.SGCN1(x, adj)

        x = x.unsqueeze(1)
        x = self.temporal_conv(x)
        x = self.spatial_conv(x)
        # print(x.shape)
        # x = self.attention(x)
        # x = x * attention_weights
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
        self.pool = nn.AvgPool2d(kernel_size=(1, 4), stride=(1, 4))
        self.dropout = nn.Dropout(dropout)
        self.spatial_conv = nn.Sequential(
            nn.Conv2d(48, 64, kernel_size=(n_channels, 1), stride=1, bias=False),
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
        self.plm = PhaseLockingMatrix()
        self.SGCN1 = Chebynet(xdim=[BATCH_SIZE, 2, 500], K=2, num_out=125, dropout=0.)
        self.SGCN2 = Chebynet(xdim=[BATCH_SIZE, 2, 500], K=2, num_out=100, dropout=0.)
        self.SGCN3 = Chebynet(xdim=[BATCH_SIZE, 2, 500], K=2, num_out=25, dropout=0.)

    def _get_feature_size(self, n_times):
        times_after_conv1 = (n_times // 4)
        times_after_conv2 = (times_after_conv1 // 4)
        times_after_conv3 = (times_after_conv2 // 2)
        return 48*2*250#128 * times_after_conv3

    def forward(self, x):
        # 动态相位矩阵构建
        adj = self.plm(x)

        # 图卷积处理
        # x = self.graph_conv(x, adj)
        x1 = self.SGCN1(x, adj)
        x2 = self.SGCN2(x, adj)
        x3 = self.SGCN3(x, adj)
        x = torch.cat([x1, x2, x3], dim=2)
        # print(x.shape)


        x = x.unsqueeze(1)
        # print(x.shape)
        x1 = self.temporal_conv1(x)
        x2 = self.temporal_conv2(x)
        x3 = self.temporal_conv3(x)
        # print(x1.shape, x2.shape, x3.shape)
        x = torch.cat([x1, x2, x3], dim=1)
        # print(x.shape)
        x = self.bn_temporal(x)
        x = self.relu(x)
        # x = self.pool(x)
        # # print(x.shape)
        # # x = self.dropout(x)
        # x = self.spatial_conv(x)
        # x = self.attention(x)
        # x = self.extra_conv(x)
        # print(x.shape)
        x = x.view(x.size(0), -1)

        x = self.feature_fusion(x)
        return x

import torch
import torch.nn as nn


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
            nn.Linear(hidden_dim, 2)  # 二分类：0=低负荷, 1=中高负荷
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
            nn.Linear(hidden_dim, 2)  # 二分类：0=中负荷, 1=高负荷
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
            nn.Linear(hidden_dim, 2)  # 二分类：0=低中负荷, 1=高负荷
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
            nn.Linear(hidden_dim, 2)  # 二分类：0=低负荷, 1=中负荷
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

        # 新增ST-SF模块
        self.st_sf_module = ST_SF_Module(n_channels, n_times, embed_dim)
        # 特征融合权重
        self.alpha = nn.Parameter(torch.tensor(0.5))  # 可学习的融合权重

    def forward(self, x, alpha=1.0):
        coarse_feat = self.coarse_feature_extractor(x)
        fine_feat = self.fine_feature_extractor(x)

        traditional_feat = self.alpha * coarse_feat + (1 - self.alpha) * fine_feat

        # ST-SF特征提取
        st_sf_feat = self.st_sf_module(x)
        # 特征融合
        combined_feat = torch.cat([traditional_feat, st_sf_feat], dim=1)
        # print(combined_feat.shape)

        domain_out = self.domain_classifier(combined_feat, alpha)
        # print(domain_out.shape)
        coarse_out_1 = self.coarse_classifier_1(combined_feat)
        fine_out_1 = self.fine_classifier_1(combined_feat)
        coarse_out_2 = self.coarse_classifier_2(combined_feat)
        fine_out_2 = self.fine_classifier_2(combined_feat)
        return coarse_out_1, fine_out_1, coarse_out_2, fine_out_2, combined_feat, domain_out, fine_feat

def convert_to_original_labels_1(coarse_preds, fine_preds):
    """
    将分层预测转换回原始三分类标签。
    参数:
        coarse_preds: 粗分类预测 (0=低负荷, 1=中高负荷)
        fine_preds: 细分类预测 (0=中负荷, 1=高负荷)
    返回:
        original_preds: 原始三分类标签 (0=低负荷, 1=中负荷, 2=高负荷)
    """
    original_preds = np.zeros_like(coarse_preds)

    # 低负荷
    original_preds[coarse_preds == 0] = 0

    # 中负荷 (粗分类=1 且 细分类=0)
    mid_mask = (coarse_preds == 1) & (fine_preds == 0)
    original_preds[mid_mask] = 1

    # 高负荷 (粗分类=1 且 细分类=1)
    high_mask = (coarse_preds == 1) & (fine_preds == 1)
    original_preds[high_mask] = 2

    return original_preds

def convert_to_original_labels_2(coarse_preds, fine_preds):
    """
    将分层预测转换回原始三分类标签。
    参数:
        coarse_preds: 粗分类预测 (0=低中负荷, 1=高负荷)
        fine_preds: 细分类预测 (0=低负荷, 1=中负荷)
    返回:
        original_preds: 原始三分类标签 (0=低负荷, 1=中负荷, 2=高负荷)
    """
    original_preds = np.zeros_like(coarse_preds)

    # 低负荷 (粗分类=0 且 细分类=0)
    low_mask = (coarse_preds == 0) & (fine_preds == 0)
    original_preds[low_mask == 0] = 0

    # 中负荷 (粗分类=0 且 细分类=1)
    mid_mask = (coarse_preds == 0) & (fine_preds == 1)
    original_preds[mid_mask] = 1

    # 高负荷
    original_preds[coarse_preds == 1] = 2

    return original_preds

def convert_hierarchical_to_original_prob(coarse_probs_1, fine_probs_1, coarse_probs_2, fine_probs_2, fusion_weight):
    """
    将分层预测的概率转换为原始三分类概率
    使用可学习的融合权重
    """
    batch_size = coarse_probs_1.size(0)

    # 策略1: 低-中高 + 中-高
    prob_strategy1 = torch.zeros(batch_size, 3, device=coarse_probs_1.device)
    prob_strategy1[:, 0] = coarse_probs_1[:, 0]  # 低负荷
    prob_strategy1[:, 1] = coarse_probs_1[:, 1] * fine_probs_1[:, 0]  # 中负荷
    prob_strategy1[:, 2] = coarse_probs_1[:, 1] * fine_probs_1[:, 1]  # 高负荷

    # 策略2: 低中-高 + 低-中
    prob_strategy2 = torch.zeros(batch_size, 3, device=coarse_probs_2.device)
    prob_strategy2[:, 0] = coarse_probs_2[:, 0] * fine_probs_2[:, 0]  # 低负荷
    prob_strategy2[:, 1] = coarse_probs_2[:, 0] * fine_probs_2[:, 1]  # 中负荷
    prob_strategy2[:, 2] = coarse_probs_2[:, 1]  # 高负荷

    # 使用可学习的权重进行融合
    # weight = torch.softmax(fusion_weight, dim=0)
    weight = fusion_weight
    # print(weight)
    final_probs = weight[0] * prob_strategy1 + weight[1] * prob_strategy2

    return final_probs
if __name__ == "__main__":
    model = HierarchicalCrossSubModel(n_channels=2, n_times=500, embed_dim=64)
    model = model.cuda()
    x = torch.randn(128, 2, 500).cuda()
    coarse_out_1, fine_out_1, coarse_out_2, fine_out_2, _, domain_out, fine_feat = model(x)
    print(coarse_out_1.shape)  # (128, 2)
    print(fine_out_1.shape)    # (128, 2)
    print(coarse_out_2.shape)  # (128, 2)
    print(fine_out_2.shape)    # (128, 2)
    print(domain_out.shape)    # (128, 1)
    print(fine_feat.shape)     # (128, 128)