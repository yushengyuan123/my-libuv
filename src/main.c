#include "stdio.h"
#include "heap-inl.h"


int main() {
    struct heap head;
    struct heap_node node;

    node.left = NULL;
    node.right = NULL;
    node.parent = NULL;

    heap_init(&head);

    printf("%d", head.nelts);
}