#include <common.h>

int main(int argc, char **argv)
{
    // init
    sf_init();
    display_init(false);

    // runtime
    display_hndlr();
}
