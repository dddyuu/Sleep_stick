
import numpy as np
from pytsk.gradient_descent import AntecedentGMF
from utils import TSKFS


#pkl转化onnx
import joblib

from skl2onnx.common.data_types import FloatTensorType
from onnxmltools.utils import save_model
import torch
from torch import nn



if __name__ == "__main__":
    model_path = 'tmp_class.pkl'
    init_center = np.load("init_center.npy")
    gmf = nn.Sequential(
        AntecedentGMF(in_dim=25, n_rule=30, high_dim=True, init_center=init_center),
        nn.LayerNorm(30),
        nn.ReLU()
    )
    model = TSKFS(in_dim=25, out_dim=2, n_rule=30, antecedent=gmf, order=1, precons=None)
    model.load_state_dict(torch.load(model_path))
    model.eval()
    tensor=torch.rand((1,25))
    temp=model(tensor)
    print(model)
    print(temp)

    output_model_path = 'model.onnx'
    input_names = ["float_input"]
    output_names = ["output_name"]
    torch.onnx.export(
        model,
        tensor,
        output_model_path,
        export_params=True,
        opset_version=12,
        input_names=input_names,
        output_names=output_names
    )
    print(f"Model has been converted and saved to {output_model_path}")