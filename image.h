#ifndef __IMAGE_H
#define __IMAGE_H	1

#include <stdio.h>

#include "utils.h"

#define COLOR_RANGE				3
#define KERNEL_SIZE				3
#define TYPE_FROM_CHR(chr)		((image_type_t)((((chr) - '1') % 3)))
#define PIXEL_MAX_VALUE			255

#ifdef __cplusplus
extern "C" {
#endif

typedef enum image_type_t {
	PBM,								/* black and white	*/
	PGM,								/* greyscale		*/
	PPM									/* colour			*/
} image_type_t;

typedef union pixel_t {
	unsigned char val;					/* for non-colour	*/
	unsigned char rgb[COLOR_RANGE];		/* RGB values		*/
} pixel_t;

typedef struct image_selection_t {
	unsigned long uprow;				/* first row		*/
	unsigned long dwrow;				/* last row			*/
	unsigned long lcol;					/* leftmost column	*/
	unsigned long rcol;					/* rightmost column */
} image_selection_t;

typedef struct image_t {
	/* Dimensions */
	unsigned long rows;
	unsigned long columns;

	/* The pixel matrix */
	pixel_t **pixels;

	/* Other useful information */
	image_type_t type;
	image_selection_t selection;
} image_t;

/**
 * Image and pixels creation
*/
pixel_t **create_pixels(size_t rows, size_t columns);
image_t *create_image(size_t rows, size_t columns, image_type_t type);
void free_image(image_t *image);

/**
 * Read and write for pixel matrices
*/
void read_pixels(FILE *in_file, image_t *image, int binary);
void print_pixels(FILE *out_file, image_t *image, int binary);

/**
 * Selection operations
*/
void update_selection(image_selection_t *selection,
	size_t up_row, size_t dw_row,
	size_t l_col, size_t r_col);
void crop_image(image_t *image, image_selection_t selection);

/**
 * Image histogram
*/
void print_histogram(image_t *image, size_t max_stars, size_t bins);

/**
 * Applies an effect using a given image kernel and division factor
*/
void apply_effect(image_t *image, image_selection_t selection,
	DEF_KERNEL(kernel), double divide);

/**
 * Rotations
*/
void rotate_selection(image_t *image, image_selection_t selection, int angle);
void rotate_image(image_t *image, int angle);

#ifdef __cplusplus
}
#endif

#endif
