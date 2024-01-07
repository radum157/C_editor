#include <stdlib.h>
#include <string.h>

#include "image.h"

#define FULL_ROTATION 				360
#define CYCLE_ROTATION				90

/**
 * Initial / Full selection
*/
static inline void init_selection(image_selection_t *selection,
	size_t rows, size_t columns)
{
	selection->uprow = 0;
	selection->lcol = 0;
	selection->dwrow = rows;
	selection->rcol = columns;
}

inline void update_selection(image_selection_t *selection,
	size_t up_row, size_t dw_row,
	size_t l_col, size_t r_col)
{
	selection->uprow = up_row;
	selection->dwrow = dw_row;
	selection->lcol = l_col;
	selection->rcol = r_col;
}

pixel_t **create_pixels(size_t rows, size_t columns)
{
	pixel_t **pixels = (pixel_t **)malloc(rows * sizeof(pixel_t *));
	DIE(!pixels, "malloc failed");

	for (size_t i = 0; i < rows; i++) {
		pixels[i] = (pixel_t *)calloc(columns, sizeof(pixel_t));
		DIE(!pixels[i], "calloc failed");
	}

	return pixels;
}

image_t *create_image(size_t rows, size_t columns, image_type_t type)
{
	image_t *image = (image_t *)malloc(sizeof(image_t));
	DIE(!image, "malloc failed");

	image->rows = rows;
	image->columns = columns;
	image->type = type;

	init_selection(&image->selection, rows, columns);
	image->pixels = create_pixels(rows, columns);

	return image;
}

void free_pixels(pixel_t **pixels, size_t size)
{
	if (!pixels)
		return;

	for (size_t i = 0; i < size; i++)
		free(pixels[i]);

	free(pixels);
	pixels = NULL;
}

void free_image(image_t *image)
{
	if (!image)
		return;

	free_pixels(image->pixels, image->rows);
	free(image);
}

void read_pixels(FILE *in_file, image_t *image, int binary)
{
	for (size_t i = 0; i < image->rows; i++)
		for (size_t j = 0; j < image->columns; j++) {
			if (image->type == PPM) {
				if (!binary)
					fscanf(in_file, "%hhu %hhu %hhu",
						&image->pixels[i][j].rgb[0],
						&image->pixels[i][j].rgb[1],
						&image->pixels[i][j].rgb[2]);
				else
					fread(image->pixels[i][j].rgb, sizeof(unsigned char),
						COLOR_RANGE, in_file);
			} else {
				if (!binary)
					fscanf(in_file, "%hhu", &image->pixels[i][j].val);
				else
					fread(&image->pixels[i][j].val, sizeof(unsigned char),
						1, in_file);
			}
		}
}

void print_pixels(FILE *out_file, image_t *image, int binary)
{
	/* Magic word is Px, where x is the image type (with added compensation) */
	fprintf(out_file, "P%d\n%lu %lu\n%d\n",
		((int)image->type + ((binary) ? 4 : 1)),
		image->columns, image->rows,
		PIXEL_MAX_VALUE);

	for (size_t i = 0; i < image->rows; i++) {
		for (size_t j = 0; j < image->columns; j++) {
			if (image->type == PPM) {
				if (binary)
					fwrite(image->pixels[i][j].rgb, sizeof(unsigned char),
						COLOR_RANGE, out_file);
				else
					fprintf(out_file, "%hhu %hhu %hhu ",
						image->pixels[i][j].rgb[0],
						image->pixels[i][j].rgb[1],
						image->pixels[i][j].rgb[2]);
			} else {
				if (binary)
					fwrite(&image->pixels[i][j].val, sizeof(unsigned char),
						1, out_file);
				else
					fprintf(out_file, "%hhu ", image->pixels[i][j].val);
			}
		}

		if (!binary)
			fprintf(out_file, "\n");
	}
}

void crop_image(image_t *image, image_selection_t selection)
{
	size_t rows = selection.dwrow - selection.uprow;
	size_t columns = selection.rcol - selection.lcol;

	pixel_t **pixels = create_pixels(rows, columns);

	for (size_t i = selection.uprow; i < selection.dwrow; i++)
		for (size_t j = selection.lcol; j < selection.rcol; j++)
			pixels[i - selection.uprow][j - selection.lcol] =
			image->pixels[i][j];

	free_pixels(image->pixels, image->rows);
	image->pixels = pixels;

	image->rows = rows;
	image->columns = columns;
	init_selection(&image->selection, rows, columns);
}

void print_histogram(image_t *image, size_t max_stars, size_t bins)
{
	int fq[PIXEL_MAX_VALUE + 1] = { 0 };

	for (size_t i = 0; i < image->rows; i++)
		for (size_t j = 0; j < image->columns; j++)
			fq[(int)image->pixels[i][j].val]++;

	int *hgram = (int *)calloc(bins, sizeof(int));
	DIE(!hgram, "calloc failed");

	int max_freq = 0;
	/* Interval size */
	size_t size = ARRAY_SIZE(fq) / bins;

	/* Split into intervals */
	for (size_t i = 0; i < bins; i++) {
		for (size_t j = 0; j < size; j++) {
			/* Interval j starts at i * size */
			hgram[i] += fq[j + i * size];
		}

		if (hgram[i] > max_freq)
			max_freq = hgram[i];
	}

	for (size_t i = 0; i < bins; i++) {
		int stars = (hgram[i] * max_stars) / max_freq;

		printf("%d\t|\t", stars);
		for (int j = 0; j < stars; j++)
			printf("*");
		printf("\n");
	}
}

