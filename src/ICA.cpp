#include "ICA.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <QDebug>
#include <stdexcept>

ICA::ICA(int n_components, int random_state)
    : n_components_(n_components), random_state_(random_state) {
}

void ICA::fit(const Eigen::MatrixXd& X) {
    // X: [channels x samples]
    const int n_channels = static_cast<int>(X.rows());
    const int n_samples = static_cast<int>(X.cols());
    if (n_components_ <= 0 || n_components_ > n_channels) {
        throw std::runtime_error("Invalid n_components for ICA.");
    }
    if (n_samples < 2) {
        throw std::runtime_error("Not enough samples to run ICA.");
    }

    // 1) ȥ��ֵ
    mean_ = X.rowwise().mean(); // [channels]
    Eigen::MatrixXd Xc = X.colwise() - mean_; // [channels x samples]

    // 2) �׻�
    // Э����: [channels x channels]
    Eigen::MatrixXd cov = (Xc * Xc.transpose()) / double(n_samples);
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(cov);
    if (es.info() != Eigen::Success) {
        throw std::runtime_error("Eigen decomposition failed in whitening.");
    }
    // ����ֵ������
    Eigen::VectorXd evals = es.eigenvalues();
    Eigen::MatrixXd evecs = es.eigenvectors();

    // ��ȡ���� n_components_
    // ������ 2 ͨ������ȼ���ȫȡ���߼�дȫ��ͨ�û���
    std::vector<int> idx(evals.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int a, int b) { return evals(a) > evals(b); });

    Eigen::MatrixXd E = Eigen::MatrixXd(evecs.rows(), n_components_);
    Eigen::VectorXd D = Eigen::VectorXd(n_components_);
    for (int i = 0; i < n_components_; ++i) {
        E.col(i) = evecs.col(idx[i]);
        D(i) = std::max(evals(idx[i]), 1e-12); // ��ֹ����
    }

    // whitening = D^{-1/2} * E^T
    whitening_ = D.cwiseInverse().cwiseSqrt().asDiagonal() * E.transpose(); // [n_components x channels]
    dewhitening_ = E * D.cwiseSqrt().asDiagonal();                             // [channels x n_components]

    Eigen::MatrixXd X_white = whitening_ * Xc;  // [n_components x samples]

    // 3) FastICA���Գƣ�
    fastica_symmetric(X_white, W_); // W_: [n_components x n_components]

    // 4) ���/��죨�ص�ԭʼ�ռ䣩
    mixing_full_ = dewhitening_ * W_.inverse(); // [channels x n_components]
    unmixing_full_ = W_ * whitening_;             // [n_components x channels]
}

Eigen::MatrixXd ICA::transform(const Eigen::MatrixXd& X) const {
    if (whitening_.size() == 0 || W_.size() == 0) {
        throw std::runtime_error("ICA not fitted before transform().");
    }
    Eigen::MatrixXd Xc = X.colwise() - mean_;
    Eigen::MatrixXd Xw = whitening_ * Xc; // [n_components x samples]
    return W_ * Xw;                       // S: [n_components x samples]
}

void ICA::set_exclude(const std::vector<int>& indices) {
    exclude_ = indices;
}

Eigen::MatrixXd ICA::apply(const Eigen::MatrixXd& X) const {
    if (mixing_full_.size() == 0 || W_.size() == 0) {
        throw std::runtime_error("ICA not fitted before apply().");
    }
    // S
    Eigen::MatrixXd S = transform(X); // [n_components x samples]

    // �����Ҫ�޳��ĳɷ�
    for (int idx : exclude_) {
        if (0 <= idx && idx < S.rows()) {
            S.row(idx).setZero();
        }
    }

    // �ؽ���Xc_clean = mixing_full * S ; X_clean = Xc_clean + mean
    Eigen::MatrixXd Xc_clean = mixing_full_ * S;              // [channels x samples]
    Eigen::MatrixXd X_clean = Xc_clean.colwise() + mean_;    // [channels x samples]
    return X_clean;
}

