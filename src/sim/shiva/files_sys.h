//==============================================================================
#ifndef files_sys_H
#define files_sys_H
#include "files.h"
#include <iostream>
//==============================================================================
class _files_sys : public _files
{
protected:
    uint read_memory(uint32_t addr, uint8_t *data_ptr, uint size);
    void write_memory(uint32_t addr, uint8_t *data_ptr, uint size);

private:
    FILE *openConfigFile(uint32_t addr, const char *fmode);
};
//==============================================================================
#endif
