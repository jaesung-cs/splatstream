import argparse
import logging
from pathlib import Path

import numpy as np
import torch
import torch.nn.functional as F

from sharp.models import (
    PredictorParams,
    RGBGaussianPredictor,
    create_predictor,
)
from sharp.utils import camera, io
from sharp.utils import logging as logging_utils
from sharp.utils.gaussians import (
    Gaussians3D,
    SceneMetaData,
    unproject_gaussians,
)

import splatstream as ss

LOGGER = logging.getLogger(__name__)


def visualize(
    input_path: Path,
    checkpoint_path: Path,
    device: str,
    verbose: bool,
):
    """Predict Gaussians from input images."""
    logging_utils.configure(logging.DEBUG if verbose else logging.INFO)

    extensions = io.get_supported_image_extensions()

    image_paths = []
    if input_path.is_file():
        if input_path.suffix in extensions:
            image_paths = [input_path]
    else:
        for ext in extensions:
            image_paths.extend(list(input_path.glob(f"**/*{ext}")))

    if len(image_paths) == 0:
        LOGGER.info("No valid images found. Input was %s.", input_path)
        return

    LOGGER.info("Processing %d valid image files.", len(image_paths))

    if device == "default":
        if torch.cuda.is_available():
            device = "cuda"
        elif torch.mps.is_available():
            device = "mps"
        else:
            device = "cpu"
    LOGGER.info("Using device %s", device)

    # Load or download checkpoint
    if checkpoint_path is None:
        LOGGER.info(
            "No checkpoint provided. Downloading default model from %s",
            DEFAULT_MODEL_URL,
        )
        state_dict = torch.hub.load_state_dict_from_url(
            DEFAULT_MODEL_URL, progress=True
        )
    else:
        LOGGER.info("Loading checkpoint from %s", checkpoint_path)
        state_dict = torch.load(checkpoint_path, weights_only=True)

    gaussian_predictor = create_predictor(PredictorParams())
    gaussian_predictor.load_state_dict(state_dict)
    gaussian_predictor.eval()
    gaussian_predictor.to(device)

    for image_path in image_paths:
        LOGGER.info("Processing %s", image_path)
        image, _, f_px = io.load_rgb(image_path)
        height, width = image.shape[:2]
        intrinsics = torch.tensor(
            [
                [f_px, 0, (width - 1) / 2.0, 0],
                [0, f_px, (height - 1) / 2.0, 0],
                [0, 0, 1, 0],
                [0, 0, 0, 1],
            ],
            device=device,
            dtype=torch.float32,
        )
        gaussians = predict_image(gaussian_predictor, image, f_px, torch.device(device))

        metadata = SceneMetaData(intrinsics[0, 0].item(), (width, height), "linearRGB")
        visualize_gaussians(gaussians, metadata, device=device)


@torch.no_grad()
def predict_image(
    predictor: RGBGaussianPredictor,
    image: np.ndarray,
    f_px: float,
    device: torch.device,
) -> Gaussians3D:
    """Predict Gaussians from an image."""
    internal_shape = (1536, 1536)

    LOGGER.info("Running preprocessing.")
    image_pt = (
        torch.from_numpy(image.copy()).float().to(device).permute(2, 0, 1) / 255.0
    )
    _, height, width = image_pt.shape
    disparity_factor = torch.tensor([f_px / width]).float().to(device)

    image_resized_pt = F.interpolate(
        image_pt[None],
        size=(internal_shape[1], internal_shape[0]),
        mode="bilinear",
        align_corners=True,
    )

    # Predict Gaussians in the NDC space.
    LOGGER.info("Running inference.")
    gaussians_ndc = predictor(image_resized_pt, disparity_factor)

    LOGGER.info("Running postprocessing.")
    intrinsics = (
        torch.tensor(
            [
                [f_px, 0, width / 2, 0],
                [0, f_px, height / 2, 0],
                [0, 0, 1, 0],
                [0, 0, 0, 1],
            ]
        )
        .float()
        .to(device)
    )
    intrinsics_resized = intrinsics.clone()
    intrinsics_resized[0] *= internal_shape[0] / width
    intrinsics_resized[1] *= internal_shape[1] / height

    # Convert Gaussians to metrics space.
    gaussians = unproject_gaussians(
        gaussians_ndc, torch.eye(4).to(device), intrinsics_resized, internal_shape
    )

    return gaussians


def visualize_gaussians(
    gaussians: Gaussians3D,
    metadata: SceneMetaData,
    params: camera.TrajectoryParams | None = None,
    device: torch.device = torch.device("cuda"),
) -> None:
    """Render a single gaussian checkpoint file."""
    (width, height) = metadata.resolution_px
    f_px = metadata.focal_length_px

    if params is None:
        params = camera.TrajectoryParams()

    intrinsics = torch.tensor(
        [
            [f_px, 0, (width - 1) / 2.0, 0],
            [0, f_px, (height - 1) / 2.0, 0],
            [0, 0, 1, 0],
            [0, 0, 0, 1],
        ],
        device=device,
        dtype=torch.float32,
    )
    camera_model = camera.create_camera_model(
        gaussians, intrinsics, resolution_px=metadata.resolution_px
    )

    trajectory = camera.create_eye_trajectory(
        gaussians, params, resolution_px=metadata.resolution_px, f_px=f_px
    )

    colors = gaussians.colors.cpu().numpy()[0, :, None, :]

    # TODO: handle pre-computed colors in viewer. Convert to 0-degree SH for now.
    C0 = 0.28209479177387814
    colors = (colors - 0.5) / C0

    splats = ss.gaussian_splats(
        gaussians.mean_vectors.cpu().numpy()[0],
        gaussians.quaternions.cpu().numpy()[0],
        gaussians.singular_values.cpu().numpy()[0],
        gaussians.opacities.cpu().numpy()[0],
        colors,
    )

    intrinsics = []
    extrinsics = []
    for _, eye_position in enumerate(trajectory):
        camera_info = camera_model.compute(eye_position)
        width = camera_info.width
        height = camera_info.height
        intrinsics.append(camera_info.intrinsics[None].cpu().numpy()[0])
        extrinsics.append(camera_info.extrinsics[None].cpu().numpy()[0])

    intrinsics = np.stack(intrinsics)[..., :3, :3]
    extrinsics = np.stack(extrinsics)

    ss.show(
        splats,
        extrinsics,
        intrinsics,
        width,
        height,
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--input_path", type=Path, required=True)
    parser.add_argument("--checkpoint_path", type=Path, default=Path("sharp_2572gikvuh.pt"))
    parser.add_argument("--device", type=str, default="cuda")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    visualize(
        input_path=args.input_path,
        checkpoint_path=args.checkpoint_path,
        device=args.device,
        verbose=args.verbose,
    )
