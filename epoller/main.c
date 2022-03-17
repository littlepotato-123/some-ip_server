#include <unistd.h>
#include "server.h"
sqlQueryQueue sqlQueryQueue::sqlQueryQueue_;
int main() {
    {
        Server server(9527, 3, 6000, 0, 
        3306,"root", "Root12345", "yourdb",12,
        8);
        server.Start(); 
    }
    printf("a\n");
    exit(1);
}