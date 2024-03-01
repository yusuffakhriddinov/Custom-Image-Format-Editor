//#include "hw2.h"

#define MISSING_ARGUMENT 1
#define UNRECOGNIZED_ARGUMENT 2
#define DUPLICATE_ARGUMENT 3
#define INPUT_FILE_MISSING 4
#define OUTPUT_FILE_UNWRITABLE 5
#define C_ARGUMENT_MISSING 6
#define C_ARGUMENT_INVALID 7
#define P_ARGUMENT_INVALID 8
#define R_ARGUMENT_INVALID 9

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> 
#include <getopt.h>
#include <bits/getopt_core.h>


int count_numbers(char *str) {
    int count = 0;
    for (char *ptr = str; *ptr != '\0'; ++ptr) {
        if (*ptr == ',') {
            count++;
        }
    }
    if (str[strlen(str) - 1] == ',') {
        count--;
    }
    return count + 1;
}
char* get_r_file_parameter(char *str) {
    // Find the first comma
    char *start_ptr = strchr(str, ',');

    // Check if a comma was found
    if (start_ptr != NULL) {
        // Move to the next character after the comma
        start_ptr++;

        // Find the next comma or the end of the string
        char *end_ptr = strchr(start_ptr, ',');

        // Calculate the length of the second part
        int length = (end_ptr != NULL) ? (int)(end_ptr - start_ptr) : (int)strlen(start_ptr);

        // Allocate memory for the result string
        char *result = (char*)malloc(length + 1);

        // Copy the second part into the result string
        strncpy(result, start_ptr, length);

        // Null-terminate the result string
        result[length] = '\0';

        return result;
    } else {
        // No comma found, return NULL or handle the error as needed
        return NULL;
    }
}

const char *getFileExtension(const char *filename) {
    const char *dot = strrchr(filename, '.');

    // Check if dot is present and it's not the first character in the filename
    if (dot != NULL && dot != filename) {
        return dot + 1; // Return the substring after the dot
    } else if (dot == filename) {
        return dot + 1; // Return the substring after the dot if dot is the first character
    }

    return ""; // Return an empty string if no extension is found
}

void copyFile(FILE *source, FILE *destination) {
    // Determine the size of the file
    fseek(source, 0, SEEK_END);
    long fileSize = ftell(source);
    fseek(source, 0, SEEK_SET);

    // Allocate memory to store the file content
    char *buffer = (char *)malloc(fileSize);
    if (buffer == NULL) {
        perror("Memory allocation failed");
        return;
    }

    // Read the content of the file into the buffer
    size_t bytesRead = fread(buffer, 1, (size_t)fileSize, source);
    if (bytesRead != (size_t)fileSize) {
        perror("Error reading from file");
        free(buffer);
        return;
    }

    // Write the buffer to the destination file
    size_t bytesWritten = fwrite(buffer, 1, bytesRead, destination);
    if (bytesWritten != bytesRead) {
        perror("Error writing to file");
    }

    // Free the allocated memory
    free(buffer);
}

void convertSBUtoPPM(FILE *sbuFile, FILE *ppmFile) {
    int width, height, numColors;
    

    // Read header information from SBU file
    fscanf(sbuFile, "SBU %d %d %d", &width, &height, &numColors);
    

    // Create a color table to store RGB triples
    int colorTable[numColors][3];

    // Read color table entries from SBU file
    for (int i = 0; i < numColors; i++) {
        fscanf(sbuFile, "%d %d %d", &colorTable[i][0], &colorTable[i][1], &colorTable[i][2]);
        
    }

    // Write PPM header to the PPM file
    fprintf(ppmFile, "P3\n%d %d\n255\n", width, height);

    // Read and process pixel data from SBU file
    int pixelIndex;
    int width_Number = 0;
    int height_Number = 1;
    while (!feof(sbuFile)) {
        if (height_Number>height){
            break;
        }
        
        if (fscanf(sbuFile, "%d", &pixelIndex) != 1) {
            int count, colorIndex;
            fscanf(sbuFile, "*%d %d", &count, &colorIndex);
            
            for (int i = 0; i < count; i++) {
                fprintf(ppmFile, "%d %d %d ", colorTable[colorIndex][0], colorTable[colorIndex][1], colorTable[colorIndex][2]);
                width_Number++;
                if (width_Number == width) {
                    height_Number++;
                    if (height_Number>height){
                        break;
                    }
                    fprintf(ppmFile, "\n");
                    width_Number = 0;
                    
                }
            }
        } else {
            // If pixelIndex is non-negative, use the color directly
            fprintf(ppmFile, "%d %d %d ", colorTable[pixelIndex][0], colorTable[pixelIndex][1], colorTable[pixelIndex][2]);
            width_Number++;

        }
        if (width_Number == width) {
                fprintf(ppmFile, "\n");
                width_Number = 0;
                height_Number++;
                if (height_Number>height){
                    break;
                }
        }
        
        
    }
}

