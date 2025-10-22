# 生成高斯核列表,不用numpy
import math
import os
import sys


def gaussian(x, sigma):
    return math.exp(-(x**2) / (2 * sigma**2)) / (math.sqrt(2 * math.pi) * sigma)


def generate_gaussian_kernel(size, sigma):
    kernel = []
    for i in range(-size, size + 1):
        for j in range(-size, size + 1):
            kernel.append(gaussian(math.sqrt(i**2 + j**2), sigma))
    return kernel


def save_kernel_to_file(kernel, filename):
    with open(filename, "w") as f:
        f.write(f"float kernel[{(2 * RADIUS + 1) ** 2}] = " + "{")  # 保留6位小数
        for value in kernel:
            f.write(f"{value:.6f}, ")  # 保留6位小数
        f.write("};\n")

RADIUS = 3
SIGMA = 1.0
KERNEL = generate_gaussian_kernel(RADIUS, SIGMA)
FILENAME = "scripts/gaussian_kernel.txt"
save_kernel_to_file(KERNEL, FILENAME)
print(f"高斯核已保存到 {FILENAME}")
