#include "hw2.h"


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
    size_t buffer_size = 1024; 
    char *buffer = (char *)malloc(buffer_size);
    long originalPositionSource = ftell(source);
    

    size_t bytesRead;

    // Read from source and write to destination
    while ((bytesRead = fread(buffer, 1, buffer_size, source)) > 0) {
        fwrite(buffer, 1, bytesRead, destination);
    }

    // Free the allocated memory
    free(buffer);
    fseek(source, originalPositionSource, SEEK_SET);
    
}

typedef struct {
    int r, g, b;
} RGBColor;

void convertSBUtoPPM(FILE *sbuFile, FILE *ppmFile) {
    int width, height, numColors;
    

    // Read header information from SBU file
    fscanf(sbuFile, "SBU %d %d %d", &width, &height, &numColors);

    // Create a color table to store RGB triples
    RGBColor *colorTable = (RGBColor *)malloc(numColors * sizeof(RGBColor));

    // Read color table entries from SBU file
    for (int i = 0; i < numColors; i++) {
        fscanf(sbuFile, "%d %d %d", &colorTable[i].r, &colorTable[i].g, &colorTable[i].b);
    }

    // Write PPM header to the PPM file
    fprintf(ppmFile, "P3\n%d %d\n255\n", width, height);

    // Read and process pixel data from SBU file
    int pixelIndex;
    int width_Number = 0;
    int height_Number = 1;
    while (!feof(sbuFile)) {
        if (height_Number > height) {
            break;
        }

        if (fscanf(sbuFile, "%d", &pixelIndex) != 1) {
            int count, colorIndex;
            fscanf(sbuFile, "*%d %d", &count, &colorIndex);

            for (int i = 0; i < count; i++) {
                fprintf(ppmFile, "%d %d %d ", colorTable[colorIndex].r, colorTable[colorIndex].g, colorTable[colorIndex].b);
                width_Number++;
                if (width_Number == width) {
                    height_Number++;
                    if (height_Number > height) {
                        break;
                    }
                    fprintf(ppmFile, "\n");
                    width_Number = 0;
                }
            }
        } else {
            // If pixelIndex is non-negative, use the color directly
            fprintf(ppmFile, "%d %d %d ", colorTable[pixelIndex].r, colorTable[pixelIndex].g, colorTable[pixelIndex].b);
            width_Number++;
        }

        if (width_Number == width) {
            fprintf(ppmFile, "\n");
            width_Number = 0;
            height_Number++;
            if (height_Number > height) {
                break;
            }
        }
    }

    // Free the allocated memory for the color table
    free(colorTable);
    fseek(ppmFile, 0, SEEK_CUR);
}



void convertPPMtoSBU(FILE *ppmFile, FILE *sbuFile) {
    int width, height;
    long originalPosition = ftell(ppmFile);
    // Read PPM header
    fscanf(ppmFile, "P3 %d %d 255", &width, &height);

    fprintf(sbuFile, "SBU\n%d %d\n", width, height);

    // Read and build color table dynamically
    RGBColor *colorTable = (RGBColor *)malloc(width * height * sizeof(RGBColor));
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
                } else if (runLength == 1) {
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

    // Free the allocated memory for the color table
    free(colorTable);
    fseek(ppmFile, originalPosition, SEEK_SET);
}



void skipLines(FILE *file, int linesToSkip) {
    char buffer[256];  // Adjust the buffer size as needed

    for (int i = 0; i < linesToSkip; ++i) {
        if (fgets(buffer, sizeof(buffer), file) == NULL) {
            fprintf(stderr, "Error reading from the file.\n");
            // Add appropriate error handling or exit the function
            return;
        }
    }
}
void moveToPosition(FILE *file, int rows, int cols, int width) {
    int r, g, b;
    int currentPositionColumn = 0;
    for (int i = 0; i < rows; i++){ 
        int currentPosition = 0;
        while(fscanf(file, "%d %d %d ", &r, &g, &b) == 3){
            currentPosition++;
            if(currentPosition==width){
                break;
            }
        }

    }
    while (fscanf(file, "%d %d %d ", &r, &g, &b) == 3) {
        currentPositionColumn++;

        if(currentPositionColumn==(cols)){  
            break;
        }

    }
    
}