std::pair<std::vector<int>, std::vector<double>>
ICA::find_bads_eog(const Eigen::MatrixXd& X,
    const Eigen::MatrixXd& eog_ref,
    double corr_threshold) const {
    // ��������ɷ�
    Eigen::MatrixXd S = transform(X); // [n_components x samples]

    const int m = static_cast<int>(S.rows());
    const int samples = static_cast<int>(S.cols());

    if (eog_ref.cols() != samples) {
        throw std::runtime_error("eog_ref samples mismatch.");
    }

    std::vector<int> bad_indices;
    std::vector<double> scores(m, 0.0);

    // ��ÿ���ɷ֣����������вο�ͨ������أ�ȡ������ֵ
    for (int i = 0; i < m; ++i) {
        double best = 0.0;
        Eigen::VectorXd si = S.row(i).transpose(); // [samples]
        for (int r = 0; r < eog_ref.rows(); ++r) {
            Eigen::VectorXd ref = eog_ref.row(r).transpose();
            best = std::max(best, abs_pearson_corr(si, ref));
        }
        scores[i] = best;
        if (best >= corr_threshold) {
            bad_indices.push_back(i);
        }
    }
    return { bad_indices, scores };
}

void ICA::fastica_symmetric(const Eigen::MatrixXd& X_white, Eigen::MatrixXd& W) const {
    // �Գ� FastICA��W ��ʼ��Ϊ�����������
    const int m = static_cast<int>(X_white.rows());
    const int n = static_cast<int>(X_white.cols());

    std::mt19937 gen(static_cast<unsigned>(random_state_));
    std::normal_distribution<double> dist(0.0, 1.0);

    Eigen::MatrixXd W0(m, m);
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < m; ++j)
            W0(i, j) = dist(gen);

    symmetric_decorrelate(W0);

    const int max_iter = 500;
    const double tol = 1e-6;

    W = W0;
    for (int it = 0; it < max_iter; ++it) {
        // WX
        Eigen::MatrixXd WX = W * X_white; // [m x n]

        // ������ g(u)=tanh(u), g'(u)=1 - tanh(u)^2
        Eigen::MatrixXd G = WX.array().tanh().matrix(); // [m x n]
        Eigen::MatrixXd Gp = (1.0 - WX.array().tanh().square()).matrix(); // [m x n]

        // W_new = (G * X^T)/n - diag(mean(g')) * W
        Eigen::VectorXd meanGp = Gp.rowwise().mean(); // [m]
        Eigen::MatrixXd W_new = (G * X_white.transpose()) / double(n)
            - meanGp.asDiagonal() * W;

        symmetric_decorrelate(W_new);

        // �����оݣ�max | abs(diag(W_new * W^T)) - 1 |
        Eigen::MatrixXd M = W_new * W.transpose();
        double lim = 0.0;
        for (int i = 0; i < m; ++i) {
            lim = std::max(lim, std::abs(std::abs(M(i, i)) - 1.0));
        }
        W = W_new;
        if (lim < tol) break;
    }
}

void ICA::symmetric_decorrelate(Eigen::MatrixXd& W) {
    // �� B = W W^T, �� B^{-1/2} = E D^{-1/2} E^T���� W <- (B^{-1/2}) W
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(W * W.transpose());
    Eigen::VectorXd d = es.eigenvalues();
    Eigen::MatrixXd E = es.eigenvectors();
    // ��ֹ��ֵ����
    for (int i = 0; i < d.size(); ++i) d(i) = std::max(d(i), 1e-12);
    Eigen::MatrixXd Dm12 = d.cwiseInverse().cwiseSqrt().asDiagonal();
    W = (E * Dm12 * E.transpose()) * W;
}

double ICA::abs_pearson_corr(const Eigen::VectorXd& x, const Eigen::VectorXd& y) {
    const int n = static_cast<int>(x.size());
    if (n != y.size() || n < 2) return 0.0;

    const double mean_x = x.mean();
    const double mean_y = y.mean();
    double num = 0.0, denx = 0.0, deny = 0.0;

    for (int i = 0; i < n; ++i) {
        const double dx = x(i) - mean_x;
        const double dy = y(i) - mean_y;
        num += dx * dy;
        denx += dx * dx;
        deny += dy * dy;
    }
    if (denx <= 0 || deny <= 0) return 0.0;
    return std::abs(num / std::sqrt(denx * deny));
}
