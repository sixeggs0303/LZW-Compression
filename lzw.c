/*
* CSCI3280 Introduction to Multimedia Systems *
* --- Declaration --- *
* I declare that the assignment here submitted is original except for source
* material explicitly acknowledged. I also acknowledge that I am aware of
* University policy and regulations on honesty in academic work, and of the
* disciplinary guidelines and procedures applicable to breaches of such policy
* and regulations, as contained in the website
* http://www.cuhk.edu.hk/policy/academichonesty/ *
* Assignment 2
* Name : Luk Ming Ho
* Student ID : 1155094761
* Email Addr : 1155094761@link.cuhk.edu.hk
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define CODE_SIZE  12
#define TRUE 1
#define FALSE 0

typedef struct dict{
  int character;
  int prefix;
}dict;

//Global Variable
dict dictTable[4096];
int nextNode;
int leftCode = 0;
int cW, pW;


/* function prototypes */
void init_dict();
void show_dict();
void add_dict(int, int);
int search_dict(int, int);
int get_first(int);
void decode(FILE*, int);
void get_file_name(char**, char**, int);

unsigned int read_code(FILE*, unsigned int);
void write_code(FILE*, unsigned int, unsigned int);
void writefileheader(FILE *,char**,int);
void readfileheader(FILE *,char**,int *);
void compress(FILE*, FILE*);
void decompress(FILE*, FILE*);

//Initialize the dictionary with ASCII (First 256 entries + EOF)
void init_dict(){
  for(nextNode = 0; nextNode <= 255; nextNode++){
    dictTable[nextNode].character = nextNode;
    dictTable[nextNode].prefix = -1;
  }
  dictTable[4095].character = EOF;
  dictTable[4095].prefix = -1;
}

//Print all the content in dictionary
void show_dict(){
  for(int i = 0; i < nextNode; i++)
  printf("[%d] = %c, prefix: %d\n", i, dictTable[i].character, dictTable[i].prefix);
}

//Add new item into dictionary
void add_dict(int prefix, int character){
  dictTable[nextNode].prefix = prefix;
  dictTable[nextNode].character = character;
  nextNode++;
}

//Search if the character exist
int search_dict(int prefix, int character){
  for(int pass = 0; pass < nextNode; pass++){
    if((dictTable[pass].prefix == prefix) && (dictTable[pass].character == character)){
      return pass;
    }
  }
  return -1;
}

//Return the first character of string in dictionary
int get_first(int index){
  if(dictTable[index].prefix == -1){
    return dictTable[index].character;
  }else{
    return get_first(dictTable[index].prefix);
  }
}

//Write the whole string into file
void decode(FILE* output, int index){
  if (dictTable[index].prefix != -1){
    decode(output, dictTable[index].prefix);
  }
  fputc(dictTable[index].character, output);
}

//Get single file name from the output_filenames
void get_file_name(char** output_file_names,char** filename, int fileIndex){
  int head = 0;
  int end = 0;
  //Get the length of the file name
  for(int pass = 0; pass <= fileIndex; pass++){
    head = end;
    while( (*output_file_names)[end] != '\n'){
      end++;
    }
    end++;
  }
  //Create file name for file name
  *filename = (char*) malloc(sizeof(char) * (end - head));
  int location;
  int filename_index = 0;
  //Get the file name
  for(location = head; location < end-1; location++){
    (*filename)[filename_index++] = (*output_file_names)[location];
  }
  (*filename)[filename_index] = '\0';

}

//Main function
int main(int argc, char **argv)
{
  int printusage = 0;
  int	no_of_file;
  char **input_file_names;
  char *output_file_names;
  FILE *lzw_file;
  char *filename = NULL;
  if (argc >= 3)
  {
    if ( strcmp(argv[1],"-c") == 0)
    {
      /* compression */
      lzw_file = fopen(argv[2] ,"wb");

      /* write the file header */
      input_file_names = argv + 3;
      no_of_file = argc - 3;
      writefileheader(lzw_file,input_file_names,no_of_file);

      //Initialize dictionary
      init_dict();

      //Loop for no. of files
      for(int fileIndex = 0; fileIndex < no_of_file; fileIndex++){
        //Open file to write
        FILE *input_file = fopen(argv[3+fileIndex] ,"rb");
        //Compress it
        compress(input_file,lzw_file);
        printf("[%s] Compressed\n", argv[3+fileIndex]);
      }
      //Flush the buffer
      if(leftCode){
        write_code(lzw_file, 0, leftCode);
      }else{
        write_code(lzw_file, 0, CODE_SIZE);
      }

      fclose(lzw_file);

    } else
    if ( strcmp(argv[1],"-d") == 0)
    {
      /* decompress */
      lzw_file = fopen(argv[2] ,"rb");

      /* read the file header */
      no_of_file = 0;
      readfileheader(lzw_file,&output_file_names,&no_of_file);
      //Initialize dictionary
      init_dict();
      //Loop for no. of files
      for(int fileIndex = 0; fileIndex < no_of_file; fileIndex++){
        //Get the file name in the file name list
        get_file_name(&output_file_names, &filename, fileIndex);
        FILE *output_file = fopen(filename ,"wb");
        //Decompress it
        decompress(lzw_file,output_file);
        printf("[%s] dcompressed\n", filename);
        //show_dict();
        free(filename);
      }
      fclose(lzw_file);
      free(output_file_names);
    }else
    printusage = 1;
  }else
  printusage = 1;

  if (printusage)
  printf("Usage: %s -<c/d> <lzw filename> <list of files>\n",argv[0]);

  return 0;
}

