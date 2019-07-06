//==============================================================================
#include <dmsg.h>
#include "files_sys.h"
#include <wordexp.h>
#include <stdio.h>
//==============================================================================
uint _files_sys::read_memory(uint32_t addr, uint8_t *data_ptr, uint size)
{
    FILE *fd = openConfigFile(addr, "r");
    if (!fd)
        return 0;
    int cnt = fread(data_ptr, 1, size, fd);
    fclose(fd);
    return cnt > 0 ? cnt : 0;
}
//==============================================================================
void _files_sys::write_memory(uint32_t addr, uint8_t *data_ptr, uint size)
{
    FILE *fd = openConfigFile(addr, "w");
    if (!fd)
        return;
    fwrite(data_ptr, size, 1, fd);
    fclose(fd);
}
//==============================================================================
FILE *_files_sys::openConfigFile(uint32_t addr, const char *fmode)
{
    static char fname[1024] = "";
    FILE *fd;
    const char *sfname = "~/";
    wordexp_t exp_result;
    wordexp(sfname, &exp_result, 0);
    memset(fname, 0, sizeof(fname));
    sprintf(fname, "%s.shiva-%u.raw", exp_result.we_wordv[0], addr);
    wordfree(&exp_result);
    fd = fopen(fname, fmode);
    //dmsg("%s\n",fname);
    return fd;
}
//==============================================================================