void copyPastePPMtoPPM(FILE *source, FILE *destination, char *copy, char *paste) {
    

    int copy_row, copy_col, copy_width, copy_height;
    int paste_row, paste_col;

    sscanf(copy, "%d,%d,%d,%d", &copy_row, &copy_col, &copy_width, &copy_height);
    sscanf(paste, "%d,%d", &paste_row, &paste_col);


    

    int source_width, source_height;
    skipLines(source, 1);
    fscanf(source, "%d %d ", &source_width, &source_height);
    
    skipLines(source, 1);

    
    
    
    
    moveToPosition(source, copy_row, copy_col, source_width); //copy_row and copy_col
    
    int size = 1000000;
    int **rectangleTable = (int **)malloc(size * sizeof(int *));

    if((copy_row+copy_height)>source_height){
        copy_height = source_height-copy_row;
    }
    if((copy_col+copy_width)>source_width){
        copy_width = source_width - copy_col;
    }
    
    for (int h = 0; h<copy_height; h++){ //copy_height  
        for (int j = h*copy_width; j < copy_width*(h+1); j++) { // Assuming copy_width is 4
            rectangleTable[j] = (int *)malloc(3 * sizeof(int));
            fscanf(source, "%d %d %d ", &rectangleTable[j][0], &rectangleTable[j][1], &rectangleTable[j][2]);
            
        }
        

        int r, g, b;
        int currentPosition = 0;
        while(fscanf(source, "%d %d %d ", &r, &g, &b) == 3){
            currentPosition++;
            if(currentPosition==(source_width-copy_width)){// copy_width
                break;
            }
        }

        
    }
    
    
    
    fseek(source, 0, SEEK_SET);

    

    int width, height;
    fscanf(source, "P3 %d %d 255", &width, &height);
    fprintf(destination, "P3\n%d %d\n255\n", width, height);

    
    for (int j = 0; j<paste_row; j++){//paste_row
        for (int i=0; i<width; i++){
            int r,g,b;
            fscanf(source, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", r, g, b);
            
        }
        fprintf(destination, "\n");
    } //This is part is correct

    for(int j = 0; j<copy_height; j++){ //height is 100
        for (int i=0; i<paste_col; i++){
            int r,g,b;
            fscanf(source, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", r, g, b);
            
        }
        for(int i = j*copy_width; i<copy_width*(1+j); i++){ //copy_width is 50
            int r,g,b;
            fscanf(source, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", rectangleTable[i][0], rectangleTable[i][1], rectangleTable[i][2]);
            
        }
        for (int i = 0; i<width-copy_width-paste_col; i++){
            int r,g,b;
            fscanf(source, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", r, g, b);
        }
        fprintf(destination, "\n");
    }

    for (int j = 0; j<height-paste_row-copy_height; j++){//paste_row
        for (int i=0; i<width; i++){
            int r,g,b;
            fscanf(source, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", r, g, b);
        }
        fprintf(destination, "\n");
    }
    
    
    for (int i = 0; i < size; i++) {
        free(rectangleTable[i]);
    }

    
    free(rectangleTable);
    
}

// Copy Paste SBU to PPM

void copyPasteSBUtoPPM(FILE *source, FILE *destination, char *copy, char *paste) {
    (void) source;
    (void) destination;
    (void) copy;
    (void) paste;

    int copy_row, copy_col, copy_width, copy_height;
    int paste_row, paste_col;

    sscanf(copy, "%d,%d,%d,%d", &copy_row, &copy_col, &copy_width, &copy_height);
    sscanf(paste, "%d,%d", &paste_row, &paste_col);

    //convert SBU to PPM

    FILE *temp  = fopen("./tests/actual_outputs/temp.ppm", "w");

    convertSBUtoPPM(source, temp);
    fclose(temp);

    //Open PPM file
    FILE *readTemp = fopen("./tests/actual_outputs/temp.ppm", "r");



    

    int source_width, source_height;
    skipLines(readTemp, 1);
    fscanf(readTemp, "%d %d ", &source_width, &source_height);
    skipLines(readTemp, 1);

    printf("%d %d ", source_height, source_width);
    
    
    
    moveToPosition(readTemp, copy_row, copy_col, source_width); //copy_row and copy_col
    
    int size = 1000000;
    int **rectangleTable = (int **)malloc(size * sizeof(int *));

    if((copy_row+copy_height)>source_height){
        copy_height = source_height-copy_row;
    }
    if((copy_col+copy_width)>source_width){
        copy_width = source_width - copy_col;
    }
    
    for (int h = 0; h<copy_height; h++){ //copy_height  
        for (int j = h*copy_width; j < copy_width*(h+1); j++) { // Assuming copy_width is 4
            rectangleTable[j] = (int *)malloc(3 * sizeof(int));
            fscanf(readTemp, "%d %d %d ", &rectangleTable[j][0], &rectangleTable[j][1], &rectangleTable[j][2]);
            
        }
        

        int r, g, b;
        int currentPosition = 0;
        while(fscanf(readTemp, "%d %d %d ", &r, &g, &b) == 3){
            currentPosition++;
            if(currentPosition==(source_width-copy_width)){// copy_width
                break;
            }
        }

        
    }
    
    printf("%d %d %d ", rectangleTable[0][0], rectangleTable[0][1], rectangleTable[0][2]);
    
    fseek(readTemp, 0, SEEK_SET);

    

    int width, height;
    fscanf(readTemp, "P3 %d %d 255", &width, &height);
    fprintf(destination, "P3\n%d %d\n255\n", width, height);

    
    for (int j = 0; j<paste_row; j++){//paste_row
        for (int i=0; i<width; i++){
            int r,g,b;
            fscanf(readTemp, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", r, g, b);
            
        }
        fprintf(destination, "\n");
    } //This is part is correct

    for(int j = 0; j<copy_height; j++){ //height is 100
        for (int i=0; i<paste_col; i++){
            int r,g,b;
            fscanf(readTemp, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", r, g, b);
            
        }
        for(int i = j*copy_width; i<copy_width*(1+j); i++){ //copy_width is 50
            int r,g,b;
            fscanf(readTemp, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", rectangleTable[i][0], rectangleTable[i][1], rectangleTable[i][2]);
            
        }
        for (int i = 0; i<width-copy_width-paste_col; i++){
            int r,g,b;
            fscanf(readTemp, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", r, g, b);
        }
        fprintf(destination, "\n");
    }

    for (int j = 0; j<height-paste_row-copy_height; j++){//paste_row
        for (int i=0; i<width; i++){
            int r,g,b;
            fscanf(readTemp, "%d %d %d ", &r, &g, &b);
            fprintf(destination, "%d %d %d ", r, g, b);
        }
        fprintf(destination, "\n");
    }
    
    
    for (int i = 0; i < size; i++) {
        free(rectangleTable[i]);
    }

    
    free(rectangleTable);
    
    fclose(readTemp);
    remove("./tests/actual_outputs/temp.ppm");
    

    
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
        
        if(cflag==1 && pflag==1){
            copyPastePPMtoPPM(fp1, fp2, copy, paste);
            printf("Done");  
        }else{
            copyFile(fp1, fp2);
        }
        
        
    } else if(strcmp(input_extension, "sbu")==0 && strcmp(output_extension, "sbu")==0){
        printf("Both is SBU");
        copyFile(fp1, fp2);
        
    } else if(strcmp(input_extension, "sbu")==0 && strcmp(output_extension, "ppm")==0){
        
        if(cflag==1 && pflag==1){
            copyPasteSBUtoPPM(fp1, fp2, copy, paste);
        }else{
             convertSBUtoPPM(fp1, fp2);
        }
    } else{
        printf("Input is ppm & output is sbu");
        convertPPMtoSBU(fp1, fp2);
        
    }
    
    
    
    

    exit(0);
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    return 0;
}






