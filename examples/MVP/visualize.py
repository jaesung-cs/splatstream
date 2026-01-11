import importlib
import os
import torch
from torch.utils.data import DataLoader
from setup import init_config
import torch.nn.functional as F

import splatstream as ss

config = init_config()

os.environ["OMP_NUM_THREADS"] = str(config.inference.get("num_threads", 1))
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# Set up tf32
torch.backends.cuda.matmul.allow_tf32 = config.inference.use_tf32
torch.backends.cudnn.allow_tf32 = config.inference.use_tf32
amp_dtype_mapping = {
    "fp16": torch.float16, 
    "bf16": torch.bfloat16, 
    "fp32": torch.float32, 
    'tf32': torch.float32
}


# Load data
dataset_name = config.inference.get("dataset_name", "data.dataset.Dataset")
module, class_name = dataset_name.rsplit(".", 1)
Dataset = importlib.import_module(module).__dict__[class_name]
dataset = Dataset(config)

dataloader = DataLoader(
    dataset,
    batch_size=config.inference.batch_size_per_gpu,
    shuffle=False,
    #num_workers=config.inference.num_workers,
    #prefetch_factor=config.inference.prefetch_factor,
    #num_workers=1,
    prefetch_factor=None,
    #persistent_workers=True,
    pin_memory=False,
)
dataloader_iter = iter(dataloader)


# Import model and load checkpoint
module, class_name = config.model.class_name.rsplit(".", 1)
LVSM = importlib.import_module(module).__dict__[class_name]
model = LVSM(config).to(device)
msg = model.load_ckpt(config.inference.ckpt_path)
print(msg)

print(f"Running inference; save results to: {config.inference.out_dir}")
import warnings
warnings.filterwarnings('ignore', category=FutureWarning)

model.eval()
cnt = 0
with torch.no_grad(), torch.autocast(
    enabled=config.inference.use_amp,
    device_type="cuda",
    dtype=amp_dtype_mapping[config.inference.amp_dtype],
):
    for batch in dataloader:
        batch = {k: v.to(device) if type(v) == torch.Tensor else v for k, v in batch.items()}
        print(cnt)
        cnt += 1
        input_data_dict = {key: value[:, :config.data.num_input_frames] if type(value) == torch.Tensor else value for key, value in batch.items()}
        target_data_dict = {key: value[:, config.data.num_input_frames:] if type(value) == torch.Tensor else None for key, value in batch.items()}
        """
        result = model(input_data_dict, target_data_dict)
        export_results(result, config.inference.out_dir, 
                       compute_metrics=config.inference.get("compute_metrics"), 
                       uid=cnt)
        """
        gaussians = model.get_gaussians(input_data_dict)
        xyz = gaussians["xyz"]
        feature = gaussians["feature"]
        scale = gaussians["scale"]
        rotation = gaussians["rotation"]
        opacity = gaussians["opacity"]

        print(target_data_dict)

        # opacity has spherical harmonics?
        #opacity = opacity.mean(dim=-2)  # (N, 1)
        opacity = opacity[..., 0]

        # activation
        #opacity = opacity.sigmoid().squeeze(-1)
        scale = scale.exp()
        rotation = F.normalize(rotation, p=2, dim=-1)

        # to numpy
        xyz = xyz.cpu().numpy()
        feature = feature.cpu().numpy()
        scale = scale.cpu().numpy()
        rotation = rotation.cpu().numpy()
        opacity = opacity.cpu().numpy()
        print(xyz.shape, feature.shape, scale.shape, rotation.shape, opacity.shape)
        #print(opacity[:20])

        # target matrices
        test_intr = target_data_dict["fxfycxcy"][0]  # (N, 4)
        test_c2w = target_data_dict["c2w"][0]  # (N, 4, 4)
        
        viewmats = test_c2w.float().inverse()  # (N, 4, 4)
        Ks = torch.zeros(test_c2w.shape[0], 3, 3).to(test_intr.device)
        Ks[:, 0, 0] = test_intr[:, 0]
        Ks[:, 1, 1] = test_intr[:, 1]
        Ks[:, 0, 2] = test_intr[:, 2]
        Ks[:, 1, 2] = test_intr[:, 3]
        Ks[:, 2, 2] = 1  # (N, 3, 3)

        Ks = Ks.cpu().numpy()
        viewmats = viewmats.cpu().numpy()
        width = 960
        height = 540

        splats = ss.gaussian_splats(xyz, rotation, scale, opacity, feature)
        ss.show(splats, viewmats, Ks, width, height)
    torch.cuda.empty_cache()
