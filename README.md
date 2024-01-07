##### C Image Editor - Marin Radu
# README

## View as webpage

```
sudo pip3 install grip
grip  README.md
# open http://localhost:6419/
```

# Description

Basic C image editor. Supports `.ppm`, `.pgm` and `.pbm` formats (both text and binary files).

# Operations

All elements inside an image are contained within a **designated interval: [0, 255]**. A union was used to store values, depending on whether or not the image is of ppm type.

Console commands:

1. **LOAD**: Loads an image of the supported format. Essentially reading a matrix from a file, ignoring all lines that start with '#'.

2. **SELECT**: Selects a range within the image for future modifications.

3. **SELECT ALL**: The same as *SELECT*-ing the matrix from `(0, 0)` to `(n, m)`, where 'n' and 'm' are the dimensions of the image.

4. **ROTATE**: Rotates the image at a given angle. The angle must be divisible by *90*.

5. **EQUALIZE**: Performs image equalisation using the histogram. See more [here](https://en.wikipedia.org/wiki/Histogram_equalization).

6. **CROP**: Crops the image down to the selection.

7. **APPLY PARAMETER**: Applies the given parameter. The parameter can be: *EDGE*, *SHARPEN*, *BLUR* or *GAUSSIAN_BLUR*. Uses image `kernels`.

8. **HISTOGRAM**: Prints the image's histogram using `ASCII` characters. The parameters should specify the maximum amount of stars and the number of bins (intervals).

9. **SAVE**: Saves the loaded image inside a given file. Specify `ascii` as a final argument to save as text.

10. **EXIT**: Frees all dynamically allocated memory and exits the program.

First part of most functions is argument parsing from the console input.

Further explanations can be found inside the header and source files (through comments, variable names etc).
