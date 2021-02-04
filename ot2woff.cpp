// ot2woff.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include "ot2woff.h"

/*
int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 4)
    {
        puts(COPYRIGHT_NOTICE);

        puts("Usage: ot2woff input_font_filename[.otf|.ttf] output_font_filename.woff [compression level]\n");
        puts("       where 'compression level' is a number from 0 to 9 (highest compression)");

        return 1;
    }
    else
    {
        const char *compression_level = (argc >=4 ) ? argv[3] : nullptr;
        OT2Woff ot;
                

        if (!ot.convert(argv[1], argv[2], compression_level))
        {
            puts(ot.error().c_str());

            return 1;
        }
        else
        {
            puts("Success");

            return 0;
        }
    }
}
*/

const char* get_output_file(const char *input, string &src)
{
    size_t pos;

    src = input;

    pos = src.find_last_of('.');

    if (pos == string::npos)
        return nullptr;
    src.replace(pos + 1, string::npos, "woff");

    return src.c_str();
}

int main(int argc, char* argv[])
{
    for (int i = 0; i < argc; ++i)
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }
    if (argc < 2)
    {
        puts("No arguments");
    }
    for (int i = 1; i < argc; ++i)
    {
        string str;
        //const char* compression_level = (argc >= 4) ? argv[3] : nullptr;
        const char* output_file = get_output_file(argv[i], str);
        OT2Woff ot;


        if (!ot.convert(argv[i], output_file, "9"))
        {
            printf("%s: %s", ot.error().c_str(), argv[i]);

            return 1;
        }
        else
        {
            printf("Success (%s)\n", argv[i]);

            return 0;
        }
    }
}
/*
int main(int argc, char* argv[])
{
    const char* args[] = { "d:/cmr10.otf", "d:/cmex10.otf" };

    argc = 2;

    argv = (char **)args;

    for( int i = 1; i < argc; ++i)
    {
        string str;
        const char* compression_level = (argc >= 4) ? argv[3] : nullptr;
        const char* output_file = get_output_file(argv[i], str);
        OT2Woff ot;


        if (!ot.convert(argv[i], output_file, "9"))
        {
            puts(ot.error().c_str());

            return 1;
        }
        else
        {
            puts("Success");

            return 0;
        }
    }
}*/