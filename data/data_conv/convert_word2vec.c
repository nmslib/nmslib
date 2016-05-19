//  Copyright 2013 Google Inc. All Rights Reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

// The conversion utility is taken from here: http://stackoverflow.com/questions/27324292/convert-word2vec-bin-file-to-text/27329142#27329142

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

const long long max_size = 2000;         // max length of strings
const long long N = 40;                  // number of closest words that will be shown
const long long max_w = 50;              // max length of vocabulary entries

int main(int argc, char **argv) {
  FILE *f;
  char file_name[max_size];
  float len;
  long long words, size, a, b;
  char ch;
  float *M;
  char *vocab;
  if (argc < 2) {
    fprintf(stderr, "Usage: ./convert_word2vec <FILE>\nwhere FILE contains word projections in the BINARY FORMAT\n");
    return 0;
  }
  strcpy(file_name, argv[1]);
  f = fopen(file_name, "rb");
  if (f == NULL) {
    fprintf(stderr, "Input file not found\n");
    return -1;
  }
  if (fscanf(f, "%lld", &words) != 1) {
    fprintf(stderr, "Error reading from input file!");
  }
  if (fscanf(f, "%lld", &size) != 1) {
    fprintf(stderr, "Error reading from input file!");
  }
  vocab = (char *)malloc((long long)words * max_w * sizeof(char));
  M = (float *)malloc((long long)words * (long long)size * sizeof(float));
  if (M == NULL) {
    fprintf(stderr, "Cannot allocate memory: %lld MB    %lld  %lld\n", (long long)words * size * sizeof(float) / 1048576, words, size);
    return -1;
  }
  for (b = 0; b < words; b++) {
    if (fscanf(f, "%s%c", &vocab[b * max_w], &ch) != 2) {
      fprintf(stderr, "Error reading from input file!");
    }
    
    for (a = 0; a < size; a++) {
      if (fread(&M[a + b * size], sizeof(float), 1, f) != 1) {
        fprintf(stderr, "Error reading from input file!");
      }
    }
    len = 0;
    for (a = 0; a < size; a++) len += M[a + b * size] * M[a + b * size];
    len = sqrt(len);
    for (a = 0; a < size; a++) M[a + b * size] /= len;
  }
  fclose(f);
  //Code added by Thomas Mensink
  //output the vectors of the binary format in text
  fprintf(stderr,"%lld %lld #File: %s\n",words,size,file_name);
  for (a = 0; a < words; a++){
    printf("%s ",&vocab[a * max_w]);
    for (b = 0; b< size; b++){ printf("%.9f ",M[a*size + b]); }
    printf("\n");
  }  

  return 0;
}