// Structure to represent RGB color
typedef struct {
    int r, g, b;
} RGBColor;

// Function to read PPM file and convert it to SBU format
void convertPPMtoSBU(FILE *ppmFile, FILE *sbuFile) {

    int width, height;
    
    // Read PPM header
    fscanf(ppmFile, "P3 %d %d 255",  &width, &height);

    fprintf(sbuFile, "SBU\n%d %d\n", width, height);
    

    // Read and build color table
    RGBColor colorTable[width*height];
    int colorTableSize = 0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            RGBColor pixel;
            fscanf(ppmFile, "%d %d %d", &pixel.r, &pixel.g, &pixel.b);

            int found = 0;
            for (int k = 0; k < colorTableSize; k++) {
                if (colorTable[k].r == pixel.r && colorTable[k].g == pixel.g && colorTable[k].b == pixel.b) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                colorTable[colorTableSize] = pixel;
                colorTableSize++;
            }
        }
    }

    // Write SBU header
    fprintf(sbuFile, "%d\n", colorTableSize);

    // Write color table
    for (int i = 0; i < colorTableSize; i++) {
        fprintf(sbuFile, "%d %d %d ", colorTable[i].r, colorTable[i].g, colorTable[i].b);
    }

    fprintf(sbuFile, "\n");

    // Write pixel data
    // Reset file pointer to read pixel data
    fseek(ppmFile, 0, SEEK_SET);
    fscanf(ppmFile, "P3 %*d %*d %*d"); // Skip PPM header

    int prevPixelIndex = -1;
    int runLength = 0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            RGBColor pixel;
            fscanf(ppmFile, "%d %d %d", &pixel.r, &pixel.g, &pixel.b);

            // Find the index of the current pixel in the color table
            int currentPixelIndex = -1;
            for (int k = 0; k < colorTableSize; k++) {
                if (colorTable[k].r == pixel.r && colorTable[k].g == pixel.g && colorTable[k].b == pixel.b) {
                    currentPixelIndex = k;
                    break;
                }
            }

            // Check for a run of identical pixels
            if (currentPixelIndex != -1 && runLength > 0 && currentPixelIndex == prevPixelIndex) {
                runLength++;
            } else {
                // Write the previous run if any
                if (runLength > 1) {
                    fprintf(sbuFile, "*%d %d ", runLength, prevPixelIndex);
                }else if(runLength==1){
                    fprintf(sbuFile, "%d ", prevPixelIndex);
                }
                // Write the current pixel
                
                runLength = 1;
                prevPixelIndex = currentPixelIndex;
            }
        }
    }

    // Write the last run if any
    if (runLength > 0) {
        fprintf(sbuFile, "*%d %d ", runLength, prevPixelIndex);
    }

}




//copy-paste functions
#define DELIMITER ","
#define MAX_BUFFER_SIZE 256

void skipLines(FILE *file, int numLines) {
    for (int i = 0; i < numLines; i++) {
        int c;
        while ((c = fgetc(file)) != '\n' && c != EOF) {
        }
    }
}

