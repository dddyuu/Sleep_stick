import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np
from scipy.signal import hilbert
# from FP1_FP2.cross_subject_dataloader import *
import time
from torch.utils.data import DataLoader, TensorDataset, Subset

batch_size = 256

# ==================== 创新点1：动态相位同步矩阵 ====================
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


# ==================== 创新点2：空时连续卷积网络 ====================
class AsymmetricConvBlock(nn.Module):
    def __init__(self):
        super().__init__()
        # FP1侧重θ波段
        self.conv_theta = nn.Sequential(
            nn.Conv1d(1, 8, 25, dilation=2, padding='same'),
            nn.BatchNorm1d(8),
            nn.ReLU()
        )
        # FP2侧重α波段
        self.conv_alpha = nn.Sequential(
            nn.Conv1d(1, 8, 50, dilation=1, padding='same'),
            nn.BatchNorm1d(8),
            nn.ReLU()
        )
        #通道降回2
        self.conv = nn.Sequential(
            nn.Conv1d(16, 2, 1),
            nn.BatchNorm1d(2),
            nn.ReLU()
        )

    def forward(self, x):
        x1 = self.conv_theta(x[:, :1])  # FP1
        x2 = self.conv_alpha(x[:, 1:])  # FP2
        x = torch.cat([x1, x2], dim=1)
        x = self.conv(x)

        return x


class DynamicGraphConv(nn.Module):
    def __init__(self, in_channels):
        super().__init__()
        self.gcn_weights = nn.Parameter(torch.randn(in_channels, in_channels))

    def forward(self, x, adj):
        # x: (batch, channels, features)
        # adj: (batch, 2, 2)
        norm_adj = F.softmax(adj, dim=-1)
        return torch.einsum('bcf,bij->bjf', x, torch.matmul(norm_adj, self.gcn_weights))

class GraphConvolution(nn.Module):

    def __init__(self, num_in, num_out, bias=False):

        super(GraphConvolution, self).__init__()

        self.num_in = num_in
        self.num_out = num_out
        self.weight = nn.Parameter(torch.FloatTensor(num_in, num_out).cuda())
        nn.init.kaiming_normal_(self.weight, )
        self.bias = None
        if bias:
            self.bias = nn.Parameter(torch.FloatTensor(num_out).cuda())
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
            support.append(torch.eye(A.shape[1]).cuda())
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


# ==================== 创新点3：元学习适配 ====================
class MetaLearner(nn.Module):
    def __init__(self, input_dim=64):
        super().__init__()
        self.fc = nn.Sequential(
            nn.Linear(input_dim, 32),
            nn.ReLU(),
            nn.Linear(32, 16)
        )

    def forward(self, support_data):
        # support_data: (batch, 2, seq_len)
        features = support_data.mean(dim=-1)  # 简单特征提取
        return self.fc(features.flatten(1))

#动态传输层
class SamplesLoss(nn.Module):
    def __init__(self, loss_type="sinkhorn", p=2, blur=0.05, scaling=0.9,
                 reach=None, diameter=None, batch_reduction="mean"):
        super().__init__()
        self.loss_type = loss_type.lower()
        self.p = p
        self.blur = blur
        self.scaling = scaling
        self.reach = reach
        self.diameter = diameter
        self.batch_reduction = batch_reduction

        if self.loss_type != "sinkhorn":
            raise NotImplementedError("Only 'sinkhorn' loss is currently implemented")

    def forward(self, x, y):
        B = x.size(0)

        # Compute all pairwise distances in one go
        C_xy = self._pairwise_distances(x, y)  # (B, N, M)

        # Sinkhorn iterations
        P_xy = self.sinkhorn(C_xy)
        transport_cost = torch.sum(P_xy * C_xy, dim=(1, 2))  # (B,)

        # Compute normalization terms (Sinkhorn divergence)
        with torch.no_grad():
            C_xx = self._pairwise_distances(x, x)  # (B, N, N)
            P_xx = self.sinkhorn(C_xx)
            cost_xx = torch.sum(P_xx * C_xx, dim=(1, 2))  # (B,)

            C_yy = self._pairwise_distances(y, y)  # (B, M, M)
            P_yy = self.sinkhorn(C_yy)
            cost_yy = torch.sum(P_yy * C_yy, dim=(1, 2))  # (B,)

        # Sinkhorn divergence
        loss = 2 * transport_cost - cost_xx - cost_yy  # (B,)
        return loss

    def _pairwise_distances(self, x, y):
        """Optimized pairwise distance computation"""
        # Use torch.cdist if available (faster)
        if hasattr(torch, 'cdist') and self.p == 2:
            return torch.cdist(x, y, p=2) ** 2
        else:
            x_col = x.unsqueeze(2)  # (B, N, 1, D)
            y_lin = y.unsqueeze(1)  # (B, 1, M, D)
            return torch.sum((torch.abs(x_col - y_lin)) ** self.p, dim=3)

    def sinkhorn(self, C):
        """Optimized Sinkhorn iterations."""
        B, N, M = C.shape
        device = C.device

        # Normalize cost matrix more efficiently
        if self.reach is not None:
            C = C / self.reach
        elif self.diameter is not None:
            C = C / self.diameter
        else:
            C_max = C.view(B, -1).max(dim=1)[0].view(B, 1, 1)
            C = C / (C_max + 1e-6)

        # Initialize dual vectors
        f = torch.zeros(B, N, device=device)
        g = torch.zeros(B, M, device=device)

        # Pre-compute frequently used terms
        blur_inv = 1.0 / self.blur
        scaling_comp = (1.0 - self.scaling)

        for _ in range(100):  # Fixed number of iterations
            # Update g
            g_new = -self.blur * torch.logsumexp((f.unsqueeze(2) - C) * blur_inv, dim=1)
            # Update f
            f_new = -self.blur * torch.logsumexp((g_new.unsqueeze(1) - C) * blur_inv, dim=2)

            # Apply scaling
            f = self.scaling * f + scaling_comp * f_new
            g = self.scaling * g + scaling_comp * g_new

        # Compute transport plan
        P = torch.exp((f.unsqueeze(2) + g.unsqueeze(1) - C) * blur_inv)
        return P / (N * M)  # Normalization

