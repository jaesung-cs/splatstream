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
    torch.cuda.synchronize()
    start_time = time.time()
    images = []
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
        images_chunk = (images_chunk.clip(0.0, 1.0) * 255.0).to(torch.uint8).cpu()
        images.append(images_chunk)
    torch.cuda.synchronize()
    end_time = time.time()
    rendering_time = end_time - start_time
    print("draw end")

    colors = torch.cat(images).numpy()
    return {
        "chunk_size": chunk_size,
        "colors": colors,
        "total_time": rendering_time,
    }