void moveToPosition(FILE *file, int rows, int cols, int width) {
    for (int i = 0; i < rows - 1; i++) {
        fseek(file, 3 * width, SEEK_CUR);
    }

    fseek(file, 3 * cols, SEEK_CUR);
}

void copyPastePPMtoPPM(FILE *source, FILE *destination, char *copy, char *paste) {
    int copy_row, copy_col, copy_width, copy_height;
    int paste_row, paste_col;
    int source_width, source_height, destination_width, destination_height;

    fscanf(source, "P3\n%d %d\n255", &source_width, &source_height);
    fscanf(destination, "P3\n%d %d\n255", &destination_width, &destination_height);

    const char delimiter[] = ",";
    char input_copy[strlen(copy) + 1];
    strcpy(input_copy, copy);

    char *token = strtok(input_copy, delimiter);
    if (token != NULL) {
        copy_row = atoi(token);
    }
    token = strtok(NULL, delimiter);

    if (token != NULL) {
        copy_col = atoi(token);
    }
    token = strtok(NULL, delimiter);

    if (token != NULL) {
        copy_width = atoi(token);
    }
    token = strtok(NULL, delimiter);

    if (token != NULL) {
        copy_height = atoi(token);
    }

    char paste_copy[strlen(paste) + 1];
    strcpy(paste_copy, paste);

    token = strtok(paste_copy, delimiter);
    if (token != NULL) {
        paste_row = atoi(token);
    }
    token = strtok(NULL, delimiter);

    if (token != NULL) {
        paste_col = atoi(token);
    }

    skipLines(source, 3);
    moveToPosition(source, copy_row, copy_col, source_width);

    int colorTable[copy_height * copy_width][3];
    for (int i = 0; i < copy_height * copy_width; i++) {
        fscanf(source, "%d %d %d ", &colorTable[i][0], &colorTable[i][1], &colorTable[i][2]);
    }

    skipLines(destination, 3);
    moveToPosition(destination, paste_row, paste_col, destination_width);

    for (int i = 0; i < copy_height * copy_width; i++) {
        // fprintf(destination, "%d %d %d ", colorTable[i][0], colorTable[i][1], colorTable[i][2]);
        fprintf(destination, "%d %d %d ", 0, 0, 0);
    }
}