/*****************************************************************
*
* writefileheader() -  write the lzw file header to support multiple files
*
****************************************************************/
void writefileheader(FILE *lzw_file,char** input_file_names,int no_of_files)
{
  int i;
  /* write the file header */
  for ( i = 0 ; i < no_of_files; i++)
  {
    fprintf(lzw_file,"%s\n",input_file_names[i]);

  }
  fputc('\n',lzw_file);

}

/*****************************************************************
*
* readfileheader() - read the fileheader from the lzw file
*
****************************************************************/
void readfileheader(FILE *lzw_file,char** output_filenames,int * no_of_files)
{
  int noofchar;
  char c,lastc;

  noofchar = 0;
  lastc = 0;
  *no_of_files=0;
  /* find where is the end of double newline */
  while((c = fgetc(lzw_file)) != EOF)
  {
    noofchar++;
    if (c =='\n')
    {
      if (lastc == c )
      /* found double newline */
      break;
      (*no_of_files)++;
    }
    lastc = c;
  }

  if (c == EOF)
  {
    /* problem .... file may have corrupted*/
    *no_of_files = 0;
    return;

  }
  /* allocate memeory for the filenames */
  *output_filenames = (char *) malloc(sizeof(char)*noofchar);
  /* roll back to start */
  fseek(lzw_file,0,SEEK_SET);

  fread((*output_filenames),1,(size_t)noofchar,lzw_file);

  return;
}

/*****************************************************************
*
* read_code() - reads a specific-size code from the code file
*
****************************************************************/
unsigned int read_code(FILE *input, unsigned int code_size)
{
  unsigned int return_value;
  static int input_bit_count = 0;
  static unsigned long input_bit_buffer = 0L;

  /* The code file is treated as an input bit-stream. Each     */
  /*   character read is stored in input_bit_buffer, which     */
  /*   is 32-bit wide.                                         */

  /* input_bit_count stores the no. of bits left in the buffer */

  while (input_bit_count <= 24) {
    input_bit_buffer |= (unsigned long) getc(input) << (24-input_bit_count);
    input_bit_count += 8;
  }

  return_value = input_bit_buffer >> (32 - code_size);
  input_bit_buffer <<= code_size;
  input_bit_count -= code_size;

  return(return_value);
}


/*****************************************************************
*
* write_code() - write a code (of specific length) to the file
*
****************************************************************/
void write_code(FILE *output, unsigned int code, unsigned int code_size)
{
  static int output_bit_count = 0;
  static unsigned long output_bit_buffer = 0L;

  /* Each output code is first stored in output_bit_buffer,    */
  /*   which is 32-bit wide. Content in output_bit_buffer is   */
  /*   written to the output file in bytes.                    */

  /* output_bit_count stores the no. of bits left              */

  output_bit_buffer |= (unsigned long) code << (32-code_size-output_bit_count);
  output_bit_count += code_size;

  while (output_bit_count >= 8) {
    putc(output_bit_buffer >> 24, output);
    output_bit_buffer <<= 8;
    output_bit_count -= 8;
  }


  /* only < 8 bits left in the buffer                          */

}

/*****************************************************************
*
* compress() - compress the source file and output the coded text
*
****************************************************************/
void compress(FILE *input, FILE *output)
{
  int P,C;
  //Get the first character
  P = fgetc(input);
  //End if the file is empty
  if (P == EOF) return;
  //Get the second character
  C = fgetc(input);
  //While the read character is not EOF
  while(C != EOF){
    int search = search_dict(P,C);
    //If the string is in dictionary
    if(search != -1){
      //Make it as prefix
      P = search;
    }else{
      //New string found
      //Print the prefix to file
      write_code(output, P, CODE_SIZE);
      leftCode = (leftCode + 12) % 8;
      //Add <P,C> to dictionary
      add_dict(P, C);
      //Current character will become the head
      P = C;
    }
    //Get next character
    C = fgetc(input);
    //If the dictionary is full, remake it
    if(nextNode == 4096){
      init_dict();
    }

  }
  //Print the last prefix and EOF to indicate end
  write_code(output, P, CODE_SIZE);
  leftCode = (leftCode + 12) % 8;
  write_code(output, 4095, CODE_SIZE);
  leftCode = (leftCode + 12) % 8;

}


/*****************************************************************
*
* decompress() - decompress a compressed file to the orig. file
*
****************************************************************/
void decompress(FILE *input, FILE *output)
{
  int readCW, newChar;
  //Read the first codeword
  cW = read_code(input, 12) & 0xFFF;

  //Print the string represented by that codeword
  decode(output, cW);

  //Read the next codeword
  readCW = read_code(input, 12)& 0xFFF;

  //Loop if the read codeword is not 4095 -> which is EOF
  while(readCW != 4095){
    //Load pW and cW
    pW = cW;
    cW = readCW;
    //If the codeword exist in the dictionary
    if (cW < nextNode){
      decode(output, cW);
      newChar = get_first(cW);
      add_dict(pW, newChar);
    }else{
      //If the codeword does not exist in the dictionary
      newChar = get_first(pW);
      decode(output, pW);
      fputc(newChar, output);
      add_dict(pW, newChar);
    }

    if (nextNode == 4096){
      init_dict();
    }
    readCW = read_code(input, 12)& 0xFFF;
  }
}
