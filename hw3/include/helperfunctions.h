//CUSTOM FUNCTIONS
int set_logues();

int get_blockSize(int size);

int checkFree(void* pp);
int coalesce(void* freeBlockHeader);

void generateFBfromFL();

void* split(sf_free_list_node* node_ptr, int rqsize);
