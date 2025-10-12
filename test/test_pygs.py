from PIL import Image
import pygs

if __name__ == "__main__":
    splats = pygs.load_from_ply("models/bonsai_30000.ply")
    image = pygs.draw(splats).numpy()

    print(image.shape, image.dtype)
    Image.fromarray(image[::-1], "RGBA").save("test.png")
