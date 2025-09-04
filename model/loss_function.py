import torch
import torch.nn as nn
import torch.nn.functional as F

class ClassAlignmentLoss(nn.Module):
    def __init__(self, temperature=0.05, inter_weight=1.0, intra_weight=1.0):
        super(ClassAlignmentLoss, self).__init__()
        self.temperature = temperature  # 控制分布的锐度
        self.inter_weight = inter_weight  # 类间分离损失的权重
        self.intra_weight = intra_weight  # 类内对齐损失的权重

    def forward(self, source_features, target_features, source_labels, target_pseudo_labels):
        source_features = F.normalize(source_features, dim=1)
        target_features = F.normalize(target_features, dim=1)
        source_original_labels = self.convert_to_original_labels(source_labels)
        target_original_labels = self.convert_to_original_labels(target_pseudo_labels)

        # 类内对齐损失
        intra_class_loss = 0.0
        valid_classes = 0
        for class_idx in range(3):
            source_class_idx = (source_original_labels == class_idx).nonzero(as_tuple=True)[0]
            target_class_idx = (target_original_labels == class_idx).nonzero(as_tuple=True)[0]
            if len(source_class_idx) == 0 or len(target_class_idx) == 0:
                continue
            valid_classes += 1
            source_class_features = source_features[source_class_idx]
            target_class_features = target_features[target_class_idx]
            source_center = torch.mean(source_class_features, dim=0, keepdim=True)
            target_center = torch.mean(target_class_features, dim=0, keepdim=True)
            center_distance = torch.sum((source_center - target_center) ** 2)
            source_intra_distance = torch.mean(torch.sum((source_class_features - source_center) ** 2, dim=1))
            target_intra_distance = torch.mean(torch.sum((target_class_features - target_center) ** 2, dim=1))
            intra_class_loss += center_distance + 0.5 * (source_intra_distance + target_intra_distance)
        if valid_classes > 0:
            intra_class_loss = intra_class_loss / valid_classes

        # 类间分离损失
        inter_class_loss = 0.0
        inter_class_count = 0
        class_centers = []
        for class_idx in range(3):
            class_idx_source = (source_original_labels == class_idx).nonzero(as_tuple=True)[0]
            class_idx_target = (target_original_labels == class_idx).nonzero(as_tuple=True)[0]
            if len(class_idx_source) == 0 and len(class_idx_target) == 0:
                class_centers.append(None)
                continue
            class_features = []
            if len(class_idx_source) > 0:
                class_features.append(source_features[class_idx_source])
            if len(class_idx_target) > 0:
                class_features.append(target_features[class_idx_target])
            if len(class_features) > 0:
                class_features = torch.cat(class_features, dim=0)
                class_center = torch.mean(class_features, dim=0, keepdim=True)
                class_centers.append(class_center)
            else:
                class_centers.append(None)
        for i in range(3):
            for j in range(i + 1, 3):
                if class_centers[i] is not None and class_centers[j] is not None:
                    distance = -torch.sum((class_centers[i] - class_centers[j]) ** 2)
                    inter_class_loss += distance
                    inter_class_count += 1
        if inter_class_count > 0:
            inter_class_loss = inter_class_loss / inter_class_count

        # 对称KL散度
        kl_div_loss = self.symmetric_kl_divergence(source_features, target_features)

        total_loss = (self.intra_weight * intra_class_loss +
                      self.inter_weight * inter_class_loss +
                      0.2 * kl_div_loss)
        return total_loss

    def symmetric_kl_divergence(self, source_features, target_features):
        source_mean = torch.mean(source_features, dim=0)
        target_mean = torch.mean(target_features, dim=0)
        source_var = torch.var(source_features, dim=0)
        target_var = torch.var(target_features, dim=0)
        source_var = torch.clamp(source_var, min=1e-5)
        target_var = torch.clamp(target_var, min=1e-5)
        kl_st = 0.5 * (torch.log(target_var / source_var) +
                       (source_var + (source_mean - target_mean) ** 2) / target_var - 1)
        kl_ts = 0.5 * (torch.log(source_var / target_var) +
                       (target_var + (target_mean - source_mean) ** 2) / source_var - 1)
        return torch.mean(kl_st + kl_ts)

    def convert_to_original_labels(self, hierarchical_labels):
        coarse_labels, fine_labels = hierarchical_labels
        original_labels = torch.zeros_like(coarse_labels)
        original_labels[coarse_labels == 0] = 0
        mid_mask = (coarse_labels == 1) & (fine_labels == 0)
        original_labels[mid_mask] = 1
        high_mask = (coarse_labels == 1) & (fine_labels == 1)
        original_labels[high_mask] = 2
        return original_labels

criterion_domain = nn.BCELoss()

class LabelSmoothingLoss(nn.Module):
    def __init__(self, classes, smoothing=0.1, dim=-1, reduction='mean'):
        super(LabelSmoothingLoss, self).__init__()
        self.classes = classes
        self.dim = dim
        self.reduction = reduction
        if isinstance(smoothing, (list, tuple)):
            assert len(smoothing) == classes, "Smoothing list length must match number of classes"
            self.smoothing = torch.tensor(smoothing)
            self.confidence = 1.0 - self.smoothing
        else:
            self.smoothing = smoothing
            self.confidence = 1.0 - smoothing

    def forward(self, pred, target, pseudo=False):
        pred = F.log_softmax(pred, dim=self.dim)
        smoothing = self.smoothing
        confidence = self.confidence
        if pseudo:
            if isinstance(smoothing, torch.Tensor):
                smoothing = smoothing * 1.5
                confidence = 1.0 - smoothing
            else:
                smoothing = min(smoothing * 1.5, 0.3)
                confidence = 1.0 - smoothing

        with torch.no_grad():
            true_dist = torch.zeros_like(pred)
            if isinstance(smoothing, torch.Tensor):
                smoothing = smoothing.to(pred.device)
                confidence = confidence.to(pred.device)
                for cls in range(self.classes):
                    true_dist[:, cls] = smoothing[cls] / (self.classes - 1)
                true_dist.scatter_(1, target.unsqueeze(1), confidence[target])
            else:
                true_dist.fill_(smoothing / (self.classes - 1))
                true_dist.scatter_(1, target.unsqueeze(1), confidence)

        loss = -torch.sum(true_dist * pred, dim=self.dim)
        if self.reduction == 'mean':
            return loss.mean()
        elif self.reduction == 'sum':
            return loss.sum()
        else:
            raise ValueError(f"Unsupported reduction: {self.reduction}")