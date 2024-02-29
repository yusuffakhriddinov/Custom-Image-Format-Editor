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
    char ch;

    // Read from source and write to destination
    while ((ch = fgetc(source)) != EOF) {
        fputc(ch, destination);
    }
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
        
        if (fscanf(sbuFile, "%d", &pixelIndex) != 1) {
            int count, colorIndex;
            fscanf(sbuFile, "*%d %d", &count, &colorIndex);
            
            for (int i = 0; i < count; i++) {
                fprintf(ppmFile, "%d %d %d ", colorTable[colorIndex][0], colorTable[colorIndex][1], colorTable[colorIndex][2]);
                width_Number++;
                if (width_Number == width) {
                    fprintf(ppmFile, "\n");
                    width_Number = 0;
                    height_Number++;
                    if (height_Number>height){
                        break;
                    }
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


// void convertPPMtoSBU(FILE *ppmFile, FILE *sbuFile) {
//     int width, height;
//     char magicNumber[3];
    

//     // Read PPM header information
//     fscanf(ppmFile, "P3 %d %d", &width, &height);

       
//     // Write SBU header to the SBU file
//     fprintf(sbuFile, "SBU\n%d %d\n", width, height);

//     // Create a color table to store RGB triples
//     int colorTable[width * height][3];
//     int numColors = 0; // Number of unique colors in the table

//     // Read and process pixel data from PPM file
//     for (int i = 0; i < width * height; i++) {
//         int red, green, blue;
//         fscanf(ppmFile, "%d %d %d", &red, &green, &blue);

//         // Check if the color is already in the table
//         int colorIndex = -1;
//         for (int j = 0; j < numColors; j++) {
//             if (colorTable[j][0] == red && colorTable[j][1] == green && colorTable[j][2] == blue) {
//                 colorIndex = j;
//                 break;
//             }
//         }

//         // If not in the table, add it
//         if (colorIndex == -1) {
//             colorTable[numColors][0] = red;
//             colorTable[numColors][1] = green;
//             colorTable[numColors][2] = blue;
//             colorIndex = numColors;
//             numColors++;
//         }

//         fprintf(sbuFile, "%d ", colorIndex);
//     }

//     // Write the color table entries to SBU file
//     fprintf(sbuFile, "\n%d", numColors);
//     for (int i = 0; i < numColors; i++) {
//         fprintf(sbuFile, " %d %d %d", colorTable[i][0], colorTable[i][1], colorTable[i][2]);
//     }
// }

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
    } else if(strcmp(input_extension, "sbu")==0 && strcmp(output_extension, "sbu")==0){
        printf("Both is SBU");
        copyFile(fp1, fp2);
    } else if(strcmp(input_extension, "sbu")==0 && strcmp(output_extension, "ppm")==0){
        printf("Input is sbu & output is ppm");
        convertSBUtoPPM(fp1, fp2);
    } else{
        printf("Input is ppm & output is sbu");
        
    }
    
    
    
    

    exit(0);
    return 1000;
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    
}



