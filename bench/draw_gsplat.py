import time

import numpy as np
import torch
import gsplat


def draw_gsplat(ply_data, draw_data, chunk_size: int | None = None):
    device = torch.device("cuda")

    # To GPU
    means = torch.from_numpy(ply_data["means"].astype(np.float32)).to(device)
    quats = torch.from_numpy(ply_data["quats"].astype(np.float32)).to(device)
    scales = torch.from_numpy(ply_data["scales"].astype(np.float32)).to(device)
    opacities = torch.from_numpy(ply_data["opacities"].astype(np.float32)).to(device)
    colors = torch.from_numpy(ply_data["colors"].astype(np.float32)).to(device)
    viewmats = torch.from_numpy(draw_data["viewmats"].astype(np.float32)).to(device)
    Ks = torch.from_numpy(draw_data["Ks"].astype(np.float32)).to(device)

    if chunk_size is None:
        chunk_size = len(viewmats)

    print("warming up...")
    for i in range(3):
        viewmats_chunk = viewmats[:chunk_size]
        Ks_chunk = Ks[:chunk_size]
        images_chunk, alphas, meta = gsplat.rasterization(
            means=means,
            quats=quats,
            scales=scales,
            opacities=opacities,
            colors=colors,
            viewmats=viewmats_chunk,
            Ks=Ks_chunk,
            width=draw_data["width"],
            height=draw_data["height"],
            sh_degree=3,
            near_plane=0.1,
            far_plane=1e3,
            rasterize_mode="antialiased",
        )
    print("warming up done")

    print("drawing...")

    start = torch.cuda.Event(enable_timing=True)
    end = torch.cuda.Event(enable_timing=True)

    images = []

    torch.cuda.synchronize()
    with torch.inference_mode():
        start.record()
        for i in range(0, len(viewmats), chunk_size):
            viewmats_chunk = viewmats[i : i + chunk_size]
            Ks_chunk = Ks[i : i + chunk_size]
            images_chunk, alphas, meta = gsplat.rasterization(
                means=means,
                quats=quats,
                scales=scales,
                opacities=opacities,
                colors=colors,
                viewmats=viewmats_chunk,
                Ks=Ks_chunk,
                width=draw_data["width"],
                height=draw_data["height"],
                sh_degree=3,
                near_plane=0.1,
                far_plane=1e3,
                rasterize_mode="antialiased",
            )
            images.append(images_chunk)
        end.record()
    torch.cuda.synchronize()
    rendering_time = start.elapsed_time(end) / 1000.0
    print("draw end")

    colors = (torch.cat(images).clip(0.0, 1.0) * 255.0).to(torch.uint8).cpu().numpy()
    return {
        "chunk_size": chunk_size,
        "colors": colors,
        "total_time": rendering_time,
    }
