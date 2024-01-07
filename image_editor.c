#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "image.h"
#include "utils.h"

#define BUFSIZ					8192

/**
 * Reads an image from a file
*/
void read_image(image_t **image, char command_line[BUFSIZ])
{
	char args[2][BUFSIZ];
	if (sscanf(command_line, "%s%s", args[0], args[1]) != 2) {
		printf("%s\n", "Invalid command");
		return;
	}

	free_image(*image);
	*image = NULL;

	FILE *in_file = fopen(args[1], "rb");
	if (!in_file) {
		printf("Failed to load %s\n", args[1]);
		return;
	}

	/**
	 * Read format:
	 * - magic word
	 * - pixel max value
	 * - image dimensions
	 * - pixels
	 * - comments anywhere before the pixel matrix
	 *
	*/
	char magic_word[4];
	fgets(magic_word, 4, in_file);

	char line[BUFSIZ];
	while (fgets(line, BUFSIZ, in_file)) {
		if (line[0] == '#')
			continue;
		if (strncmp(line, "255", 3) == 0)
			break;

		int rows, columns;
		sscanf(line, "%d%d", &columns, &rows);

		*image = create_image(rows, columns, TYPE_FROM_CHR(magic_word[1]));
	}

	read_pixels(in_file, *image, magic_word[1] > '3');
	fclose(in_file);

	printf("Loaded %s\n", args[1]);
}

/**
 * Saves an image to a file
*/
void save_image(char command_line[BUFSIZ], image_t *image)
{
	char args[3][BUFSIZ];
	int read = sscanf(command_line, "%s%s%s",
		args[0], args[1], args[2]);
	if (read < 2 || read > 3) {
		puts("Invalid command");
		return;
	}

	int binary = (read == 2);
	FILE *out_file = fopen(args[1], (binary) ? "wb" : "wt");
	DIE(!out_file, "fopen failed");

	print_pixels(out_file, image, binary);
	fclose(out_file);

	printf("Saved %s\n", args[1]);
}

/**
 * @return If a given coordinate is valid
*/
static inline int _valid_coordinate(long coord,
	long lower_limit, long upper_limit)
{
	return (coord >= lower_limit && coord <= upper_limit);
}

/**
 * Selects a given range
*/
void select_range(image_t *image, char command_line[BUFSIZ])
{
	char args[2][BUFSIZ];
	int ret = sscanf(command_line, "%s%s", args[0], args[1]);
	if (ret <= 1) {
		puts("Invalid command");
		return;
	}

	if (strcmp(args[1], "ALL") == 0) {
		update_selection(&image->selection, 0, image->rows, 0, image->columns);
		puts("Selected ALL");
		return;
	}

	int coord[4];
	if (sscanf(command_line, "%s%d%d%d%d",
		args[0], &coord[0], &coord[2], &coord[1], &coord[3]) != 5) {
		puts("Invalid command");
		return;
	}

	/* Validate and put coordinates in place */
	if (coord[0] == coord[1] || coord[2] == coord[3]) {
		puts("Invalid set of coordinates");
		return;
	}

	size_t size = ARRAY_SIZE(coord) / 2;
	for (size_t i = 0; i < size; i++)
		if (!_valid_coordinate(coord[i], 0, image->columns) ||
			!_valid_coordinate(coord[i + size], 0, image->rows)) {
			puts("Invalid set of coordinates");
			return;
		}

	if (coord[0] > coord[1])
		SWAP_NUMERIC(coord[0], coord[1]);
	if (coord[2] > coord[3])
		SWAP_NUMERIC(coord[2], coord[3]);

	update_selection(&image->selection,
		coord[2], coord[3], coord[0], coord[1]);
	printf("Selected %d %d %d %d\n",
		coord[0], coord[2], coord[1], coord[3]);
}

/**
 * Equalises the image histogram
*/
void equalise_image(image_t *image)
{
	if (image->type != PGM) {
		puts("Black and white image needed");
		return;
	}

	int fq[PIXEL_MAX_VALUE + 1] = { 0 };
	int hgram[PIXEL_MAX_VALUE + 1];

	/* Generate histogram frequencies */
	for (size_t i = 0; i < image->rows; i++)
		for (size_t j = 0; j < image->columns; j++)
			fq[(int)image->pixels[i][j].val]++;

	hgram[0] = fq[0];
	for (size_t i = 1; i < ARRAY_SIZE(fq); i++)
		hgram[i] = hgram[i - 1] + fq[i];

	double area = image->rows * image->columns;

	/* Equalise with formula (PIXEL_MAX * freq) / surface area of image */
	for (size_t i = 0; i < image->rows; i++)
		for (size_t j = 0; j < image->columns; j++) {
			double freq = hgram[(int)image->pixels[i][j].val];
			image->pixels[i][j].val = _clamp((PIXEL_MAX_VALUE * freq) / area,
				0, PIXEL_MAX_VALUE);
		}

	puts("Equalize done");
}

/**
 * Auxillary that calls print_histogram from image.h
*/
void _print_histogram(image_t *image, char command_line[BUFSIZ])
{
	char args[4][BUFSIZ];
	if (sscanf(command_line, "%s%s%s%s",
		args[0], args[1], args[2], args[3]) != 3) {
		puts("Invalid command");
		return;
	}

	if (image->type != PGM) {
		puts("Black and white image needed");
		return;
	}

	int stars = atoi(args[1]);
	int bins = atoi(args[2]);

	print_histogram(image, stars, bins);
}

