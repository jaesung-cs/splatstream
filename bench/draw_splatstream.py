import time

import splatstream as ss


def draw_splatstream(ply_data, draw_data):
    # TODO: measure loading time and rendering time separately
    print("loading splats...")
    start_time = time.time()
    splats = ss.gaussian_splats(
        means=ply_data["means"],
        quats=ply_data["quats"],
        scales=ply_data["scales"],
        opacities=ply_data["opacities"],
        colors=ply_data["colors"],
    )
    print("loading splats done")

    print("drawing...")
    images = ss.draw(
        splats=splats,
        viewmats=draw_data["viewmats"],
        Ks=draw_data["Ks"],
        width=draw_data["width"],
        height=draw_data["height"],
        near=0.1,
        far=1e3,
    ).numpy()
    end_time = time.time()
    rendering_time = end_time - start_time
    print("draw end")

    return {
        "colors": images[..., :3],
        "total_time": rendering_time,
    }
