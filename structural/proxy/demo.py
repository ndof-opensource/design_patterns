import numpy as np
import numba as nb

# ------------------------------------------------------------------------------------------------------
# Demonstrating advanced NumPy indexing techniques
# ------------------------------------------------------------------------------------------------------


# Create a 4x5 array
A = np.arange(20).reshape(4, 5)
print("Original array A:\n", A)

# --- Slicing ---
# Select the first 3 rows and last 2 columns
sliced = A[:3, -2:]
print("\nSliced A[:3, -2:]:\n", sliced)

# --- Broadcasting ---
# Add a row vector to all rows of A (broadcasting)
row_vec = np.array([10, 20, 30, 40, 50])
broadcasted = A + row_vec
print("\nBroadcasted A + row_vec:\n", broadcasted)

# --- Fancy indexing ---
# Select rows 0 and 2, and columns 1 and 3
fancy = A[[0, 2], :][:, [1, 3]]
print("\nFancy indexing A[[0, 2], :][:, [1, 3]]:\n", fancy)


# ------------------------------------------------------------------------------------------------------
# Using Numba for JIT compilation
# ------------------------------------------------------------------------------------------------------

import numpy as np
from numba import jit

# JIT-compiled convolution function
@jit(nopython=True)
def convolve2d(image, kernel):
    img_h, img_w = image.shape
    k_h, k_w = kernel.shape
    out_h = img_h - k_h + 1
    out_w = img_w - k_w + 1
    output = np.zeros((out_h, out_w))

    for i in range(out_h):
        for j in range(out_w):
            acc = 0.0
            for m in range(k_h):
                for n in range(k_w):
                    acc += image[i + m, j + n] * kernel[m, n]
            output[i, j] = acc

    return output

# Sample input image (5x5) and kernel (3x3)
image = np.array([
    [1, 2, 3, 0, 1],
    [4, 5, 6, 1, 0],
    [7, 8, 9, 0, 1],
    [1, 3, 5, 7, 9],
    [0, 2, 4, 6, 8]
], dtype=np.float32)

kernel = np.array([
    [1, 0, -1],
    [1, 0, -1],
    [1, 0, -1]
], dtype=np.float32)

# Perform convolution
result = convolve2d(image, kernel)
print("Convolved result:\n", result)


# ------------------------------------------------------------------------------------------------------
# Using NumPy's stride tricks for sliding window view
# ------------------------------------------------------------------------------------------------------
import numpy as np
from numpy.lib.stride_tricks import as_strided

# Create a 1D array
x = np.array([1, 2, 3, 4, 5, 6])

# Set window size
window_size = 3

# Create a sliding window view using strides
def sliding_window_view(arr, window_size):
    n = arr.shape[0]
    stride = arr.strides[0]
    shape = (n - window_size + 1, window_size)
    strides = (stride, stride)
    return as_strided(arr, shape=shape, strides=strides)

# Use the function
windows = sliding_window_view(x, window_size)

print("Original array:\n", x)
print("Sliding windows:\n", windows)