/**
 * Auxillary that calls apply_effect from image.h
*/
void _apply_effect(image_t *image, char command_line[BUFSIZ])
{
	char args[2][BUFSIZ];
	if (sscanf(command_line, "%s%s", args[0], args[1]) != 2) {
		puts("Invalid command");
		return;
	}

	if (image->type == PGM) {
		puts("Easy, Charlie Chaplin");
		return;
	}

	/* Split into cases for kernel and divide factor creation */
	if (strcmp(args[1], "BLUR") == 0) {
		DEF_KERNEL(kernel) = { { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 } };
		apply_effect(image, image->selection, kernel, 9.0);
	} else if (strcmp(args[1], "GAUSSIAN_BLUR") == 0) {
		DEF_KERNEL(kernel) = { { 1, 2, 1 }, { 2, 4, 2 }, { 1, 2, 1 } };
		apply_effect(image, image->selection, kernel, 16.0);
	} else if (strcmp(args[1], "SHARPEN") == 0) {
		DEF_KERNEL(kernel) = { { 0, -1, 0 }, { -1, 5, -1 }, { 0, -1, 0 } };
		apply_effect(image, image->selection, kernel, 1.0);
	} else if (strcmp(args[1], "EDGE") == 0) {
		DEF_KERNEL(kernel) = { { -1, -1, -1 }, { -1, 8, -1 }, { -1, -1, -1 } };
		apply_effect(image, image->selection, kernel, 1.0);
	} else {
		puts("APPLY parameter invalid");
		return;
	}

	printf("APPLY %s done\n", args[1]);
}

/**
 * @return The angle if it is valid or -EXIT_FAILURE
*/
int _valid_angle(const char *angle)
{
	size_t it = (angle[0] == '-') ? 1 : 0;

	/* Check for non-digits */
	for (; angle[it]; it++)
		if (!isdigit(angle[it]))
			return -EXIT_FAILURE;

	int int_angle = atoi(angle);

	if (int_angle % 90)
		return -EXIT_FAILURE;
	return int_angle;
}

/**
 * @return If the image is fully selected in @a selection
*/
static inline int _selected_all(image_t *image, image_selection_t selection)
{
	return (selection.uprow == 0 && selection.lcol == 0
		&& selection.dwrow == image->rows && selection.rcol == image->columns);
}

/**
 * Auxillary that calls rotate_image or rotate_selection from image.h
*/
void _rotate_selection(image_t *image, char command_line[BUFSIZ])
{
	char args[2][BUFSIZ];
	if (sscanf(command_line, "%s%s", args[0], args[1]) != 2) {
		puts("Invalid command");
		return;
	}

	int angle = _valid_angle(args[1]);
	if (angle == -EXIT_FAILURE) {
		puts("Unsupported rotation angle");
		return;
	}

	/* Check which type of rotation */
	if (_selected_all(image, image->selection)) {
		rotate_image(image, angle);
		printf("Rotated %s\n", args[1]);
		return;
	}

	if (image->selection.dwrow - image->selection.uprow
		!= image->selection.rcol - image->selection.lcol) {
		puts("The selection must be square");
		return;
	}

	rotate_selection(image, image->selection, angle);
	printf("Rotated %s\n", args[1]);
}

/**
 *	Executes a given command line
 *
 *	@return EXIT_SUCCESS if the exit command was received
*/
static int execute_command(char command_line[BUFSIZ], image_t **image)
{
	char command[BUFSIZ];
	if (sscanf(command_line, "%s", command) != 1) {
		puts("invalid command");
		return EXIT_FAILURE;
	}

	/* Split into cases */
	if (strcmp(command, "LOAD") == 0) {
		read_image(image, command_line);
		return EXIT_FAILURE;
	}

	if (!(*image)) {
		puts("No image loaded");
		return EXIT_FAILURE;
	}

	if (strcmp(command, "EXIT") == 0) {
		free_image(*image);
		return EXIT_SUCCESS;
	} else if (strcmp(command, "SAVE") == 0) {
		save_image(command_line, *image);
	} else if (strcmp(command, "SELECT") == 0) {
		select_range(*image, command_line);
	} else if (strcmp(command, "CROP") == 0) {
		if (!_selected_all((*image), (*image)->selection))
			crop_image(*image, (*image)->selection);
		puts("Image cropped");
	} else if (strcmp(command, "EQUALIZE") == 0) {
		equalise_image(*image);
	} else if (strcmp(command, "HISTOGRAM") == 0) {
		_print_histogram(*image, command_line);
	} else if (strcmp(command, "APPLY") == 0) {
		_apply_effect(*image, command_line);
	} else if (strcmp(command, "ROTATE") == 0) {
		_rotate_selection(*image, command_line);
	} else {
		puts("Invalid command");
	}

	return EXIT_FAILURE;
}

/**
 * Entry point
*/
int main(void)
{
	image_t *image = NULL;
	char line_buf[BUFSIZ];

	/* Get the commandline then execute */
	while (fgets(line_buf, BUFSIZ, stdin))
		if (!execute_command(line_buf, &image))
			break;

	return 0;
}
