from PIL import Image
from math import *  # pyright: ignore[reportWildcardImportFromLibrary]

radius = 70
size = radius * 2 + 2
output_path = "./textures/density.png"
density_img = Image.new("RGBA", (size, size))
solution = 2048
v = 3.14159 * radius ** 6 / 6
mass = 100

def densityToAlpha(dist, radius):
    a = radius ** 2 - dist ** 2
    return 0.1 * a ** 3 / v


x0 = size // 2
y0 = size // 2
for theta in range(solution):
    t = pi * 2 * theta / solution
    for r0 in range(0, radius):
        x = x0 + r0 * cos(t)
        y = y0 + r0 * sin(t)
        d = 255 * 0.6 * densityToAlpha(r0, radius)
        color = (255, 255, 255, d)
        color = (int(color[0]), int(color[1]), int(color[2]), int(color[3]))
        # color value is multiplied by 0.00001 in shader
        # color = (int(255 * x / size), int(255 * y / size), 255, int(255 * fade(r0, radius)))
        density_img.putpixel((int(x), int(y)), color)

        print(f"Set pixel at ({int(x)}, {int(y)}) to density {color}")


density_img.save(output_path, "PNG")
density_img.show()
print(f"Max density: {densityToAlpha(0, radius) * 255}")
print(f"Saved density texture to {output_path}")