class DynamicOTLayer(nn.Module):
    """动态最优传输层"""
    def __init__(self, feat_dim, label_dim=1, epsilon=0.1):
        super().__init__()
        self.ot_loss = SamplesLoss("sinkhorn", p=2, blur=epsilon)
        self.label_proj = nn.Linear(1, feat_dim)

    def forward(self, features, labels):
        # features: (B, T, D), labels: (B, L)
        labels_proj = self.label_proj(labels.reshape(-1,1).unsqueeze(1))  # (B, 1, D)

        # 计算时间序列OT距离
        cost_matrix = self._dtw_cost_matrix(features, labels_proj)

        gamma = self.ot_loss(features, labels_proj)
        # print("gamma:", gamma.shape)

        # 特征对齐
        aligned_features = gamma.unsqueeze(1).unsqueeze(1) * features
        return aligned_features, cost_matrix

    def _dtw_cost_matrix(self, x, y):
        # 动态时间规整距离矩阵
        x, y = x.unsqueeze(-1).permute(0,3,1,2), y.unsqueeze(-2)
        return (x - y).abs().sum(-1).sqrt()


# ==================== 完整模型 ====================
class NDTRSModel(nn.Module):
    def __init__(self, num_classes=3, xdim=[batch_size, 2, 250], kadj=2, num_out=63, dropout=0.):
        super().__init__()
        # 特征提取
        self.asym_conv = AsymmetricConvBlock()
        self.plm = PhaseLockingMatrix()
        self.graph_conv = DynamicGraphConv(2)
        self.SGCN1 = Chebynet(xdim, kadj, num_out, dropout)

        # 动态归一化
        self.dynamic_norm = nn.InstanceNorm1d(16)

        # 元学习组件
        self.meta_learner = MetaLearner()

        #动态最优传输
        self.ot_layer = DynamicOTLayer(2, num_classes)
        # 反事实模块
        self.cf_proj = nn.Linear(2, 2)

        # 分类头
        self.classifier = nn.Sequential(
            nn.Linear(2*250, 100),
            nn.Dropout(0.2),
            nn.ReLU(),
            nn.Linear(100, num_classes)
        )

    def forward(self, x, y, do_cf=False, support_data=None):
        # 1. 非对称卷积处理
        x = self.asym_conv(x)  # (batch, 16, seq_len)

        # 2. 动态相位矩阵构建
        adj = self.plm(x)

        # 3. 图卷积处理
        # x = self.graph_conv(x, adj)
        x = self.SGCN1(x, adj)

        # 最优传输对齐
        x = x.transpose(1, 2)
        if y is not None:
            x_align, ot_cost = self.ot_layer(x, y)
        else:
            x_align = x.mean(1)  # 测试时使用均值

        # 反事实干预
        if do_cf:
            cf_feat = self.cf_proj(x_align.detach())
            x_align = x_align + cf_feat - cf_feat.mean(0, keepdim=True)

        # 4. 元学习适配
        if support_data is not None:
            meta_features = self.meta_learner(support_data)
            x_align = x_align + meta_features.unsqueeze(-1)

        # print(x.shape)
        # 5. 分类决策
        # x = F.adaptive_avg_pool1d(x, 1).squeeze(-1)
        x = x_align.reshape(-1,2*250)

        return self.classifier(x)

class CounterfactualLoss(nn.Module):
    def __init__(self, alpha=0.5):
        super().__init__()
        self.alpha = alpha

    def forward(self, logits, y, cf_layer):
        # 计算反事实扰动
        cf_weight = cf_layer.weight
        perturbation = torch.randn_like(cf_weight) * self.alpha
        cf_logits = logits + (cf_weight + perturbation).mean()

        # 保持预测一致性
        return F.kl_div(
            F.log_softmax(cf_logits, dim=-1),
            F.softmax(logits.detach(), dim=-1),
            reduction='batchmean'
        )

