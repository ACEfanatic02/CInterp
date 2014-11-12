
int foo;

int bar = 12; // don't parse me bro

float f = 1.2f;

int hex = 0x1234abcdef;
int fill = 0xdcdcdcdcd;

char c = 'a';

//char oops = 'invalid';
char tab = '\t';

char * string = "this is a string!!";
char * escapes = "this \t has\nescapes!!\r\n";
char * nested = "this has a \" string\" in it";

bool r = p && q;

if (r || (q && p)) {
    do {
        func();
    } while (true);
}