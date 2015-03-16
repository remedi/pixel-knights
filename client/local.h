#ifndef LOCAL_H
#define LOCAL_H

// Frees memory pointed by 'ptr'. Used as thread cleanup handler
void free_memory(void*);
 
// This function is a starting point for a thread.
// This thread is called when doing local testing.
// This function mimics server behaviour with it's messages.
void* local_server(void*);

#endif