device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
start_time = time.time()


#训练集
X_train = np.concatenate([np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/data/lsb.npy"), np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/data/das.npy"),
                          np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/data/cyl.npy"), np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/data/gr.npy"),
                          np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/data/hzh.npy"), np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/data/nj.npy")])
X_train = torch.tensor(X_train.real.astype(float), dtype=torch.float)
# X_train = (X_train - X_train.mean(dim=-1, keepdim=True)) / (X_train.std(dim=-1, keepdim=True) + 1e-6)
# X_train = torch.cat([X_train[0:60,:,:], X_train[120:180,:,:], X_train[240:300,:,:]])
print(X_train.shape)

Y_train = np.concatenate([np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/label/lsb.npy"), np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/label/das.npy"),
                          np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/label/cyl.npy"), np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/label/gr.npy"),
                          np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/label/hzh.npy"), np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/label/nj.npy")])
Y_train = torch.from_numpy(np.array(Y_train)).float()
# Y_train = torch.cat([Y_train[0:60,], Y_train[120:180,], Y_train[240:300,]])

#测试集
X_test = torch.tensor(np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/data/wsm.npy").real.astype(float), dtype=torch.float)
# X_test = (X_test - X_test.mean(dim=-1, keepdim=True)) / (X_test.std(dim=-1, keepdim=True) + 1e-6)
# X_test = torch.cat([X_test[60:120,:,:], X_test[180:240,:,:], X_test[300:360,:,:]])

Y_test = torch.from_numpy(np.array(np.load("G:/博士成果/认知工作负荷/额贴离线数据/data_select/label/wsm.npy"))).float()
# Y_test = torch.cat([Y_test[60:120,], Y_test[180:240,], Y_test[300:360,]])

train_data = TensorDataset(X_train, Y_train)
test_data = TensorDataset(X_test, Y_test)
train_loader = DataLoader(train_data, batch_size=batch_size, drop_last=True, shuffle=True)
test_loader = DataLoader(test_data, batch_size=batch_size, drop_last=True, shuffle=True)

num_epochs = 500
learning_rate = 0.0005
device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

min_acc = 0.3

model = NDTRSModel(num_classes=3, xdim=[batch_size, 2, 250], kadj=2, num_out=250, dropout=0.).to(device)
# model.load_state_dict(torch.load("G:/博士成果/认知工作负荷/FP1_FP2模型/1.pth"))
# model.eval()
criterion_cf = CounterfactualLoss(alpha=0.5)
criterion_cls = nn.CrossEntropyLoss()
optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate, weight_decay=0.03)

train_acc_list = []
train_loss_list = []
test_acc_list = []
test_loss_list = []

target_iter = iter(test_loader)

for epoch in range(num_epochs):
    # -------------------------------------------------
    total_train_acc = 0
    total_train_loss = 0
    class_loss_total = 0
    domain_loss_total = 0
    alignment_loss_total = 0
    train_loss = 0

    for source_data, source_labels in train_loader:
        source_data = source_data.to(device)
        source_labels = source_labels.to(device)

        source_class_output = model(source_data, source_labels) #, ot_cost
        source_labels = torch.tensor(source_labels, dtype=torch.int64)

        class_loss = criterion_cls(source_class_output, source_labels)
        cf_loss = criterion_cf(source_class_output, source_labels, model.cf_proj)
        loss = class_loss + 0.3 * cf_loss

        optimizer.zero_grad()
        loss.backward()
        nn.utils.clip_grad_norm_(model.parameters(), 2.0)
        optimizer.step()

        train_acc = (source_class_output.argmax(dim=1) == source_labels).sum()

        total_train_loss = total_train_loss + loss.item()
        total_train_acc += train_acc

    total_test_acc = 0
    total_test_loss = 0

    with torch.no_grad():
        for de, labels in test_loader:
            de = de.to(device)
            labels = labels.to(device)

            class_output = model(de, labels)
            test_loss = criterion_cls(class_output, labels.long())

            test_acc = (class_output.argmax(dim=1) == labels).sum()

            test_loss_list.append(test_loss)
            total_test_loss = total_test_loss + test_loss.item()

            test_acc_list.append(test_acc)
            total_test_acc += test_acc

    if (total_test_acc / len(test_data)) > min_acc:
        min_acc = total_test_acc / len(test_data)
        # res_TP_TN_FP_FN = TP_TN_FP_FN
        # torch.save(model.state_dict(), 'G:/博士成果/认知工作负荷/FP1_FP2模型/1.pth')
    # print result
    # print(train_len, test_len)
    print("Epoch: {}/{} ".format(epoch + 1, num_epochs),
          "Training Loss: {:.4f} ".format(total_train_loss / len(train_loader)),
          "Training Accuracy: {:.4f} ".format(total_train_acc / len(train_data)),
          "Test Loss: {:.4f} ".format(total_test_loss / len(test_loader)),
          "Test Accuracy: {:.4f}".format(total_test_acc / len(test_data))
          )
print(min_acc)