int main(int argc, char **argv) {
    int option;
    int highest_priority = 0;

    int iflag = 0;
    int oflag = 0;
    int pflag = 0;
    int rflag = 0;
    int cflag = 0;
    int p_provided = 0;
    int c_provided = 0;
    FILE *fp1;
    FILE *fp2;
    FILE *fp3;
    const char* input_extension;
    const char* output_extension;

    //copy-paste
    char* copy;
    char* paste;
    
    while ((option = getopt(argc, argv, "i:o:p:r:c:")) != -1) {
        switch (option) {
            case 'i':
                fp1 = fopen(optarg, "r");
                if (optarg == NULL || optarg[0] == '-') {
                    highest_priority = MISSING_ARGUMENT;
                } else if (iflag) {
                    highest_priority = DUPLICATE_ARGUMENT;
                }else {
                    iflag++;
                }
                input_extension = getFileExtension(optarg);
                break;

            case 'o':
                fp2 = fopen(optarg, "w");
                if (optarg == NULL || optarg[0] == '-') {
                    highest_priority = MISSING_ARGUMENT;
                } else if (oflag) {
                    highest_priority = DUPLICATE_ARGUMENT;
                } else {
                    oflag++;
                }
                output_extension = getFileExtension(optarg);
                break;

            case 'p':
                if (optarg == NULL || optarg[0] == '-') {
                    highest_priority = MISSING_ARGUMENT;
                } else if (pflag) {
                    highest_priority = DUPLICATE_ARGUMENT;
                } else if (count_numbers(optarg) != 2) {
                    highest_priority = P_ARGUMENT_INVALID;
                } else {
                    pflag++;
                    p_provided++;
                }
                paste=optarg;

                break;

            case 'r':
                printf("%s", get_r_file_parameter(optarg));
                if(optarg!=NULL){
                    fp3 = fopen(get_r_file_parameter(optarg), "r");
                }
                
                if (optarg == NULL || optarg[0] == '-') {
                    highest_priority = MISSING_ARGUMENT;
                } else if (rflag) {
                    highest_priority = DUPLICATE_ARGUMENT;
                } else if (count_numbers(optarg) != 5) {
                    highest_priority = R_ARGUMENT_INVALID;
                } else {
                    rflag++;
                }
                break;

            case 'c':
                if (cflag) {
                    highest_priority = DUPLICATE_ARGUMENT;
                } else if (count_numbers(optarg) != 4) {
                    highest_priority = C_ARGUMENT_INVALID;
                } else {
                    cflag++;
                    c_provided++;
                }
                copy = optarg;
                break;

            case '?':
                if (optopt == 'i' || optopt == 'o' || optopt == 'p' || optopt == 'r' || optopt == 'c') {
                    highest_priority = MISSING_ARGUMENT;
                } else {
                    highest_priority = UNRECOGNIZED_ARGUMENT;
                }
                break;

            default:
                highest_priority = MISSING_ARGUMENT;
                break;
        }

        if (highest_priority > 0 && highest_priority != R_ARGUMENT_INVALID) {
            printf("%s\n", highest_priority == MISSING_ARGUMENT ? "MISSING_ARGUMENT" :
                             highest_priority == UNRECOGNIZED_ARGUMENT ? "UNRECOGNIZED_ARGUMENT" :
                             highest_priority == DUPLICATE_ARGUMENT ? "DUPLICATE_ARGUMENT" :
                             highest_priority == C_ARGUMENT_MISSING ? "C_ARGUMENT_MISSING" :
                             highest_priority == C_ARGUMENT_INVALID ? "C_ARGUMENT_INVALID" :
                             highest_priority == P_ARGUMENT_INVALID ? "P_ARGUMENT_INVALID" :
                             highest_priority == R_ARGUMENT_INVALID ? "R_ARGUMENT_INVALID" :
                             highest_priority == INPUT_FILE_MISSING ? "INPUT_FILE_MISSING" :
                             highest_priority == OUTPUT_FILE_UNWRITABLE ? "OUTPUT_FILE_UNWRITABLE" : "UNKNOWN_ERROR");

            exit(highest_priority);
        }
    }

    if (iflag == 0 || oflag == 0) {
        printf("MISSING_ARGUMENT1\n");
        exit(MISSING_ARGUMENT);
    }
    if(fp1==NULL){
        printf("INPUT_FILE_MISSING");
        exit(INPUT_FILE_MISSING);
    }
    if(fp2==NULL){
        printf("OUTPUT_FILE_UNWRITABLE");
        exit(OUTPUT_FILE_UNWRITABLE);
    }
    if (p_provided == 1 && c_provided == 0) {
        printf("C_ARGUMENT_MISSING\n");
        exit(C_ARGUMENT_MISSING);
    }

    if (highest_priority == R_ARGUMENT_INVALID || (fp3==NULL && rflag>0)) {
        printf("R_ARGUMENT_INVALID\n");
        exit(R_ARGUMENT_INVALID);
    }
    
    //PART 2
    

    // Read header information from SBU file
    
    
    if(strcmp(input_extension, "ppm")==0 && strcmp(output_extension, "ppm")==0){
        printf("Both is ppm");
        copyFile(fp1, fp2);
        
        if (cflag==1 && pflag==1){
            copyPastePPMtoPPM(fp1, fp2, copy, paste);
            printf("Done");
            
        }
    } else if(strcmp(input_extension, "sbu")==0 && strcmp(output_extension, "sbu")==0){
        printf("Both is SBU");
        copyFile(fp1, fp2);
    } else if(strcmp(input_extension, "sbu")==0 && strcmp(output_extension, "ppm")==0){
        printf("Input is sbu & output is ppm");
        convertSBUtoPPM(fp1, fp2);
    } else{
        printf("Input is ppm & output is sbu");
        convertPPMtoSBU(fp1, fp2);
        
    }
    
    
    
    

    exit(0);
    return 1000;
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    
}