/**
 * @return The resulted pixel after applying an effect on a given position
*/
pixel_t _apply_on_pixel(pixel_t **pixels, DEF_KERNEL(kernel), double divide,
	size_t row, size_t col, image_type_t type)
{
	double buffer[COLOR_RANGE] = { 0 };

	/* Compute sum of neighbours */
	for (size_t i = 0; i < KERNEL_SIZE; i++)
		for (size_t j = 0; j < KERNEL_SIZE; j++) {
			if (type == PPM) {
				buffer[0] +=
					kernel[i][j] * pixels[row - 1 + i][col - 1 + j].rgb[0];
				buffer[1] +=
					kernel[i][j] * pixels[row - 1 + i][col - 1 + j].rgb[1];
				buffer[2] +=
					kernel[i][j] * pixels[row - 1 + i][col - 1 + j].rgb[2];
			} else {
				buffer[0] +=
					kernel[i][j] * pixels[row - 1 + i][col - 1 + j].val;
			}
		}

	/* Copy into result */
	pixel_t result;
	if (type == PPM) {
		result.rgb[0] = _clamp(round(buffer[0] / divide), 0, PIXEL_MAX_VALUE);
		result.rgb[1] = _clamp(round(buffer[1] / divide), 0, PIXEL_MAX_VALUE);
		result.rgb[2] = _clamp(round(buffer[2] / divide), 0, PIXEL_MAX_VALUE);
	} else {
		result.val = _clamp(round(buffer[0] / divide), 0, PIXEL_MAX_VALUE);
	}

	return result;
}

void apply_effect(image_t *image, image_selection_t selection,
	DEF_KERNEL(kernel), double divide)
{
	size_t rows = selection.dwrow - selection.uprow;
	size_t columns = selection.rcol - selection.lcol;

	/* Cannot be done in-place */
	pixel_t **result = create_pixels(rows, columns);

	for (size_t i = selection.uprow; i < selection.dwrow; i++) {
		for (size_t j = selection.lcol; j < selection.rcol; j++) {
			/* Leave edges */
			if (i == 0 || i >= image->rows - 1 ||
				j == 0 || j >= image->columns - 1) {
				result[i - selection.uprow][j - selection.lcol] =
					image->pixels[i][j];
				continue;
			}

			result[i - selection.uprow][j - selection.lcol] =
				_apply_on_pixel(image->pixels, kernel, divide,
					i, j, image->type);
		}
	}

	for (size_t i = selection.uprow; i < selection.dwrow; i++)
		for (size_t j = selection.lcol; j < selection.rcol; j++)
			image->pixels[i][j] =
			result[i - selection.uprow][j - selection.lcol];

	free_pixels(result, rows);
}

/**
 * Performs one rotation to the righ (90 degrees) on the @a selection
*/
void _rotate_right(image_t *image, image_selection_t selection)
{
	size_t start = selection.uprow;
	size_t end = selection.dwrow;
	size_t middle = (start + end) / 2;

	/* Sacrifice time for space for large images. For small images, irrelevant. */
	for (size_t i = start; i < middle; i++) {
		for (size_t j = i; j < end - i + start; j++)
			SWAP_ANY(image->pixels[j][i], image->pixels[i][end - j + start - 1],
				pixel_t);

		for (size_t j = i + 1; j < end - i + start; j++)
			SWAP_ANY(image->pixels[end - i + start - 1][j], image->pixels[j][i],
				pixel_t);

		for (size_t j = i + 1; j < end - i + start - 1; j++)
			SWAP_ANY(image->pixels[j][end - i + start - 1],
				image->pixels[end - i + start - 1][end - j + start - 1],
				pixel_t);
	}
}

void rotate_selection(image_t *image, image_selection_t selection, int angle)
{
	if (angle % FULL_ROTATION == 0)
		return;

	int rotations = (angle < 0)
		? FULL_ROTATION + angle % FULL_ROTATION
		: angle % FULL_ROTATION;
	rotations /= CYCLE_ROTATION;

	for (int i = 0; i < rotations; i++)
		_rotate_right(image, selection);
}

void rotate_image(image_t *image, int angle)
{
	if (angle % FULL_ROTATION == 0)
		return;

	int rotations = (angle < 0)
		? FULL_ROTATION + angle % FULL_ROTATION
		: angle % FULL_ROTATION;
	rotations /= CYCLE_ROTATION;

	size_t rows = (rotations % 2) ? image->columns : image->rows;
	size_t columns = (rotations % 2) ? image->rows : image->columns;

	/* Sacrifice space for time out of convenience */
	pixel_t **result = create_pixels(rows, columns);

	for (size_t i = 0; i < rows; i++) {
		for (size_t j = 0; j < columns; j++) {
			switch (rotations) {
			case 1:
				result[i][j] = image->pixels[image->rows - 1 - j][i];
				break;

			case 2:
				result[i][j] =
					image->pixels[image->rows - 1 - i][image->columns - 1 - j];
				break;

			case 3:
				result[i][j] = image->pixels[j][image->columns - 1 - i];
				break;

			default:
				DIE(1, "unexpected case");
			}
		}
	}

	free_pixels(image->pixels, image->rows);

	image->pixels = result;
	image->rows = rows;
	image->columns = columns;

	init_selection(&image->selection, image->rows, image->columns);
}
