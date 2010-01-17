/*
 * Copyright (c) 2009 Jack Christopher Kastorff
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions, and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name Chris Kastorff may not be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 */

#include <die.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

/*
 * input files
 */

int getfilesamplerate(char *path) {
    SF_INFO info;
    SNDFILE *snd;

    memset(&info, 0, sizeof(SF_INFO));

    if ( (snd = sf_open(path, SFM_READ, &info)) == NULL )
        diem("Couldn't open a sound file for reading", path);

    int ret = info.samplerate;

    if ( sf_close(snd) )
        die("Couldn't close read file");

    return ret;
}

typedef struct inputfile {
    SNDFILE *sf;
    SF_INFO *info;
    double *readbuf;
    double *samples;
    int channel;
} inputfile;

inputfile * open_inputfile(char *path, int channel, int requiredsamplerate) {
    inputfile * inf;

    if ( (inf = malloc(sizeof(*inf))) == NULL )
        die("Couldn't malloc space for an inputfile structure");

    if ( (inf->info = malloc(sizeof(SF_INFO))) == NULL )
        die("Couldn't malloc space for an SF_INFO structure");

    memset(inf->info, 0, sizeof(SF_INFO));

    if ( (inf->sf = sf_open(path, SFM_READ, inf->info)) == NULL )
        diem("Couldn't open input file for reading", path);

    if ( channel > inf->info->channels )
        diem("Can't use an input file, not enough channels.", path);

    inf->channel = channel;

    if ( inf->info->samplerate != requiredsamplerate )
        die("Sample rates of input files do not match");

    if ( (inf->readbuf = malloc(sizeof(double) * inf->info->channels * BUFFER_SIZE)) == NULL )
        die("Couldn't malloc space for input buffer");

    if ( (inf->samples = malloc(sizeof(double) * BUFFER_SIZE)) == NULL )
        die("Couldn't malloc space for input buffer");

    return inf;
}

int readblock(inputfile *inf) {
    int num = sf_readf_double(inf->sf, inf->readbuf, BUFFER_SIZE);

    for (int i = 0; i < num; i++)
        inf->samples[i] = inf->readbuf[i*inf->info->channels + inf->channel-1];

    for (int i = num; i < BUFFER_SIZE; i++)
        inf->samples[i] = 0;

    return num;
}

/*
 * output files
 */

typedef struct outputfile {
    SNDFILE *sf;
    SF_INFO *info;
    double *outbuf;

    inputfile **input;
    int inputs;
} outputfile;

outputfile * open_outputfile(char *path, int inputcount, int samplerate) {
    outputfile *of;

    if ( (of = malloc(sizeof(*of))) == NULL )
        die("Couldn't malloc space for output file structure");

    if ( (of->info = malloc(sizeof(SF_INFO))) == NULL )
        die("Couldn't malloc space for SF_INFO structure");

    memset(of->info, 0, sizeof(SF_INFO));

    of->info->samplerate = samplerate;
    of->info->channels   = inputcount;
    of->info->format     = SF_FORMAT_WAV | SF_FORMAT_PCM_24 | SF_ENDIAN_FILE;

    if ( (of->sf = sf_open(path, SFM_WRITE, of->info)) == NULL )
        diem("Couldn't open output file", path);

    if ( (of->outbuf = malloc(sizeof(double) * inputcount * BUFFER_SIZE)) == NULL )
        die("Couldn't malloc space for output buffer");

    if ( (of->input = malloc(sizeof(inputfile*) * inputcount)) == NULL )
        die("Couldn't malloc space for input buffer list");

    of->inputs = inputcount;

    return of;
}

/*
 * argument parsing and main loop
 */

void mainloop(outputfile *of) {
    int maxread = 1;

    while ( maxread != 0 ) {
        maxread = 0;

        for (int i = 0; i < of->inputs; i++) {
            int this = readblock(of->input[i]);
            if ( this > maxread )
                maxread = this;
        }

        for (int i = 0; i < maxread; i++) {
            for (int j = 0; j < of->inputs; j++)
                of->outbuf[i*of->inputs+j] = of->input[j]->samples[i];
        }

        if ( maxread != 0 )
            sf_writef_double(of->sf, of->outbuf, maxread);
    }
}

void parse_input_argument(char *in, char **path, int *channel) {
    if ( (*path = malloc(sizeof(char) * strlen(in))) == NULL )
        die("Couldn't malloc space for input path argument");

    int i = 0;
    while ( in[i] && in[i] != ':' ) {
        (*path)[i] = in[i];
        i++;
    }

    if ( ! in[i] )
        diem("Couldn't parse input argument, no channel found!", in);

    *channel = atoi(in+i+1);

    if ( *channel < 1 )
        diem("Couldn't parse input argument: bad channel spec", in);
}

int main(int argc, char **argv) {
    int inputs = argc-2;
    inputfile ** input;

    if ( inputs < 1 )
        die("Need at least one input file");

    if ( (input = malloc(sizeof(inputfile*) * inputs)) == NULL )
        die("Couldn't malloc space for inputs structure");

    int rate = 0;

    for (int i = 2; i < argc; i++) {
        char *path;
        int channel;

        parse_input_argument(argv[i], &path, &channel);

        if ( ! rate )
            rate = getfilesamplerate(path);

        input[i-2] = open_inputfile(path,channel,rate);
    }

    char *outfn = argv[1];

    outputfile *of = open_outputfile(outfn, inputs, rate);

    for (int i = 0; i < inputs; i++)
        of->input[i] = input[i];

    mainloop(of);

    for (int i = 0; i < of->inputs; i++)
        sf_close(of->input[i]->sf);

    if ( sf_close(of->sf) )
        diem("Couldn't close output file", sf_strerror(of->sf));

    exit(0);
}

